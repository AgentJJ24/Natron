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


#include "EffectInstanceActionResults.h"

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/Hash64.h"
#include "Engine/NodeMetadata.h"


namespace bip = boost::interprocess;

NATRON_NAMESPACE_ENTER;

EffectInstanceActionKeyBase::EffectInstanceActionKeyBase(U64 nodeTimeInvariantHash,
                                                         TimeValue time,
                                                         ViewIdx view,
                                                         const RenderScale& scale,
                                                         const std::string& pluginID)
: _data()
{
    _data.nodeTimeInvariantHash = nodeTimeInvariantHash;
    _data.time = time;
    _data.view = view;
    _data.scale = scale;
    setHolderPluginID(pluginID);
}

void
EffectInstanceActionKeyBase::appendToHash(Hash64* hash) const
{
    hash->append(_data.nodeTimeInvariantHash);
    hash->append((double)_data.time);
    hash->append((int)_data.view);
    hash->append(_data.scale.x);
    hash->append(_data.scale.y);
}

void
EffectInstanceActionKeyBase::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const
{
    objectPointers->push_back(writeNamedSharedObject(_data, objectNamesPrefix + "KeyData", segment));
    CacheEntryKeyBase::toMemorySegment(segment, objectNamesPrefix, objectPointers);
} // toMemorySegment

void
EffectInstanceActionKeyBase::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix)
{
    readNamedSharedObject(objectNamesPrefix + "KeyData", segment, &_data);
    CacheEntryKeyBase::fromMemorySegment(segment, objectNamesPrefix);
} // fromMemorySegment

std::size_t
EffectInstanceActionKeyBase::getMetadataSize() const
{
    std::size_t ret = CacheEntryKeyBase::getMetadataSize();
    ret += sizeof(_data.nodeTimeInvariantHash);
    ret += sizeof(_data.time);
    ret += sizeof(_data.view);
    ret += sizeof(_data.scale);
    return ret;
}

GetRegionOfDefinitionResults::GetRegionOfDefinitionResults()
: CacheEntryBase(appPTR->getCache())
{

}


GetRegionOfDefinitionResultsPtr
GetRegionOfDefinitionResults::create(const GetRegionOfDefinitionKeyPtr& key)
{
    GetRegionOfDefinitionResultsPtr ret(new GetRegionOfDefinitionResults());
    ret->setKey(key);
    return ret;
}

const RectD&
GetRegionOfDefinitionResults::getRoD() const
{
    return _rod;
}

void
GetRegionOfDefinitionResults::setRoD(const RectD& rod)
{
    _rod = rod;
}

std::size_t
GetRegionOfDefinitionResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    ret += sizeof(_rod);
    return ret;
}


void
GetRegionOfDefinitionResults::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    objectPointers->push_back(writeNamedSharedObject(_rod, objectNamesPrefix + "rod", segment));
    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
} // toMemorySegment

void
GetRegionOfDefinitionResults::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    readNamedSharedObject(objectNamesPrefix + "rod", segment, &_rod);
    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);
} // fromMemorySegment

IsIdentityResults::IsIdentityResults()
: CacheEntryBase(appPTR->getCache())
, _data()
{
    _data.identityInputNb = -1;
    _data.identityTime = TimeValue(0);
    _data.identityView = ViewIdx(0);
}

IsIdentityResultsPtr
IsIdentityResults::create(const IsIdentityKeyPtr& key)
{
    IsIdentityResultsPtr ret(new IsIdentityResults());
    ret->setKey(key);
    return ret;

}


void
IsIdentityResults::getIdentityData(int* identityInputNb, TimeValue* identityTime, ViewIdx* identityView) const
{
    *identityInputNb = _data.identityInputNb;
    *identityTime = _data.identityTime;
    *identityView = _data.identityView;
}

void
IsIdentityResults::setIdentityData(int identityInputNb, TimeValue identityTime, ViewIdx identityView)
{
    _data.identityInputNb = identityInputNb;
    _data.identityTime = identityTime;
    _data.identityView = identityView;
}

std::size_t
IsIdentityResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    ret += sizeof(_data.identityInputNb);
    ret += sizeof(_data.identityTime);
    ret += sizeof(_data.identityView);
    return ret;
}

void
IsIdentityResults::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    objectPointers->push_back(writeNamedSharedObject(_data, objectNamesPrefix + "Results", segment));
    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
} // toMemorySegment

void
IsIdentityResults::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    readNamedSharedObject(objectNamesPrefix + "Results", segment, &_data);
    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);
} // fromMemorySegment

GetFramesNeededResults::GetFramesNeededResults()
: CacheEntryBase(appPTR->getCache())
, _framesNeeded()
{

}

GetFramesNeededResultsPtr
GetFramesNeededResults::create(const GetFramesNeededKeyPtr& key)
{
    GetFramesNeededResultsPtr ret(new GetFramesNeededResults());
    ret->setKey(key);
    return ret;

}


void
GetFramesNeededResults::getFramesNeeded(FramesNeededMap *framesNeeded) const
{
    *framesNeeded = _framesNeeded;
}

void
GetFramesNeededResults::setFramesNeeded(const FramesNeededMap &framesNeeded)
{
    _framesNeeded = framesNeeded;
}

std::size_t
GetFramesNeededResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    // Hint a fake size, that's enough to ensure the memory allocation is ok
    ret += 1024;
    return ret;
}

// This is made complicated so it can be inserted in interprocess data structures.
typedef bip::allocator<ViewIdx, ExternalSegmentType::segment_manager> ViewIdx_allocator;
typedef bip::allocator<RangeD, ExternalSegmentType::segment_manager> RangeD_allocator;

// A vector of RangeD
typedef bip::vector<RangeD, RangeD_allocator> RangeDVector_ExternalSegment;

typedef bip::allocator<RangeDVector_ExternalSegment, ExternalSegmentType::segment_manager> RangeDVector_allocator;

typedef std::pair<const ViewIdx, RangeDVector_ExternalSegment> FrameRangesMap_value_type;

typedef bip::allocator<FrameRangesMap_value_type, ExternalSegmentType::segment_manager> FrameRangesMap_value_type_allocator;

// A map<ViewIdx, vector<RangeD> >
typedef bip::map<ViewIdx, RangeDVector_ExternalSegment, std::less<ViewIdx>, FrameRangesMap_value_type_allocator> FrameRangesMap_ExternalSegment;

typedef std::pair<const int, FrameRangesMap_ExternalSegment> FramesNeededMap_value_type;

typedef bip::allocator<FramesNeededMap_value_type, ExternalSegmentType::segment_manager> FramesNeededMap_value_type_allocator;

// A map<int,  map<ViewIdx, vector<RangeD> > >
typedef bip::map<int, FrameRangesMap_ExternalSegment, std::less<int>, FramesNeededMap_value_type_allocator> FramesNeededMap_ExternalSegment;


typedef bip::allocator<FramesNeededMap_ExternalSegment, ExternalSegmentType::segment_manager> FramesNeededMap_ExternalSegment_allocator;

void
GetFramesNeededResults::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    // An allocator convertible to any allocator<T, segment_manager_t> type
    void_allocator alloc_inst (segment->get_segment_manager());

    FramesNeededMap_ExternalSegment* externalMap = segment->construct<FramesNeededMap_ExternalSegment>(std::string(objectNamesPrefix + "framesNeeded").c_str())(alloc_inst);
    if (!externalMap) {
        throw std::bad_alloc();
    }
    for (FramesNeededMap::const_iterator it = _framesNeeded.begin(); it != _framesNeeded.end(); ++it) {

        FrameRangesMap_ExternalSegment extFrameRangeMap(alloc_inst);

        for (FrameRangesMap::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

            RangeDVector_ExternalSegment extRangeVec(alloc_inst);
            for (std::size_t i = 0; i < it2->second.size(); ++i) {
                extRangeVec.push_back(it2->second[i]);
            }
            FrameRangesMap_value_type v = std::make_pair(it2->first, extRangeVec);
            extFrameRangeMap.insert(v);
        }

        FramesNeededMap_value_type v = std::make_pair(it->first, extFrameRangeMap);
        externalMap->insert(v);
    }
    
    objectPointers->push_back(segment->get_handle_from_address(externalMap));
    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
} // toMemorySegment

void
GetFramesNeededResults::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    FramesNeededMap_ExternalSegment *externalMap = segment->find<FramesNeededMap_ExternalSegment>(std::string(objectNamesPrefix + "framesNeeded").c_str()).first;
    if (!externalMap) {
        throw std::bad_alloc();
    }
    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);


    for (FramesNeededMap_ExternalSegment::iterator it = externalMap->begin(); it != externalMap->end(); ++it) {
        FrameRangesMap& frameRangeMap = _framesNeeded[it->first];
        for (FrameRangesMap_ExternalSegment::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            std::vector<RangeD> &rangeVec = frameRangeMap[it2->first];

            for (std::size_t i = 0; i < it2->second.size(); ++i) {
                rangeVec.push_back(it2->second[i]);
            }
        }
    }
} // fromMemorySegment



GetFrameRangeResults::GetFrameRangeResults()
: CacheEntryBase(appPTR->getCache())
, _range()
{
    _range.min = _range.max = 0;
}

GetFrameRangeResultsPtr
GetFrameRangeResults::create(const GetFrameRangeKeyPtr& key)
{
    GetFrameRangeResultsPtr ret(new GetFrameRangeResults());
    ret->setKey(key);
    return ret;

}


void
GetFrameRangeResults::getFrameRangeResults(RangeD *range) const
{
    *range = _range;
}

void
GetFrameRangeResults::setFrameRangeResults(const RangeD &range)
{
    _range = range;
}

std::size_t
GetFrameRangeResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    ret += sizeof(_range);
    return ret;
}


void
GetFrameRangeResults::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    objectPointers->push_back(writeNamedSharedObject(_range, objectNamesPrefix + "range", segment));
    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
} // toMemorySegment

void
GetFrameRangeResults::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    readNamedSharedObject(objectNamesPrefix + "range", segment, &_range);
    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);
} // fromMemorySegment



GetTimeInvariantMetaDatasResults::GetTimeInvariantMetaDatasResults()
: CacheEntryBase(appPTR->getCache())
, _metadatas()
{

}

GetTimeInvariantMetaDatasResultsPtr
GetTimeInvariantMetaDatasResults::create(const GetTimeInvariantMetaDatasKeyPtr& key)
{
    GetTimeInvariantMetaDatasResultsPtr ret(new GetTimeInvariantMetaDatasResults());
    ret->setKey(key);
    return ret;

}


const NodeMetadataPtr&
GetTimeInvariantMetaDatasResults::getMetadatasResults() const
{
    return _metadatas;
}

void
GetTimeInvariantMetaDatasResults::setMetadatasResults(const NodeMetadataPtr &metadatas)
{
    _metadatas = metadatas;
}

std::size_t
GetTimeInvariantMetaDatasResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    // Hint a fake size
    ret += 1024;
    return ret;
}


void
GetTimeInvariantMetaDatasResults::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    assert(_metadatas);
    _metadatas->toMemorySegment(segment, objectNamesPrefix, objectPointers);
    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
} // toMemorySegment

void
GetTimeInvariantMetaDatasResults::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    assert(_metadatas);
    _metadatas->fromMemorySegment(segment, objectNamesPrefix);
    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);
} // fromMemorySegment



GetComponentsResults::GetComponentsResults()
: CacheEntryBase(appPTR->getCache())
, _neededInputLayers()
, _producedLayers()
, _passThroughPlanes()
, _data()
{
    _data.passThroughInputNb = -1;
    _data.passThroughTime = TimeValue(0);
    _data.passThroughView = ViewIdx(0);
    _data.processAllLayers = false;
    _data.doR = _data.doG = _data.doB = _data.doA = true;

}

GetComponentsResultsPtr
GetComponentsResults::create(const GetComponentsKeyPtr& key)
{
    GetComponentsResultsPtr ret(new GetComponentsResults());
    ret->setKey(key);
    return ret;

}


void
GetComponentsResults::getResults(std::map<int, std::list<ImagePlaneDesc> >* neededInputLayers,
                                 std::list<ImagePlaneDesc>* producedLayers,
                                 std::list<ImagePlaneDesc>* passThroughPlanes,
                                 int *passThroughInputNb,
                                 TimeValue *passThroughTime,
                                 ViewIdx *passThroughView,
                                 std::bitset<4> *processChannels,
                                 bool *processAllLayers) const
{
    *neededInputLayers = _neededInputLayers;
    *producedLayers = _producedLayers;
    *passThroughPlanes = _passThroughPlanes;
    *passThroughInputNb = _data.passThroughInputNb;
    *passThroughTime = _data.passThroughTime;
    *passThroughView = _data.passThroughView;
    (*processChannels)[0] = _data.doR;
    (*processChannels)[1] = _data.doG;
    (*processChannels)[2] = _data.doB;
    (*processChannels)[3] = _data.doA;
    *processAllLayers = _data.processAllLayers;
}

void
GetComponentsResults::setResults(const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers,
                                 const std::list<ImagePlaneDesc>& producedLayers,
                                 const std::list<ImagePlaneDesc>& passThroughPlanes,
                                 int passThroughInputNb,
                                 TimeValue passThroughTime,
                                 ViewIdx passThroughView,
                                 std::bitset<4> processChannels,
                                 bool processAllLayers)
{
    _neededInputLayers = neededInputLayers;
    _producedLayers = producedLayers;
    _passThroughPlanes = passThroughPlanes;
    _data.passThroughInputNb = passThroughInputNb;
    _data.passThroughTime = passThroughTime;
    _data.passThroughView = passThroughView;
    _data.doR = processChannels[0];
    _data.doG = processChannels[1];
    _data.doB = processChannels[2];
    _data.doA = processChannels[3];
    _data.processAllLayers = processAllLayers;
}

std::size_t
GetComponentsResults::getMetadataSize() const
{
    std::size_t ret = CacheEntryBase::getMetadataSize();
    // Hint a fake size
    ret += 1024;
    return ret;
}

typedef bip::allocator<String_ExternalSegment, ExternalSegmentType::segment_manager> String_ExternalSegment_allocator;
typedef bip::vector<String_ExternalSegment, String_ExternalSegment_allocator> StringVector_ExternalSegment;

// A simplified version of ImagePlaneDesc class that can go in shared memory
class MM_ImagePlaneDesc
{

public:

    String_ExternalSegment planeID, planeLabel, channelsLabel;
    StringVector_ExternalSegment channels;

    MM_ImagePlaneDesc(const void_allocator& allocator)
    : planeID(allocator)
    , planeLabel(allocator)
    , channelsLabel(allocator)
    , channels(allocator)
    {

    }

    void operator=(const MM_ImagePlaneDesc& other)
    {
        planeID = other.planeID;
        planeLabel = other.planeLabel;
        channelsLabel = other.channelsLabel;
        channels = other.channels;
    }
};

typedef bip::allocator<MM_ImagePlaneDesc, ExternalSegmentType::segment_manager> ImagePlaneDesc_ExternalSegment_allocator;
typedef bip::vector<MM_ImagePlaneDesc, ImagePlaneDesc_ExternalSegment_allocator> ImagePlaneDescVector_ExternalSegment;

typedef std::pair<const int, ImagePlaneDescVector_ExternalSegment> NeededInputLayersValueType;
typedef bip::allocator<NeededInputLayersValueType, ExternalSegmentType::segment_manager> NeededInputLayersValueType_allocator;

typedef bip::map<int, ImagePlaneDescVector_ExternalSegment, std::less<int>, NeededInputLayersValueType_allocator> NeededInputLayersMap_ExternalSegment;

static void imageComponentsListToSharedMemoryComponentsList(const void_allocator& allocator, const std::list<ImagePlaneDesc>& inComps, ImagePlaneDescVector_ExternalSegment* outComps)
{
    for (std::list<ImagePlaneDesc>::const_iterator it = inComps.begin() ; it != inComps.end(); ++it) {
        MM_ImagePlaneDesc comps(allocator);

        const std::vector<std::string>& channels = it->getChannels();
        for (std::size_t i = 0; i < channels.size(); ++i) {
            String_ExternalSegment chan(allocator);
            chan.append(channels[i].c_str());
            comps.channels.push_back(chan);
        }
        comps.channelsLabel.append(it->getChannelsLabel().c_str());
        comps.planeID.append(it->getPlaneID().c_str());
        comps.planeLabel.append(it->getPlaneLabel().c_str());
        outComps->push_back(comps);
    }
}

static void imageComponentsListFromSharedMemoryComponentsList(const ImagePlaneDescVector_ExternalSegment& inComps, std::list<ImagePlaneDesc>* outComps)
{
    for (ImagePlaneDescVector_ExternalSegment::const_iterator it = inComps.begin() ; it != inComps.end(); ++it) {
        std::string planeID(it->planeID.c_str());
        std::string planeLabel(it->planeLabel.c_str());
        std::string channelsLabel(it->channelsLabel.c_str());
        std::vector<std::string> channels(it->channels.size());

        int i = 0;
        for (StringVector_ExternalSegment::const_iterator it2 = it->channels.begin(); it2 != it->channels.end(); ++it2, ++i) {
            channels[i] = std::string(it2->c_str());
        }
        ImagePlaneDesc c(planeID, planeLabel, channelsLabel, channels);
        outComps->push_back(c);
    }
}

void
GetComponentsResults::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers, void* tileDataPtr) const
{
    // An allocator convertible to any allocator<T, segment_manager_t> type
    void_allocator alloc_inst(segment->get_segment_manager());

    {
        NeededInputLayersMap_ExternalSegment *neededLayers = segment->construct<NeededInputLayersMap_ExternalSegment>(std::string(objectNamesPrefix + "neededInputLayers").c_str())(alloc_inst);
        if (!neededLayers) {
            throw std::bad_alloc();
        }

        for (std::map<int, std::list<ImagePlaneDesc> >::const_iterator it = _neededInputLayers.begin(); it != _neededInputLayers.end(); ++it) {
            ImagePlaneDescVector_ExternalSegment vec(alloc_inst);
            imageComponentsListToSharedMemoryComponentsList(alloc_inst, it->second, &vec);

            NeededInputLayersValueType v = std::make_pair(it->first, vec);
            neededLayers->insert(v);
        }
        objectPointers->push_back(segment->get_handle_from_address(neededLayers));
    }
    {
        ImagePlaneDescVector_ExternalSegment *producedLayers = segment->construct<ImagePlaneDescVector_ExternalSegment>(std::string(objectNamesPrefix + "producedLayers").c_str())(alloc_inst);
        if (!producedLayers) {
            throw std::bad_alloc();
        }
        imageComponentsListToSharedMemoryComponentsList(alloc_inst, _producedLayers, producedLayers);
        objectPointers->push_back(segment->get_handle_from_address(producedLayers));
    }
    {
        ImagePlaneDescVector_ExternalSegment *ptPlanes = segment->construct<ImagePlaneDescVector_ExternalSegment>(std::string(objectNamesPrefix + "passThroughLayers").c_str())(alloc_inst);
        if (!ptPlanes) {
            throw std::bad_alloc();
        }
        imageComponentsListToSharedMemoryComponentsList(alloc_inst, _passThroughPlanes, ptPlanes);
        objectPointers->push_back(segment->get_handle_from_address(ptPlanes));
    }

    objectPointers->push_back(writeNamedSharedObject(_data, objectNamesPrefix + "PassThroughData", segment));

    CacheEntryBase::toMemorySegment(segment, objectNamesPrefix, objectPointers, tileDataPtr);
} // toMemorySegment

void
GetComponentsResults::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, const void* tileDataPtr)
{
    {
        NeededInputLayersMap_ExternalSegment *neededLayers = segment->find<NeededInputLayersMap_ExternalSegment>(std::string(objectNamesPrefix + "neededInputLayers").c_str()).first;
        if (!neededLayers) {
            throw std::bad_alloc();
        }
        for (NeededInputLayersMap_ExternalSegment::const_iterator it = neededLayers->begin(); it != neededLayers->end(); ++it) {
            std::list<ImagePlaneDesc>& comps = _neededInputLayers[it->first];
            imageComponentsListFromSharedMemoryComponentsList(it->second, &comps);
        }
    }
    {
        ImagePlaneDescVector_ExternalSegment* producedLayers = segment->find<ImagePlaneDescVector_ExternalSegment>(std::string(objectNamesPrefix + "producedLayers").c_str()).first;
        if (!producedLayers) {
            throw std::bad_alloc();
        }
        imageComponentsListFromSharedMemoryComponentsList(*producedLayers, &_producedLayers);
    }
    {
        ImagePlaneDescVector_ExternalSegment* ptLayers = segment->find<ImagePlaneDescVector_ExternalSegment>(std::string(objectNamesPrefix + "passThroughLayers").c_str()).first;
        if (!ptLayers) {
            throw std::bad_alloc();
        }
        imageComponentsListFromSharedMemoryComponentsList(*ptLayers, &_passThroughPlanes);
    }
    readNamedSharedObject(objectNamesPrefix + "PassThroughData", segment, &_data);

    CacheEntryBase::fromMemorySegment(segment, objectNamesPrefix, tileDataPtr);
} // fromMemorySegment


NATRON_NAMESPACE_EXIT;
