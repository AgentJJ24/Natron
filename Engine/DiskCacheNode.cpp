/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "DiskCacheNode.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/RenderQueue.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;

struct DiskCacheNodePrivate
{
    KnobChoiceWPtr frameRange;
    KnobIntWPtr firstFrame;
    KnobIntWPtr lastFrame;
    KnobButtonWPtr preRender;

    DiskCacheNodePrivate()
    {
    }
};

PluginPtr
DiskCacheNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create((void*)DiskCacheNode::create, PLUGINID_NATRON_DISKCACHE, "DiskCache", 1, 0, grouping);

    QString desc =  tr("This node caches all images of the connected input node onto the disk with full 32bit floating point raw data. "
                       "When an image is found in the cache, %1 will then not request the input branch to render out that image. "
                       "The DiskCache node only caches full images and does not split up the images in chunks.  "
                       "The DiskCache node is useful if you're working with a large and complex node tree: this allows to break the tree into smaller "
                       "branches and cache any branch that you're no longer working on. The cached images are saved by default in the same directory that is used "
                       "for the viewer cache but you can set its location and size in the preferences. A solid state drive disk is recommended for efficiency of this node. "
                       "By default all images that pass into the node are cached but they depend on the zoom-level of the viewer. For convenience you can cache "
                       "a specific frame range at scale 100% much like a writer node would do.\n"
                       "WARNING: The DiskCache node must be part of the tree when you want to read cached data from it.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) );
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/diskcache_icon.png");
    return ret;
}


DiskCacheNode::DiskCacheNode(const NodePtr& node)
    : EffectInstance(node)
    , _imp( new DiskCacheNodePrivate() )
{
}

DiskCacheNode::~DiskCacheNode()
{
}

void
DiskCacheNode::addAcceptedComponents(int /*inputNb*/,
                                     std::bitset<4>* supported)
{
    (*supported)[0] = (*supported)[1] = (*supported)[2] = (*supported)[3] = 1;

}

void
DiskCacheNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

bool
DiskCacheNode::shouldCacheOutput(bool /*isFrameVaryingOrAnimated*/,
                                 const TreeRenderNodeArgsPtr& /*render*/,
                                 int /*visitsCount*/) const
{
    // The disk cache node always caches.
    return true;
}

void
DiskCacheNode::initializeKnobs()
{
    KnobPagePtr page = AppManager::createKnob<KnobPage>( shared_from_this(), tr("Controls") );
    KnobChoicePtr frameRange = AppManager::createKnob<KnobChoice>( shared_from_this(), tr("Frame range") );

    frameRange->setName("frameRange");
    frameRange->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("Input frame range", "", ""));
        choices.push_back(ChoiceOption("Project frame range", "", ""));
        choices.push_back(ChoiceOption("Manual","", ""));
        frameRange->populateChoices(choices);
    }
    frameRange->setEvaluateOnChange(false);
    frameRange->setDefaultValue(0);
    page->addKnob(frameRange);
    _imp->frameRange = frameRange;

    KnobIntPtr firstFrame = AppManager::createKnob<KnobInt>( shared_from_this(), tr("First Frame") );
    firstFrame->setAnimationEnabled(false);
    firstFrame->setName("firstFrame");
    firstFrame->disableSlider();
    firstFrame->setEvaluateOnChange(false);
    firstFrame->setAddNewLine(false);
    firstFrame->setDefaultValue(1);
    firstFrame->setSecret(true);
    page->addKnob(firstFrame);
    _imp->firstFrame = firstFrame;

    KnobIntPtr lastFrame = AppManager::createKnob<KnobInt>( shared_from_this(), tr("Last Frame") );
    lastFrame->setAnimationEnabled(false);
    lastFrame->setName("LastFrame");
    lastFrame->disableSlider();
    lastFrame->setEvaluateOnChange(false);
    lastFrame->setDefaultValue(100);
    lastFrame->setSecret(true);
    page->addKnob(lastFrame);
    _imp->lastFrame = lastFrame;

    KnobButtonPtr preRender = AppManager::createKnob<KnobButton>( shared_from_this(), tr("Pre-cache") );
    preRender->setName("preRender");
    preRender->setEvaluateOnChange(false);
    preRender->setHintToolTip( tr("Cache the frame range specified by rendering images at zoom-level 100% only.") );
    page->addKnob(preRender);
    _imp->preRender = preRender;
}

bool
DiskCacheNode::knobChanged(const KnobIPtr& k,
                           ValueChangedReasonEnum /*reason*/,
                           ViewSetSpec /*view*/,
                           TimeValue /*time*/)
{
    bool ret = true;

    if (_imp->frameRange.lock() == k) {
        int idx = _imp->frameRange.lock()->getValue(DimIdx(0));
        switch (idx) {
        case 0:
        case 1:
            _imp->firstFrame.lock()->setSecret(true);
            _imp->lastFrame.lock()->setSecret(true);
            break;
        case 2:
            _imp->firstFrame.lock()->setSecret(false);
            _imp->lastFrame.lock()->setSecret(false);
            break;
        default:
            break;
        }
    } else if (_imp->preRender.lock() == k) {
        RenderQueue::RenderWork w;
        w.renderLabel = tr("Caching").toStdString();
        w.treeRoot = getNode();
        w.frameStep = TimeValue(1.);
        w.useRenderStats = false;
        std::list<RenderQueue::RenderWork> works;
        works.push_back(w);
        getApp()->getRenderQueue()->renderNonBlocking(works);
    } else {
        ret = false;
    }

    return ret;
}

ActionRetCodeEnum
DiskCacheNode::getFrameRange(const TreeRenderNodeArgsPtr& render,
                             double *first,
                             double *last)
{
    int idx = _imp->frameRange.lock()->getValue();

    switch (idx) {
    case 0: {
        EffectInstancePtr input = getInput(0);
        if (input) {
            TreeRenderNodeArgsPtr inputRender;
            if (render) {
                inputRender = render->getInputRenderArgs(0);
            }
            GetFrameRangeResultsPtr results;
            ActionRetCodeEnum stat = input->getFrameRange_public(inputRender, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            RangeD range;
            results->getFrameRangeResults(&range);
            *first = range.min;
            *last = range.max;

        }
        break;
    }
    case 1: {
        TimeValue left, right;
        getApp()->getProject()->getFrameRange(&left, &right);
        *first = left;
        *last = right;
        break;
    }
    case 2: {
        *first = _imp->firstFrame.lock()->getValue();
        *last = _imp->lastFrame.lock()->getValue();
    };
    default:
        break;
    }
    return eActionStatusOK;
}

ActionRetCodeEnum
DiskCacheNode::render(const RenderActionArgs& args)
{
    // fetch source images and copy them

    for (std::list<std::pair<ImagePlaneDesc, ImagePtr > >::const_iterator it = args.outputPlanes.begin(); it != args.outputPlanes.end(); ++it) {
        std::list<ImagePlaneDesc> layersToFetch;
        layersToFetch.push_back(it->first);

        GetImageInArgs inArgs(args);
        inArgs.inputNb = 0;
        inArgs.layers = &layersToFetch;
        GetImageOutArgs outArgs;
        if (!getImagePlanes(inArgs, &outArgs)) {
            return eActionStatusInputDisconnected;
        }

        ImagePtr inputImage = outArgs.imagePlanes.begin()->second;

        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = args.roi;
        it->second->copyPixels(*inputImage, cpyArgs);

    }
    return eActionStatusOK;

} // render


bool
DiskCacheNode::isHostChannelSelectorSupported(bool* /*defaultR*/,
                                              bool* /*defaultG*/,
                                              bool* /*defaultB*/,
                                              bool* /*defaultA*/) const
{
    return false;
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;

#include "moc_DiskCacheNode.cpp"

