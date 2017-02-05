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

#ifndef TRACKERNODE_H
#define TRACKERNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/NodeGroup.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

#define kTrackerUIParamDefaultMarkerPatternWinSize "defPatternWinSize"
#define kTrackerUIParamDefaultMarkerPatternWinSizeLabel "Default Pattern Size"
#define kTrackerUIParamDefaultMarkerPatternWinSizeHint "The size in pixels of the pattern that created markers will have by default"

#define kTrackerUIParamDefaultMarkerSearchWinSize "defSearchWinSize"
#define kTrackerUIParamDefaultMarkerSearchWinSizeLabel "Default Search Area Size"
#define kTrackerUIParamDefaultMarkerSearchWinSizeHint "The size in pixels of the search window that created markers will have by default"

#define kTrackerUIParamDefaultMotionModel "defMotionModel"
#define kTrackerUIParamDefaultMotionModelLabel "Default Motion Model"
#define kTrackerUIParamDefaultMotionModelHint "The motion model that new tracks have by default"

class TrackerNodePrivate;
class TrackerNode
    : public NodeGroup
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    TrackerNode(const NodePtr& node);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new TrackerNode(node) );
    }


    static PluginPtr createPlugin();

    virtual ~TrackerNode();


    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;

    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual bool isHostMaskingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool isHostMixingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return false; }


    virtual bool isHostChannelSelectorSupported(bool* /*defaultR*/,
                                                bool* /*defaultG*/,
                                                bool* /*defaultB*/,
                                                bool* /*defaultA*/) const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool hasOverlay() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool isSubGraphUserVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isSubGraphPersistent() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void onKnobsLoaded() OVERRIDE FINAL;

    virtual void setupInitialSubGraphState() OVERRIDE FINAL;

    TrackerHelperPtr getTracker() const;

public Q_SLOTS:


    void onCornerPinSolverWatcherFinished();
    void onTransformSolverWatcherFinished();

    void onCornerPinSolverWatcherProgress(int progress);
    void onTransformSolverWatcherProgress(int progress);

private:

    void initializeViewerUIKnobs(const KnobPagePtr& trackingPage);

    void initializeTrackRangeDialogKnobs(const KnobPagePtr& trackingPage);

    void initializeRightClickMenuKnobs(const KnobPagePtr& trackingPage);

    void initializeTrackingPageKnobs(const KnobPagePtr& trackingPage);

    void initializeTransformPageKnobs(const KnobPagePtr& transformPage);

    virtual void drawOverlay(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(TimeValue time, const RenderScale & renderScale, ViewIdx view,
                                    const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenDoubleClicked(TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyRepeat(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayFocusGained(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayFocusLost(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual void onInteractViewportSelectionCleared() OVERRIDE FINAL;
    virtual void onInteractViewportSelectionUpdated(const RectD& rectangle, bool onRelease) OVERRIDE FINAL;
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;
    virtual void refreshExtraStateAfterTimeChanged(bool isPlayback, TimeValue time)  OVERRIDE FINAL;
    virtual void evaluate(bool isSignificant, bool refreshMetadatas) OVERRIDE FINAL;

private:

    boost::shared_ptr<TrackerNodePrivate> _imp;
};

inline TrackerNodePtr
toTrackerNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<TrackerNode>(effect);
}

NATRON_NAMESPACE_EXIT;

#endif // TRACKERNODE_H
