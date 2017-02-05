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

#ifndef Engine_JoinViewsNode_h
#define Engine_JoinViewsNode_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EffectInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct JoinViewsNodePrivate;
class JoinViewsNode
    : public EffectInstance
{


private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    JoinViewsNode(const NodePtr& node);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new JoinViewsNode(node) );
    }


    static PluginPtr createPlugin();

    virtual ~JoinViewsNode();


    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;


    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual void addAcceptedComponents(int inputNb, std::bitset<4>* comps) OVERRIDE FINAL;

    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;


    virtual bool isViewAware() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;


private:

    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;

    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                            const RenderScale & scale,
                            const RectI & roi,
                            ViewIdx view,
                            const TreeRenderNodeArgsPtr& render,
                            TimeValue* inputTime,
                            ViewIdx* inputView,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
    boost::scoped_ptr<JoinViewsNodePrivate> _imp;
};

inline JoinViewsNodePtr
toJoinViewsNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<JoinViewsNode>(effect);
}

NATRON_NAMESPACE_EXIT;
#endif // Engine_JoinViewsNode_h
