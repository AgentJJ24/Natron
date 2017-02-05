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

#ifndef ROTOSHAPERENDERNODE_H
#define ROTOSHAPERENDERNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

#define kRotoShapeRenderNodeParamOutputComponents "outputComponents"
#define kRotoShapeRenderNodeParamOutputComponentsLabel "Output Components"

#define kRotoShapeRenderNodeParamOutputComponentsAlpha "Alpha"
#define kRotoShapeRenderNodeParamOutputComponentsRGBA "RGBA"

#define kRotoShapeRenderNodeParamType "type"
#define kRotoShapeRenderNodeParamTypeLabel "Type"

#define kRotoShapeRenderNodeParamTypeSolid "Solid"
#define kRotoShapeRenderNodeParamTypeSmear "Smear"


class RotoShapeRenderNodePrivate;
class RotoShapeRenderNode
    : public EffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    RotoShapeRenderNode(NodePtr n);


public:

    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoShapeRenderNode(node) );
    }

    static PluginPtr createPlugin();

    virtual ~RotoShapeRenderNode();

    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new RotoShapeRenderNode(n);
    }

    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }


    virtual std::string getInputLabel (int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return std::string("Source");
    }

    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void addAcceptedComponents(int inputNb, std::bitset<4>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;


    // We cannot support tiles with our algorithm
    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool canCPUImplementationSupportOSMesa() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash)  OVERRIDE FINAL;

private:

    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual void purgeCaches() OVERRIDE FINAL;

    virtual ActionRetCodeEnum attachOpenGLContext(TimeValue time, ViewIdx view, const RenderScale& scale, const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data) OVERRIDE FINAL;

    virtual ActionRetCodeEnum dettachOpenGLContext(const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data) OVERRIDE FINAL;

    virtual ActionRetCodeEnum getRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view, const TreeRenderNodeArgsPtr& render,RectD* rod) OVERRIDE WARN_UNUSED_RETURN;

    virtual ActionRetCodeEnum getTimeInvariantMetaDatas(NodeMetadata& metadata) OVERRIDE FINAL;

    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                            const RenderScale & scale,
                            const RectI & roi,
                            ViewIdx view,
                            const TreeRenderNodeArgsPtr& render,
                            TimeValue* inputTime,
                            ViewIdx* inputView,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    boost::scoped_ptr<RotoShapeRenderNodePrivate> _imp;

};

NATRON_NAMESPACE_EXIT;

#endif // ROTOSHAPERENDERNODE_H
