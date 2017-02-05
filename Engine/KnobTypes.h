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

#ifndef NATRON_ENGINE_KNOBTYPES_H
#define NATRON_ENGINE_KNOBTYPES_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#include <map>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QVector>
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "Global/GlobalDefines.h"
#include "Engine/ChoiceOption.h"
#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"
#include "Engine/ChoiceOption.h"

NATRON_NAMESPACE_ENTER;

#define kFontSizeTag "<font size=\""
#define kFontColorTag "color=\""
#define kFontFaceTag "face=\""
#define kFontEndTag "</font>"
#define kBoldStartTag "<b>"
#define kBoldEndTag "</b>"
#define kItalicStartTag "<i>"
#define kItalicEndTag "</i>"

inline KnobBoolBasePtr
toKnobBoolBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobBoolBase>(knob);
}

inline KnobDoubleBasePtr
toKnobDoubleBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobDoubleBase>(knob);
}

inline KnobIntBasePtr
toKnobIntBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobIntBase>(knob);
}

inline KnobStringBasePtr
toKnobStringBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobStringBase>(knob);
}

/******************************KnobInt**************************************/

class KnobInt
    : public QObject, public KnobIntBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobInt(const KnobHolderPtr& holder,
            const std::string &label,
            int dimension,
            bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                             const std::string &label,
                             int dimension,
                             bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobInt(holder, label, dimension, declaredByPlugin));
    }

    static KnobIntPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobIntPtr(new KnobInt(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }

    void disableSlider();


    bool isSliderDisabled() const;

    static const std::string & typeNameStatic();

    void setAsRectangle()
    {
        if (getNDimensions() == 4) {
            _isRectangle = true;
            disableSlider();
        }
    }

    bool isRectangle() const
    {
        return _isRectangle;
    }

    void setValueCenteredInSpinBox(bool enabled) { _isValueCenteredInSpinbox = enabled; }

    bool isValueCenteredInSpinBox() const { return _isValueCenteredInSpinbox; }

    // For 2D int parameters, the UI will have a keybind recorder
    // and the first dimension stores the symbol and the 2nd the modifiers
    void setAsShortcutKnob(bool isShortcutKnob) {
        _isShortcutKnob = isShortcutKnob;
    }

    bool isShortcutKnob() const
    {
        return _isShortcutKnob;
    }
public:

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void setIncrement(int incr, DimIdx index = DimIdx(0));

    void setIncrement(const std::vector<int> &incr);

    const std::vector<int> &getIncrements() const;


Q_SIGNALS:


    void incrementChanged(double incr, DimIdx index);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    std::vector<int> _increments;
    bool _disableSlider;
    bool _isRectangle;
    bool _isValueCenteredInSpinbox;
    bool _isShortcutKnob;
    static const std::string _typeNameStr;
};

inline KnobIntPtr
toKnobInt(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobInt>(knob);
}

/******************************KnobBool**************************************/

class KnobBool
    :  public KnobBoolBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobBool(const KnobHolderPtr& holder,
             const std::string &label,
             int dimension,
             bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobBool(holder, label, dimension, declaredByPlugin));
    }
    
    static KnobBoolPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobBoolPtr(new KnobBool(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    /// Can this type be animated?
    /// BooleanParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();
    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }


private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

inline KnobBoolPtr
toKnobBool(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobBool>(knob);
}

/******************************KnobDouble**************************************/

class KnobDouble
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobDouble(const KnobHolderPtr& holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin );

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobDouble(holder, label, dimension, declaredByPlugin));
    }

    static KnobDoublePtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobDoublePtr(new KnobDouble(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual ~KnobDouble();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void disableSlider();

    bool isSliderDisabled() const;

    const std::vector<double> &getIncrements() const;
    const std::vector<int> &getDecimals() const;

    void setIncrement(double incr, DimIdx index = DimIdx(0));

    void setDecimals(int decis, DimIdx index = DimIdx(0));

    void setIncrement(const std::vector<double> &incr);

    void setDecimals(const std::vector<int> &decis);

    static const std::string & typeNameStatic();

    ValueIsNormalizedEnum getValueIsNormalized(DimIdx dimension) const
    {
        assert(dimension >= 0 && dimension < (int)_valueIsNormalized.size());
        return _valueIsNormalized[dimension];
    }

    void setValueIsNormalized(DimIdx dimension,
                              ValueIsNormalizedEnum state);
    void setSpatial(bool spatial);

    bool getIsSpatial() const
    {
        return _spatial;
    }

    /**
     * @brief Normalize the default values, set the _defaultValuesAreNormalized to true and
     * calls setDefaultValue with the good parameters.
     * Later when restoring the default values, this flag will be used to know whether we need
     * to denormalize the default stored values to the set the "live" values.
     * Hence this SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters otherwise
     * they would be denormalized before being passed to the plug-in.
     *
     * If *all* the following conditions hold:
     * - this is a double value
     * - this is a non normalised spatial double parameter, i.e. kOfxParamPropDoubleType is set to one of
     *   - kOfxParamDoubleTypeX
     *   - kOfxParamDoubleTypeXAbsolute
     *   - kOfxParamDoubleTypeY
     *   - kOfxParamDoubleTypeYAbsolute
     *   - kOfxParamDoubleTypeXY
     *   - kOfxParamDoubleTypeXYAbsolute
     * - kOfxParamPropDefaultCoordinateSystem is set to kOfxParamCoordinatesNormalised
     * Knob<T>::resetToDefaultValue should denormalize
     * the default value, using the input size.
     * Input size be defined as the first available of:
     * - the RoD of the "Source" clip
     * - the RoD of the first non-mask non-optional input clip (in case there is no "Source" clip) (note that if these clips are not connected, you get the current project window, which is the default value for GetRegionOfDefinition)

     * see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
     * and http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#APIChanges_1_2_SpatialParameters
     **/
    void setDefaultValuesAreNormalized(bool normalized);

    /**
     * @brief Returns whether the default values are stored normalized or not.
     **/
    bool getDefaultValuesAreNormalized() const
    {
        return _defaultValuesAreNormalized;
    }

    /**
     * @brief Denormalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    double denormalize(DimIdx dimension, TimeValue time, double value) const;

    /**
     * @brief Normalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    double normalize(DimIdx dimension, TimeValue time, double value) const;

    void setHasHostOverlayHandle(bool handle);

    bool getHasHostOverlayHandle() const;

    virtual bool useHostOverlayHandle() const OVERRIDE { return getHasHostOverlayHandle(); }

    void setAsRectangle()
    {
        if (getNDimensions() == 4) {
            _isRectangle = true;
        }
    }

    bool isRectangle() const
    {
        return _isRectangle;
    }

Q_SIGNALS:

    void incrementChanged(double incr, DimIdx index);

    void decimalsChanged(int deci, DimIdx index);

private:

    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const OVERRIDE FINAL;

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    bool _spatial;
    bool _isRectangle;
    std::vector<double>  _increments;
    std::vector<int> _decimals;
    bool _disableSlider;

    /// to support ofx deprecated normalizd params:
    /// the first and second dimensions of the double param( hence a pair ) have a normalized state.
    /// BY default they have eValueIsNormalizedNone
    /// if the double type is one of
    /// - kOfxParamDoubleTypeNormalisedX - normalised size wrt to the project's X dimension (1D only),
    /// - kOfxParamDoubleTypeNormalisedXAbsolute - normalised absolute position on the X axis (1D only)
    /// - kOfxParamDoubleTypeNormalisedY - normalised size wrt to the project's Y dimension(1D only),
    /// - kOfxParamDoubleTypeNormalisedYAbsolute - normalised absolute position on the Y axis (1D only)
    /// - kOfxParamDoubleTypeNormalisedXY - normalised to the project's X and Y size (2D only),
    /// - kOfxParamDoubleTypeNormalisedXYAbsolute - normalised to the projects X and Y size, and is an absolute position on the image plane,
    std::vector<ValueIsNormalizedEnum> _valueIsNormalized;

    ///For double params respecting the kOfxParamCoordinatesNormalised
    ///This tells us that only the default value is stored normalized.
    ///This SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters.
    bool _defaultValuesAreNormalized;
    bool _hasHostOverlayHandle;
    static const std::string _typeNameStr;
};

inline KnobDoublePtr
toKnobDouble(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobDouble>(knob);
}

/******************************KnobButton**************************************/

class KnobButton
    : public KnobBoolBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobButton(const KnobHolderPtr& holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobButton(holder, label, dimension, declaredByPlugin));
    }

    static KnobButtonPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobButtonPtr(new KnobButton(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    static const std::string & typeNameStatic();

    void setAsRenderButton()
    {
        _renderButton = true;
    }

    bool isRenderButton() const
    {
        return _renderButton;
    }

    // Trigger the knobChanged handler for this knob
    // Returns true if the knobChanged handler was caught and an action was done
    bool trigger();

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void setCheckable(bool b)
    {
        _checkable = b;
    }

    bool getIsCheckable() const
    {
        return _checkable;
    }

    void setAsToolButtonAction(bool b)
    {
        _isToolButtonAction = b;
    }

    bool getIsToolButtonAction() const
    {
        return _isToolButtonAction;
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _renderButton;
    bool _checkable;
    bool _isToolButtonAction;
};

inline KnobButtonPtr
toKnobButton(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobButton>(knob);
}

/******************************KnobChoice**************************************/



class ChoiceKnobDimView : public ValueKnobDimView<int>
{
public:

    // For a choice parameter we need to know the strings
    std::vector<ChoiceOption> menuOptions;

    // For choice parameters the value is held by a string because if the option disappears from the menu
    // we still need to remember the user choice
    ChoiceOption activeEntry;

    //  Each item in the list will add a separator after the index specified by the integer.
    std::vector<int> separators;

    // Optional shortcuts visible for menu entries. All items in the menu don't need a shortcut
    // so they are mapped against their index. The string corresponds to a shortcut ID that was registered
    // on the node during the getPluginShortcuts function on the same node.
    std::map<int, std::string> shortcuts;

    // Optional icons for menu entries. All items in the menu don't need an icon
    // so they are mapped against their index.
    std::map<int, std::string> menuIcons;

    // A pointer to a callback called when the "new" item is invoked for a knob.
    // If not set the menu will not have the "new" item in the menu.
    typedef void (*KnobChoiceNewItemCallback)(const KnobChoicePtr& knob);
    KnobChoiceNewItemCallback addNewChoiceCallback;

    // When not empty, the size of the combobox will be fixed so that the content of this string can be displayed entirely.
    // This is so that the combobox can have a fixed custom width.
    std::string textToFitHorizontally;

    // When true the menu is considered cascading
    bool isCascading;

    // For choice menus that may change the entry that was selected by the user may disappear.
    // In this case, if this flag is true a warning will be displayed next to the menu.
    bool showMissingEntryWarning;

    // When the entry corresponding to the index is selected, the combobox frame will get the associated color.
    std::map<int, RGBAColourD> menuColors;

    ChoiceKnobDimView();

    virtual ~ChoiceKnobDimView()
    {
        
    }

    virtual bool setValueAndCheckIfChanged(const int& value) OVERRIDE;

    virtual bool copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs) OVERRIDE;
};


typedef boost::shared_ptr<ChoiceKnobDimView> ChoiceKnobDimViewPtr;

inline ChoiceKnobDimViewPtr
toChoiceKnobDimView(const KnobDimViewBasePtr& data)
{
    return boost::dynamic_pointer_cast<ChoiceKnobDimView>(data);
}


class KnobChoice
    : public QObject, public KnobIntBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON


private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobChoice(const KnobHolderPtr& holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

public:

    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobChoice(holder, label, dimension, declaredByPlugin));
    }

    static KnobChoicePtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobChoicePtr(new KnobChoice(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual ~KnobChoice();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    /**
     * @brief Fills-up the menu with the given entries and optionnally their tooltip.
     * @param entriesHelp Can either be empty, meaning no-tooltip or must be of the size of the entries.
     * @param mergingFunctor If not set, the internal menu will be completely reset and replaced with the given entries.
     * Otherwise the internal menu entries will be merged with the given entries according to this equality functor.
     * @param mergingData Can be passed when mergingFunctor is not null to speed up the comparisons.
     *
     * @returns true if something changed, false otherwise.
     **/

    bool populateChoices(const std::vector<ChoiceOption>& entries);

    /**
     * @brief Set optional shortcuts visible for menu entries. All items in the menu don't need a shortcut
     * so they are mapped against their index. The string corresponds to a shortcut ID that was registered
     * on the node during the getPluginShortcuts function on the same node.
     **/
    void setShortcuts(const std::map<int, std::string>& shortcuts);
    std::map<int, std::string> getShortcuts() const;

    /**
     * @brief Set optional icons for menu entries. All items in the menu don't need an icon
     * so they are mapped against their index.
     **/
    void setIcons(const std::map<int, std::string>& icons);
    std::map<int, std::string> getIcons() const;


    /**
     * @brief Set a list of separators. Each item in the list will add a separator after the index
     * specified by the integer.
     **/
    void setSeparators(const std::vector<int>& separators);
    std::vector<int> getSeparators() const;

    /**
     * @brief Clears the menu, the current index will no longer correspond to a valid entry
     **/
    void resetChoices(ViewSetSpec view = ViewSetSpec::all());


    /**
     * @brief Append an option to the menu 
     * @param help Optionnally specify the tooltip that should be displayed when the user hovers the entry in the menu
     **/
    void appendChoice( const ChoiceOption& option, ViewSetSpec view = ViewSetSpec::all() );

    /**
     * @brief Returns true if the entry for the given view is valid, that is: it still belongs to the menu entries.
     **/
    bool isActiveEntryPresentInEntries(ViewIdx view) const;

    /**
     * @brief Get all menu entries
     **/
    std::vector<ChoiceOption> getEntries(ViewIdx view = ViewIdx(0)) const;

    /**
     * @brief Get one menu entry. Throws an invalid_argument exception if index is invalid
     **/
    ChoiceOption getEntry(int v, ViewIdx view = ViewIdx(0)) const;

    /**
     * @brief Get the active entry text
     **/
    ChoiceOption getActiveEntry(ViewIdx view = ViewIdx(0));

    /**
     * @brief Set the active entry text. If the view does not exist in the knob an invalid
     * argument exception is thrown
     **/
    void setActiveEntry(const ChoiceOption& entry, ViewSetSpec view = ViewSetSpec::all());

    int getNumEntries(ViewIdx view = ViewIdx(0)) const;

    /// Can this type be animated?
    /// ChoiceParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();
    std::string getHintToolTipFull() const;

    static int choiceMatch(const std::string& choiceID, const std::vector<ChoiceOption>& entries, ChoiceOption* matchedEntry);

    /**
     * @brief When set the menu will have a "New" entry which the user can select to create a new entry on its own.
     **/
    void setNewOptionCallback(ChoiceKnobDimView::KnobChoiceNewItemCallback callback);

    ChoiceKnobDimView::KnobChoiceNewItemCallback getNewOptionCallback() const;

    void setCascading(bool cascading);

    bool isCascading() const;

    /// set the KnobChoice value from the label
    ValueChangedReturnCodeEnum setValueFromID(const std::string & value, ViewSetSpec view = ViewSetSpec::all());

    /// set the KnobChoice default value from the label
    void setDefaultValueFromID(const std::string & value);
    void setDefaultValueFromIDWithoutApplying(const std::string & value);

    void setMissingEntryWarningEnabled(bool enabled);
    bool isMissingEntryWarningEnabled() const;

    void setColorForIndex(int index, const RGBAColourD& color);
    bool getColorForIndex(int index, RGBAColourD* color) const;


    void setTextToFitHorizontally(const std::string& text);
    std::string getTextToFitHorizontally() const;

    virtual bool canLinkWith(const KnobIPtr & other, DimIdx thisDimension, ViewIdx thisView, DimIdx otherDim, ViewIdx otherView, std::string* error) const OVERRIDE WARN_UNUSED_RETURN;

    virtual void onLinkChanged() OVERRIDE FINAL;

Q_SIGNALS:

    void populated();
    void entriesReset();
    void entryAppended();

private:
    

    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const OVERRIDE FINAL;


    void findAndSetOldChoice();

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

    virtual KnobDimViewBasePtr createDimViewData() const OVERRIDE;


private:

    static const std::string _typeNameStr;


};

inline KnobChoicePtr
toKnobChoice(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobChoice>(knob);
}

/******************************KnobSeparator**************************************/

class KnobSeparator
    : public KnobBoolBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobSeparator(const KnobHolderPtr& holder,
                  const std::string &label,
                  int dimension,
                  bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobSeparator(holder, label, dimension, declaredByPlugin));
    }

    static KnobSeparatorPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobSeparatorPtr(new KnobSeparator(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    static const std::string & typeNameStatic();
    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

inline KnobSeparatorPtr
toKnobSeparator(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobSeparator>(knob);
}

/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
 **/
class KnobColor
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobColor(const KnobHolderPtr& holder,
              const std::string &label,
              int dimension,
              bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobColor(holder, label, dimension, declaredByPlugin));
    }

    static KnobColorPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobColorPtr(new KnobColor(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    static const std::string & typeNameStatic();

    void setPickingEnabled(ViewSetSpec view, bool enabled)
    {
        Q_EMIT pickingEnabled(view, enabled);
    }

    /**
     * @brief When simplified, the GUI of the knob should not have any spinbox and sliders but just a label to click and openup a color dialog
     **/
    void setSimplified(bool simp);
    bool isSimplified() const;

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }


Q_SIGNALS:

    void pickingEnabled(ViewSetSpec,bool);

    void minMaxChanged(double mini, double maxi, int index = 0);

    void displayMinMaxChanged(double mini, double maxi, int index = 0);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    bool _simplifiedMode;
    static const std::string _typeNameStr;
};

inline KnobColorPtr
toKnobColor(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobColor>(knob);
}

/******************************KnobString**************************************/


class KnobString
    : public AnimatingKnobStringHelper
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobString(const KnobHolderPtr& holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobString(holder, label, dimension, declaredByPlugin));
    }

    static KnobStringPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobStringPtr(new KnobString(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual ~KnobString();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();

    void setAsMultiLine()
    {
        _multiLine = true;
    }

    void setUsesRichText(bool useRichText)
    {
        _richText = useRichText;
    }

    bool isMultiLine() const
    {
        return _multiLine;
    }

    bool usesRichText() const
    {
        return _richText;
    }

    void setAsCustomHTMLText(bool custom)
    {
        _customHtmlText = custom;
    }

    bool isCustomHTMLText() const
    {
        return _customHtmlText;
    }

    void setAsLabel();

    bool isLabel() const
    {
        return _isLabel;
    }

    void setAsCustom()
    {
        _isCustom = true;
    }

    bool isCustomKnob() const
    {
        return _isCustom;
    }

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return !_multiLine;
    }

    int getFontSize() const
    {
        return _fontSize;
    }

    void setFontSize(int size)
    {
        _fontSize = size;
    }

    std::string getFontFamily() const
    {
        return _fontFamily;
    }

    void setFontFamily(const std::string& family) {
        _fontFamily = family;
    }

    void getFontColor(double* r, double* g, double* b) const
    {
        *r = _fontColor[0];
        *g = _fontColor[1];
        *b = _fontColor[2];
    }

    void setFontColor(double r, double g, double b)
    {
        _fontColor[0] = r;
        _fontColor[1] = g;
        _fontColor[2] = b;
    }

    bool getItalicActivated() const
    {
        return _italicActivated;
    }

    void setItalicActivated(bool b) {
        _italicActivated = b;
    }

    bool getBoldActivated() const
    {
        return _boldActivated;
    }

    void setBoldActivated(bool b) {
        _boldActivated = b;
    }

    /**
     * @brief Relevant for multi-lines with rich text enables. It tells if
     * the string has content without the html tags
     **/
    bool hasContentWithoutHtmlTags();

    static int getDefaultFontPointSize();

    static bool parseFont(const QString & s, int *fontSize, QString* fontFamily, bool* isBold, bool* isItalic, double* r, double *g, double* b);

    /**
     * @brief Make a html friendly font tag from font properties
     **/
    static QString makeFontTag(const QString& family, int fontSize, double r, double g, double b);

    /**
     * @brief Surround the given text by the given font tag
     **/
    static QString decorateTextWithFontTag(const QString& family, int fontSize, double r, double g, double b, bool isBold, bool isItalic, const QString& text);

    /**
     * @brief Remove any custom Natron html tag content from the given text and returned a stripped version of it.
     **/
    static QString removeNatronHtmlTag(QString text);

    /**
     * @brief Get the content in between custom Natron html tags if any
     **/
    static QString getNatronHtmlTagContent(QString text);

    /**
     * @brief The goal here is to remove all the tags added automatically by Natron (like font color,size family etc...)
     * so the user does not see them in the user interface. Those tags are  present in the internal value held by the knob.
     **/
    static QString removeAutoAddedHtmlTags(QString text, bool removeNatronTag = true);

    QString decorateStringWithCurrentState(const QString& str);

    /**
     * @brief Same as getValue() but decorates the string with the current font state. Only useful if rich text has been enabled
     **/
    QString getValueDecorated(TimeValue time, ViewIdx view);

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _multiLine;
    bool _richText;
    bool _customHtmlText;
    bool _isLabel;
    bool _isCustom;
    int _fontSize;
    bool _boldActivated;
    bool _italicActivated;
    std::string _fontFamily;
    double _fontColor[3];
};

inline KnobStringPtr
toKnobString(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobString>(knob);
}

/******************************KnobGroup**************************************/
class KnobGroup
    :  public QObject, public KnobBoolBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    std::vector< KnobIWPtr > _children;
    bool _isTab;
    bool _isToolButton;
    bool _isDialog;

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobGroup(const KnobHolderPtr& holder,
              const std::string &label,
              int dimension,
              bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobGroup(holder, label, dimension, declaredByPlugin));
    }

    static KnobGroupPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobGroupPtr(new KnobGroup(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    void addKnob(const KnobIPtr& k);
    void removeKnob(const KnobIPtr& k);

    bool moveOneStepUp(const KnobIPtr& k);
    bool moveOneStepDown(const KnobIPtr& k);

    void insertKnob(int index, const KnobIPtr& k);

    std::vector< KnobIPtr > getChildren() const;

    void setAsTab();

    bool isTab() const;

    void setAsToolButton(bool b);
    bool getIsToolButton() const;

    void setAsDialog(bool b);
    bool getIsDialog() const;

    static const std::string & typeNameStatic();

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    static const std::string _typeNameStr;
};

inline KnobGroupPtr
toKnobGroup(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobGroup>(knob);
}

/******************************PAGE_KNOB**************************************/

class KnobPage
    :  public QObject, public KnobBoolBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobPage(const KnobHolderPtr& holder,
             const std::string &label,
             int dimension,
             bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobPage(holder, label, dimension, declaredByPlugin));
    }

    static KnobPagePtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobPagePtr(new KnobPage(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    void addKnob(const KnobIPtr& k);

    void setAsToolBar(bool b)
    {
        _isToolBar = b;
    }

    bool getIsToolBar() const
    {
        return _isToolBar;
    }

    bool moveOneStepUp(const KnobIPtr& k);
    bool moveOneStepDown(const KnobIPtr& k);

    void removeKnob(const KnobIPtr& k);

    void insertKnob(int index, const KnobIPtr& k);

    std::vector< KnobIPtr >  getChildren() const;
    static const std::string & typeNameStatic();

private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    bool _isToolBar;
    std::vector< KnobIWPtr > _children;
    static const std::string _typeNameStr;
};

inline KnobPagePtr
toKnobPage(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobPage>(knob);
}

/******************************KnobParametric**************************************/


class ParametricKnobDimView : public ValueKnobDimView<double>
{
public:

    CurvePtr parametricCurve;

    ParametricKnobDimView()
    : parametricCurve()
    {

    }

    virtual ~ParametricKnobDimView()
    {
        
    }

    virtual bool copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs) OVERRIDE;
};

typedef boost::shared_ptr<ParametricKnobDimView> ParametricKnobDimViewPtr;

inline ParametricKnobDimViewPtr
toParametricKnobDimView(const KnobDimViewBasePtr& data)
{
    return boost::dynamic_pointer_cast<ParametricKnobDimView>(data);
}

class KnobParametric
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    mutable QMutex _curvesMutex;
    std::vector< CurvePtr >  _defaultCurves;
    std::vector<RGBAColourD> _curvesColor;

private: // derives from KnobI
    KnobParametric(const KnobHolderPtr& holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin );


    virtual void populate() OVERRIDE FINAL;

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int nDims,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobParametric(holder, label, nDims, declaredByPlugin));
    }

    static KnobParametricPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int nDims,
                                bool declaredByPlugin = true)
    {
        return KnobParametricPtr(new KnobParametric(holder, label.toStdString(), nDims, declaredByPlugin));
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    // Don't allow other knobs to slave to this one
    virtual bool canLinkWith(const KnobIPtr & /*other*/, DimIdx /*thisDimension*/, ViewIdx /*thisView*/,  DimIdx /*otherDim*/, ViewIdx /*otherView*/, std::string* /*error*/) const OVERRIDE FINAL;

    virtual void onLinkChanged() OVERRIDE FINAL;

    void setCurveColor(DimIdx dimension, double r, double g, double b);

    void setPeriodic(bool periodic);

    void getCurveColor(DimIdx dimension, double* r, double* g, double* b);

    void setParametricRange(double min, double max);

    void setDefaultCurvesFromCurves();

    std::pair<double, double> getParametricRange() const WARN_UNUSED_RETURN;
    CurvePtr getParametricCurve(DimIdx dimension, ViewIdx view) const;
    CurvePtr getDefaultParametricCurve(DimIdx dimension) const;
    ActionRetCodeEnum addControlPoint(ValueChangedReasonEnum reason, DimIdx dimension, double key, double value, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    ActionRetCodeEnum addControlPoint(ValueChangedReasonEnum reason, DimIdx dimension, double key, double value, double leftDerivative, double rightDerivative, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    ActionRetCodeEnum evaluateCurve(DimIdx dimension, ViewIdx view, double parametricPosition, double *returnValue) const WARN_UNUSED_RETURN;
    ActionRetCodeEnum getNControlPoints(DimIdx dimension, ViewIdx view, int *returnValue) const WARN_UNUSED_RETURN;
    ActionRetCodeEnum getNthControlPoint(DimIdx dimension,
                                  ViewIdx view,
                                  int nthCtl,
                                  double *key,
                                  double *value) const WARN_UNUSED_RETURN;
    ActionRetCodeEnum getNthControlPoint(DimIdx dimension,
                                  ViewIdx view,
                                  int nthCtl,
                                  double *key,
                                  double *value,
                                  double *leftDerivative,
                                  double *rightDerivative) const WARN_UNUSED_RETURN;

    ActionRetCodeEnum setNthControlPointInterpolation(ValueChangedReasonEnum reason,
                                               DimIdx dimension,
                                               ViewSetSpec view,
                                               int nThCtl,
                                               KeyframeTypeEnum interpolation) WARN_UNUSED_RETURN;

    ActionRetCodeEnum setNthControlPoint(ValueChangedReasonEnum reason,
                                  DimIdx dimension,
                                  ViewSetSpec view,
                                  int nthCtl,
                                  double key,
                                  double value) WARN_UNUSED_RETURN;

    ActionRetCodeEnum setNthControlPoint(ValueChangedReasonEnum reason,
                                  DimIdx dimension,
                                  ViewSetSpec view,
                                  int nthCtl,
                                  double key,
                                  double value,
                                  double leftDerivative,
                                  double rightDerivative) WARN_UNUSED_RETURN;


    ActionRetCodeEnum deleteControlPoint(ValueChangedReasonEnum reason, DimIdx dimension, ViewSetSpec view, int nthCtl) WARN_UNUSED_RETURN;
    ActionRetCodeEnum deleteAllControlPoints(ValueChangedReasonEnum reason, DimIdx dimension, ViewSetSpec view) WARN_UNUSED_RETURN;
    static const std::string & typeNameStatic() WARN_UNUSED_RETURN;

    void saveParametricCurves(std::map<std::string,std::list< SERIALIZATION_NAMESPACE::CurveSerialization > >* curves) const;

    void loadParametricCurves(const std::map<std::string,std::list< SERIALIZATION_NAMESPACE::CurveSerialization > >& curves);

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE FINAL;


    //////////// Overriden from AnimatingObjectI
    virtual CurvePtr getAnimationCurve(ViewIdx idx, DimIdx dimension) const OVERRIDE FINAL;
    virtual bool cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* stringAnimation) OVERRIDE;
    virtual void deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) OVERRIDE;
    virtual bool warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes = 0) OVERRIDE ;
    virtual void removeAnimation(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) OVERRIDE ;
    virtual void deleteAnimationBeforeTime(TimeValue time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationAfterTime(TimeValue time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) OVERRIDE ;
    virtual bool setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double left, double right)  OVERRIDE WARN_UNUSED_RETURN;
    virtual bool setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double derivative, bool isLeft) OVERRIDE WARN_UNUSED_RETURN;

    virtual ValueChangedReturnCodeEnum setDoubleValueAtTime(TimeValue time, double value, ViewSetSpec view = ViewSetSpec::all(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonUserEdited, KeyFrame* newKey = 0) OVERRIDE ;
    virtual void setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSetSpec view = ViewSetSpec::all(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonUserEdited, std::vector<KeyFrame>* newKey = 0) OVERRIDE ;
    virtual void setDoubleValueAtTimeAcrossDimensions(TimeValue time, const std::vector<double>& values, DimIdx dimensionStartIndex = DimIdx(0), ViewSetSpec view = ViewSetSpec::all(), ValueChangedReasonEnum reason = eValueChangedReasonUserEdited, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE ;
    virtual void setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& keysPerDimension, ValueChangedReasonEnum reason = eValueChangedReasonUserEdited) OVERRIDE ;
    //////////// End from AnimatingObjectI

Q_SIGNALS:


    ///emitted when the state of a curve changed at the indicated dimension
    void curveChanged(DimSpec);

    void curveColorChanged(DimSpec);

private:

    CurvePtr getParametricCurveInternal(DimIdx dimension, ViewIdx view, ParametricKnobDimViewPtr* data) const;

    void signalCurveChanged(DimSpec dimension, const KnobDimViewBasePtr& data);

    virtual KnobDimViewBasePtr createDimViewData() const OVERRIDE;

    ValueChangedReturnCodeEnum setKeyFrameInternal(TimeValue time,
                                                   double value,
                                                   DimIdx dimension,
                                                   ViewIdx view,
                                                   KeyFrame* newKey);

    virtual void resetExtraToDefaultValue(DimSpec dimension, ViewSetSpec view) OVERRIDE FINAL;
    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const OVERRIDE FINAL;
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

    static const std::string _typeNameStr;
};

inline KnobParametricPtr
toKnobParametric(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobParametric>(knob);
}

/**
 * @brief A Table containing strings. The number of columns is static.
 **/
class KnobTable
    : public QObject, public KnobStringBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // derives from KnobI, parent of KnobLayer, KnobPath
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobTable(const KnobHolderPtr& holder,
              const std::string &description,
              int dimension,
              bool declaredByPlugin);

    KnobTable(const KnobHolderPtr& holder,
              const QString &description,
              int dimension,
              bool declaredByPlugin);

public:
    virtual ~KnobTable();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    void getTable(std::list<std::vector<std::string> >* table);
    void getTableSingleCol(std::list<std::string>* table);

    void decodeFromKnobTableFormat(const std::string& value, std::list<std::vector<std::string> >* table);
    std::string encodeToKnobTableFormat(const std::list<std::vector<std::string> >& table);
    std::string encodeToKnobTableFormatSingleCol(const std::list<std::string>& table);

    void setTable(const std::list<std::vector<std::string> >& table);
    void setTableSingleCol(const std::list<std::string>& table);
    void appendRow(const std::vector<std::string>& row);
    void appendRowSingleCol(const std::string& row);
    void insertRow(int index, const std::vector<std::string>& row);
    void insertRowSingleCol(int index, const std::string& row);
    void removeRow(int index);

    virtual int getColumnsCount() const = 0;
    virtual std::string getColumnLabel(int col) const = 0;
    virtual bool isCellEnabled(int row, int col, const QStringList& values) const = 0;
    virtual bool isCellBracketDecorated(int /*row*/,
                                        int /*col*/) const
    {
        return false;
    }

    virtual bool isColumnEditable(int /*col*/)
    {
        return true;
    }

    virtual bool useEditButton() const
    {
        return true;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL
    {
        return false;
    }
};

class KnobLayers
    : public KnobTable
{
    Q_DECLARE_TR_FUNCTIONS(KnobLayers)

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobLayers(const KnobHolderPtr& holder,
               const std::string &description,
               int dimension,
               bool declaredByPlugin)
        : KnobTable(holder, description, dimension, declaredByPlugin)
    {
    }

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobLayers(holder, label, dimension, declaredByPlugin));
    }

    static KnobLayersPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobLayersPtr(new KnobLayers(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual ~KnobLayers()
    {
    }


    virtual int getColumnsCount() const OVERRIDE FINAL
    {
        return 3;
    }

    virtual std::string getColumnLabel(int col) const OVERRIDE FINAL
    {
        if (col == 0) {
            return tr("Name").toStdString();
        } else if (col == 1) {
            return tr("Channels").toStdString();
        } else if (col == 2) {
            return tr("Components Type").toStdString();
        } else {
            return std::string();
        }
    }

    virtual bool isCellEnabled(int /*row*/,
                               int /*col*/,
                               const QStringList& /*values*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isColumnEditable(int col) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        if (col == 1) {
            return false;
        }

        return true;
    }

    static const std::string & typeNameStatic() WARN_UNUSED_RETURN;

private:

    virtual const std::string & typeName() const OVERRIDE FINAL
    {
        return typeNameStatic();
    }

    static const std::string _typeNameStr;
};

inline KnobTablePtr
toKnobTable(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobTable>(knob);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_KNOBTYPES_H
