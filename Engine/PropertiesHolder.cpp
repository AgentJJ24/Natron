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


#include "PropertiesHolder.h"

NATRON_NAMESPACE_ENTER

PropertiesHolder::PropertiesHolder()
: _properties()
, _propertiesInitialized(false)
{
}

PropertiesHolder::PropertiesHolder(const PropertiesHolder& other)
: _properties()
, _propertiesInitialized(other._propertiesInitialized)
{
    for (std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator it = other._properties.begin(); it!=other._properties.end(); ++it) {
        boost::shared_ptr<PropertyBase> duplicate = it->second->createDuplicate();
        _properties.insert(std::make_pair(it->first, duplicate));
    }
}

PropertiesHolder::~PropertiesHolder()
{
    
}

NATRON_NAMESPACE_EXIT
