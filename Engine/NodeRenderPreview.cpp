/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "NodePrivate.h"

#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/TreeRender.h"

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER

/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// output is always RGBA with alpha = 255
template<typename PIX, int maxValue, int srcNComps, bool convertToSrgb>
void
renderPreviewTemplated(const void* srcPtrs[4],
                       const RectI& srcBounds,
                       int *dstWidth,
                       int *dstHeight,
                       unsigned int* dstPixels)
{
    double yZoomFactor = *dstHeight / (double)srcBounds.height();
    double xZoomFactor = *dstWidth / (double)srcBounds.width();

    double zoomFactor;
    if (xZoomFactor < yZoomFactor) {
        zoomFactor = xZoomFactor;
        *dstHeight = srcBounds.height() * zoomFactor;
    } else {
        zoomFactor = yZoomFactor;
        *dstWidth = srcBounds.width() * zoomFactor;
    }




    for (int i = 0; i < *dstHeight; ++i) {

        // just use nearest neighbor
        const double srcY_f = (i - *dstHeight / 2.) / zoomFactor + (srcBounds.y1 + srcBounds.y2) / 2.;
        const int srcY_i = std::floor(srcY_f + 0.5);

        U32 *dst_pixels = dstPixels + *dstWidth * (*dstHeight - 1 - i);

        const PIX* srcPixelPtrs[4];
        int pixelStride;
        Image::getChannelPointers<PIX, srcNComps>((const PIX**)srcPtrs, srcBounds.x1, srcY_i, srcBounds, (PIX**)srcPixelPtrs, &pixelStride);

        if (!srcPixelPtrs[0]) {
            // out of bounds
            for (int j = 0; j < *dstWidth; ++j) {
#ifndef __NATRON_WIN32__
                dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
                dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
            }
        } else {
            for (int j = 0; j < *dstWidth; ++j) {


                const double srcX_f = (j - *dstWidth / 2.) / zoomFactor + (srcBounds.x1 + srcBounds.x2) / 2.;
                const int srcX_i = std::floor(srcX_f + 0.5) - srcBounds.x1;     // round to nearest

                if ( (srcX_i < 0) || ( srcX_i >= (srcBounds.x2 - srcBounds.x1) ) ) {
#ifndef __NATRON_WIN32__
                    dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
                    dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
                } else {

                    float tmpPix[3];
                    for (int c = 0; c < srcNComps; ++c) {
                        if (srcPixelPtrs[c]) {
                            tmpPix[c] = Image::convertPixelDepth<PIX, float>(*srcPixelPtrs[c]);
                            ++srcPixelPtrs[c];
                        } else {
                            tmpPix[c] = 0;
                        }
                    }
                    if (srcNComps == 1) {
                        tmpPix[0] = tmpPix[1] = tmpPix[2];
                    }
                    int r = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(tmpPix[0]) : tmpPix[0]);
                    int g = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(tmpPix[1]) : tmpPix[1]);
                    int b = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(tmpPix[2]) : tmpPix[2]);
                    dst_pixels[j] = toBGRA(r, g, b, 255);
                }
            }
        }
    }
}   // renderPreviewTemplated

template<typename PIX, int maxValue, int srcNComps>
void
renderPreviewForColorSpace(const void* srcPtrs[4],
                           const RectI& srcBounds,
                           int *dstWidth,
                           int *dstHeight,
                           bool convertToSrgb,
                           unsigned int* dstPixels)
{
    if (convertToSrgb) {
        renderPreviewTemplated<PIX, maxValue, srcNComps, true>(srcPtrs, srcBounds, dstWidth, dstHeight, dstPixels);
    } else {
        renderPreviewTemplated<PIX, maxValue, srcNComps, false>(srcPtrs, srcBounds, dstWidth, dstHeight, dstPixels);
    }
}

///output is always RGBA with alpha = 255
template<typename PIX, int maxValue>
void
renderPreviewForDepth(const void* srcPtrs[4],
                      const RectI& srcBounds,
                      int srcNComps,
                      int *dstWidth,
                      int *dstHeight,
                      bool convertToSrgb,
                      unsigned int* dstPixels)
{
    switch (srcNComps) {
        case 0:
            return;
        case 1:
            renderPreviewForColorSpace<PIX, maxValue, 1>(srcPtrs, srcBounds, dstWidth, dstHeight, convertToSrgb, dstPixels);
            break;
        case 2:
            renderPreviewForColorSpace<PIX, maxValue, 2>(srcPtrs, srcBounds, dstWidth, dstHeight, convertToSrgb, dstPixels);
            break;
        case 3:
            renderPreviewForColorSpace<PIX, maxValue, 3>(srcPtrs, srcBounds, dstWidth, dstHeight, convertToSrgb, dstPixels);
            break;
        case 4:
            renderPreviewForColorSpace<PIX, maxValue, 4>(srcPtrs, srcBounds, dstWidth, dstHeight, convertToSrgb, dstPixels);
            break;
        default:
            break;
    }
}   // renderPreviewForDepth

static void renderPreviewInternal(const void* srcPtrs[4],
                                  ImageBitDepthEnum srcBitDepth,
                                  const RectI& srcBounds,
                                  int srcNComps,
                                  int* width,
                                  int *height,
                                  bool convertToSrgb,
                                  unsigned int* buf)
{
    switch ( srcBitDepth ) {
        case eImageBitDepthByte: {
            renderPreviewForDepth<unsigned char, 255>(srcPtrs, srcBounds, srcNComps, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthShort: {
            renderPreviewForDepth<unsigned short, 65535>(srcPtrs, srcBounds, srcNComps, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthHalf:
            break;
        case eImageBitDepthFloat: {
            renderPreviewForDepth<float, 1>(srcPtrs, srcBounds, srcNComps , width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthNone:
            break;
    }

} // renderPreviewInternal

NATRON_NAMESPACE_ANONYMOUS_EXIT




bool
Node::makePreviewImage(TimeValue time,
                       int *width,
                       int *height,
                       unsigned int* buf)
{
    if (!isNodeCreated()) {
        return false;
    }


    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        if (_imp->isBeingDestroyed) {
            return false;
        }
    }

    if ( _imp->checkForExitPreview() ) {
        return false;
    }


    /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    ComputingPreviewSetter_RAII computingPreviewRAII( _imp.get() );

    EffectInstancePtr effect;
    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        NodePtr outputNode = isGroup->getOutputNodeInput();
        if (outputNode) {
            return outputNode->makePreviewImage(time, width, height, buf);
        }
        return false;
    } else {
        effect = _imp->effect;
    }

    if (!effect) {
        return false;
    }


    RectD rod;

    {
        RenderScale scale(1.);

        GetRegionOfDefinitionResultsPtr actionResults;
        ActionRetCodeEnum stat = effect->getRegionOfDefinition_public(time, scale, ViewIdx(0), TreeRenderNodeArgsPtr(), &actionResults);
        if (isFailureRetCode(stat)) {
            return false;
        }
        rod = actionResults->getRoD();
    }
    if (rod.isNull()) {
        return false;
    }

    // Compute the mipmap level to pass to render
    double yZoomFactor = (double)*height / (double)rod.height();
    double xZoomFactor = (double)*width / (double)rod.width();
    double closestPowerOf2X = xZoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil( std::log(xZoomFactor) / std::log(2.) ) );
    double closestPowerOf2Y = yZoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil( std::log(yZoomFactor) / std::log(2.) ) );
    int closestPowerOf2 = std::max(closestPowerOf2X, closestPowerOf2Y);

    unsigned int mipMapLevel = std::min(std::log( (double)closestPowerOf2 ) / std::log(2.), 5.);

    TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
    {
        args->treeRoot = shared_from_this();
        args->time = time;
        args->view = ViewIdx(0);

        // Render all layers produced
        args->layers = 0;
        args->mipMapLevel = mipMapLevel;

        args->proxyScale = RenderScale(1.);

        // Render the RoD
        args->canonicalRoI = 0;
        args->draftMode = false;
        args->playback = false;
        args->byPassCache = false;
    }

    std::map<ImagePlaneDesc, ImagePtr> planes;
    TreeRenderPtr render = TreeRender::create(args);
    ActionRetCodeEnum stat = render->launchRender(&planes);
    if (isFailureRetCode(stat)) {
        return false;
    }
    assert(!planes.empty());

    const ImagePtr& img = planes.begin()->second;

    // we convert only when input is Linear.
    // Rec709 and sRGB is acceptable for preview
    bool convertToSrgb = getApp()->getDefaultColorSpaceForBitDepth( img->getBitDepth() ) == eViewerColorSpaceLinear;

    // Ensure we have an untiled format
    ImagePtr imageForPreview = img;
    if (imageForPreview->getBufferFormat() == eImageBufferLayoutMonoChannelTiled || imageForPreview->getStorageMode() == eStorageModeGLTex){

        try {
            {
                Image::InitStorageArgs initArgs;
                initArgs.bounds = imageForPreview->getBounds();
                initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
                initArgs.storage = eStorageModeRAM;
                initArgs.layer = imageForPreview->getLayer();
                initArgs.bitdepth = imageForPreview->getBitDepth();
                imageForPreview = Image::create(initArgs);
            }
            {
                Image::CopyPixelsArgs cpyArgs;
                cpyArgs.roi = imageForPreview->getBounds();
                imageForPreview->copyPixels(*img, cpyArgs);
            }
        } catch (...) {
            return false;
        }
    }
    Image::Tile mainTile;
    imageForPreview->getTileAt(0, &mainTile);

    Image::CPUTileData tileData;
    imageForPreview->getCPUTileData(mainTile, &tileData);

    renderPreviewInternal((const void**)tileData.ptrs, tileData.bitDepth, tileData.tileBounds, tileData.nComps, width, height, convertToSrgb, buf);
    
    return true;
} // makePreviewImage

void
Node::refreshPreviewsAfterProjectLoad()
{
    computePreviewImage(TimeValue(getApp()->getTimeLine()->currentFrame()));
    Q_EMIT s_refreshPreviewsAfterProjectLoadRequested();
}


bool
Node::isRenderingPreview() const
{
    QMutexLocker l(&_imp->computingPreviewMutex);

    return _imp->computingPreview;
}



static void
refreshPreviewsRecursivelyUpstreamInternal(TimeValue time,
                                           const NodePtr& node,
                                           std::list<NodePtr>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }

    marked.push_back(node);

    std::vector<NodeWPtr > inputs = node->getInputs_copy();

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        NodePtr input = inputs[i].lock();
        if (input) {
            input->refreshPreviewsRecursivelyUpstream(time);
        }
    }
}

void
Node::refreshPreviewsRecursivelyUpstream(TimeValue time)
{
    std::list<NodePtr> marked;

    refreshPreviewsRecursivelyUpstreamInternal(time, shared_from_this(), marked);
}

static void
refreshPreviewsRecursivelyDownstreamInternal(TimeValue time,
                                             const NodePtr& node,
                                             std::list<NodePtr>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }

    marked.push_back(node);

    NodesList outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->refreshPreviewsRecursivelyDownstream(time);
    }
}

void
Node::refreshPreviewsRecursivelyDownstream(TimeValue time)
{
    if ( !getNodeGui() ) {
        return;
    }
    std::list<NodePtr> marked;
    refreshPreviewsRecursivelyDownstreamInternal(time, shared_from_this(), marked);
}



bool
Node::makePreviewByDefault() const
{
    ///MT-safe, never changes
    assert(_imp->effect);

    return _imp->effect->makePreviewByDefault();
}

void
Node::togglePreview()
{
    ///MT-safe from Knob
    KnobBoolPtr b = _imp->previewEnabledKnob.lock();
    if (!b) {
        return;
    }
    b->setValue( !b->getValue() );
}

bool
Node::isPreviewEnabled() const
{
    ///MT-safe from EffectInstance
    KnobBoolPtr b = _imp->previewEnabledKnob.lock();
    if (!b) {
        return false;
    }

    return b->getValue();
}
NATRON_NAMESPACE_EXIT;
