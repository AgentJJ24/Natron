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

#ifndef NATRON_ENGINE_KNOBFILE_H
#define NATRON_ENGINE_KNOBFILE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <map>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "Engine/KnobTypes.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/******************************KnobFile**************************************/

class KnobFile
    : public QObject, public AnimatingKnobStringHelper
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobFile(const KnobHolderPtr& holder,
             const std::string &description,
             int dimension,
             bool declaredByPlugin);

public:

    enum KnobFileDialogTypeEnum
    {
        eKnobFileDialogTypeOpenFile,
        eKnobFileDialogTypeOpenFileSequences,
        eKnobFileDialogTypeSaveFile,
        eKnobFileDialogTypeSaveFileSequences
    };

    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobFile(holder, label, dimension, declaredByPlugin));
    }

    static KnobFilePtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobFilePtr(new KnobFile(holder, label.toStdString(), dimension, declaredByPlugin));
    }

    virtual ~KnobFile();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    static const std::string & typeNameStatic();

    void setDialogType(KnobFileDialogTypeEnum type) {
        _dialogType = type;
    }

    KnobFileDialogTypeEnum getDialogType() const
    {
        return _dialogType;
    }

    void setDialogFilters(const std::vector<std::string>& filters)
    {
        _dialogFilters = filters;
    }

    const std::vector<std::string>& getDialogFilters() const
    {
        return _dialogFilters;
    }

    void open_file()
    {
        Q_EMIT openFile();
    }

    void reloadFile();


    std::string getRawFileName(DimIdx dimension = DimIdx(0), ViewIdx view = ViewIdx(0)) WARN_UNUSED_RETURN;

    virtual std::string getValue(DimIdx dimension = DimIdx(0), ViewIdx view = ViewIdx(0), bool clampToMinMax = true) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getValueAtTime(TimeValue time, DimIdx dimension = DimIdx(0), ViewIdx view = ViewIdx(0), bool clampToMinMax = true) OVERRIDE FINAL WARN_UNUSED_RETURN;

Q_SIGNALS:

    void openFile();

private:

    /**
     * @brief a KnobFile is never animated but it's value may change, indicate this to the plug-in
     **/
    virtual bool evaluateValueChangeOnTimeChange() const OVERRIDE FINAL { return true; }

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;
    static const std::string _typeNameStr;
    KnobFileDialogTypeEnum _dialogType;
    std::vector<std::string> _dialogFilters;
};


/******************************KnobPath**************************************/


/**
 * @brief The string value is encoded the following way:
 * <Name>Lala</Name><Value>MyValue</Value>
 **/
class KnobPath
    : public KnobTable
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobPath(const KnobHolderPtr& holder,
             const std::string &description,
             int dimension,
             bool declaredByPlugin);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobHelperPtr(new KnobPath(holder, label, dimension, declaredByPlugin));
    }

    static KnobPathPtr create(const KnobHolderPtr& holder,
                                const QString &label,
                                int dimension,
                                bool declaredByPlugin = true)
    {
        return KnobPathPtr(new KnobPath(holder, label.toStdString(), dimension, declaredByPlugin));
    }


    static const std::string & typeNameStatic();


    void setMultiPath(bool b);

    bool isMultiPath() const;

    /*
       @brief same as setMultiPath except that there will be only variable names, no values
     */
    void setAsStringList(bool b);
    bool getIsStringList() const;

    void getPaths(std::list<std::string>* paths);
    void prependPath(const std::string& path);
    void appendPath(const std::string& path);


    virtual int getColumnsCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return _isStringList ? 1 : 2;
    }

    virtual std::string getColumnLabel(int col) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        switch (col) {
        case 0:

            return "Name";
        case 1:

            return "Value";
        default:

            return "";
        }
    }

    virtual bool isCellEnabled(int row, int col, const QStringList& values) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isCellBracketDecorated(int row, int col) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool useEditButton() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return _isMultiPath && !_isStringList;
    }

private:

    static std::string generateUniquePathID(const std::list<std::vector<std::string> >& paths);
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _isMultiPath;
    bool _isStringList;
};

inline KnobFilePtr
toKnobFile(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobFile>(knob);
}


inline KnobPathPtr
toKnobPath(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobPath>(knob);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_KNOBFILE_H
