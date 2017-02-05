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


#include "ImagePrivate.h"

NATRON_NAMESPACE_ENTER;

void
ImagePrivate::initTileAndFetchFromCache(const Image::InitStorageArgs& args, int tx, int ty, int nTilesWidth, int tileSizeX, int tileSizeY)
{
    CachePtr cache = appPTR->getCache();

    int tile_i = nTilesWidth * ty + tx;
    Image::Tile& tile = tiles[tile_i];

    const std::string& planeID = args.layer.getPlaneID();

    // How many buffer should we make for a tile
    // A mono channel image should have one per channel
    std::vector<int> channelIndices;
    switch (args.bufferFormat) {
        case eImageBufferLayoutMonoChannelTiled: {

            for (int nc = 0; nc < args.layer.getNumComponents(); ++nc) {
                if (args.components[nc]) {
                    channelIndices.push_back(nc);
                }
            }
        }   break;
        case eImageBufferLayoutRGBACoplanarFullRect:
        case eImageBufferLayoutRGBAPackedFullRect:
            channelIndices.push_back(-1);
            break;
    }


    switch (args.bufferFormat) {
        case eImageBufferLayoutMonoChannelTiled:
            assert(tileSizeX != 0 && tileSizeY != 0);
            // The tile bounds may not necessarily be a square if we are on the edge.
            tile.tileBounds.x1 = args.bounds.x1 + (tx * tileSizeX);
            tile.tileBounds.y1 = args.bounds.y1 + (ty * tileSizeY);
            tile.tileBounds.x2 = std::min(tile.tileBounds.x1 + tileSizeX, args.bounds.x2);
            tile.tileBounds.y2 = std::min(tile.tileBounds.y1 + tileSizeY, args.bounds.y2);
            break;
        case eImageBufferLayoutRGBACoplanarFullRect:
        case eImageBufferLayoutRGBAPackedFullRect:
            // Single tile that covers the entire image
            tile.tileBounds = args.bounds;
            break;
    }


    tile.perChannelTile.resize(channelIndices.size());

    for (std::size_t c = 0; c < channelIndices.size(); ++c) {

        Image::MonoChannelTile& thisChannelTile = tile.perChannelTile[c];
        thisChannelTile.channelIndex = channelIndices[c];

        std::string channelName;
        switch (args.bufferFormat) {
            case eImageBufferLayoutMonoChannelTiled: {
                const std::vector<std::string>& compNames = args.layer.getChannels();
                assert(thisChannelTile.channelIndex >= 0 && thisChannelTile.channelIndex < (int)compNames.size());
                channelName = planeID + "." + compNames[thisChannelTile.channelIndex];
            }   break;
            case eImageBufferLayoutRGBACoplanarFullRect:
            case eImageBufferLayoutRGBAPackedFullRect:
                channelName = planeID;
                break;
        }





        boost::shared_ptr<AllocateMemoryArgs> allocArgs;

        CacheImageTileStoragePtr cachedBuffer;
        {
            // Allocate a new entry
            switch (args.storage) {
                case eStorageModeDisk: {
                    cachedBuffer.reset(new CacheImageTileStorage(cache));
                    thisChannelTile.buffer = cachedBuffer;
                    boost::shared_ptr<AllocateMemoryArgs> a(new AllocateMemoryArgs());
                    a->bitDepth = args.bitdepth;
                    allocArgs = a;
                }   break;
                case eStorageModeGLTex: {
                    GLImageStoragePtr buffer(new GLImageStorage());
                    thisChannelTile.buffer = buffer;
                    boost::shared_ptr<GLAllocateMemoryArgs> a(new GLAllocateMemoryArgs());
                    a->textureTarget = args.textureTarget;
                    a->glContext = args.glContext;
                    a->bounds = tile.tileBounds;
                    a->bitDepth = args.bitdepth;
                    allocArgs = a;
                }   break;
                case eStorageModeRAM: {
                    RAMImageStoragePtr buffer(new RAMImageStorage());
                    thisChannelTile.buffer = buffer;
                    boost::shared_ptr<RAMAllocateMemoryArgs> a(new RAMAllocateMemoryArgs());
                    a->bitDepth = args.bitdepth;
                    a->bounds = tile.tileBounds;

                    if (thisChannelTile.channelIndex == -1) {
                        a->numComponents = (std::size_t)args.layer.getNumComponents();
                    } else {
                        a->numComponents = 1;
                    }
                    allocArgs = a;
                }   break;
                case eStorageModeNone:
                    assert(false);
                    throw std::bad_alloc();
                    break;
            }
            assert(allocArgs && thisChannelTile.buffer);

            if (!args.delayAllocation) {
                // Allocate the memory for the tile.
                // This may throw a std::bad_alloc
                thisChannelTile.buffer->allocateMemory(*allocArgs);
            } else {
                // Delay the allocation
                thisChannelTile.buffer->setAllocateMemoryArgs(allocArgs);
            }
        } // allocArgs

        // This is the key for the tile at the requested draft/mipmap level
        ImageTileKeyPtr requestedScaleKey;
        if (cachePolicy != eCacheAccessModeNone) {
            requestedScaleKey.reset(new ImageTileKey(args.nodeTimeInvariantHash,
                                                     args.time,
                                                     args.view,
                                                     channelName,
                                                     args.proxyScale,
                                                     args.mipMapLevel,
                                                     args.isDraft,
                                                     args.bitdepth,
                                                     tx,
                                                     ty));
            cachedBuffer->setKey(requestedScaleKey);
        }

        // If the entry wants to be cached but we don't want to read from the cache
        // we must remove from the cache any entry that already exists at the given hash.
        if (cachePolicy == eCacheAccessModeWriteOnly) {
            CacheEntryLockerPtr locker = cache->get(cachedBuffer);
            if (locker->getStatus() == CacheEntryLocker::eCacheEntryStatusCached) {
                cache->removeEntry(cachedBuffer);
            }
        }

        // Look in the cache
        if (cachePolicy == eCacheAccessModeReadWrite || cachePolicy == eCacheAccessModeWriteOnly) {

            // First look for a tile at the proxy + mipmap scale, if not found look for a tile at proxy scale and downscale it.
            // This is the default cache lookup scale: for OpenGL textures, always assume them at full proxy scale
            // since downscaling is handled by OpenGL itself
            int nMipMapLookups;
            unsigned firstLookupLevel;
            if (args.storage != eStorageModeRAM && args.storage != eStorageModeDisk) {
                nMipMapLookups = 1;
                firstLookupLevel = 0;
            } else {
                nMipMapLookups = (args.mipMapLevel != 0) ? 2 : 1;
                firstLookupLevel = args.mipMapLevel;
            }

            // Retain the pointer give by the Cache::get function for the key we are interested in.
            CacheEntryLockerPtr requestedScaleLocker;

            bool isCached = false;
            for (int mipmap_i = 0; mipmap_i < nMipMapLookups; ++mipmap_i) {

                const unsigned int lookupLevel = mipmap_i == 0 ? firstLookupLevel : 0;

                // Only look for a draft tile in the cache if the image allows draft
                const int nDraftLookups = args.isDraft ? 2 : 1;

                for (int draft_i = 0; draft_i < nDraftLookups; ++draft_i) {

                    const bool useDraft = (const bool)draft_i;

                    ImageTileKeyPtr keyToReadCache(new ImageTileKey(args.nodeTimeInvariantHash,
                                                                    args.time,
                                                                    args.view,
                                                                    channelName,
                                                                    args.proxyScale,
                                                                    lookupLevel,
                                                                    useDraft,
                                                                    args.bitdepth,
                                                                    tx,
                                                                    ty));

                    assert(cachedBuffer);
                    cachedBuffer->setKey(keyToReadCache);

                    // Store the entry locker pointer
                    thisChannelTile.entryLocker = cache->get(cachedBuffer);

                    if (useDraft == args.isDraft && lookupLevel == args.mipMapLevel) {
                        assert(requestedScaleKey->getHash() == keyToReadCache->getHash());
                        requestedScaleLocker = thisChannelTile.entryLocker;
                    }

                    if (thisChannelTile.entryLocker->getStatus() == CacheEntryLocker::eCacheEntryStatusCached) {
                        isCached = true;
                        // We found a cache entry, don't continue to look for a tile computed in draft mode.
                        break;
                    }
                } // for each draft mode to check
                if (isCached) {

                    if (args.storage == eStorageModeRAM || args.storage == eStorageModeDisk) {
                        // If the image fetched is at a upper scale, we must downscale
                        if (lookupLevel != firstLookupLevel) {
                            assert(firstLookupLevel > lookupLevel);

                            const unsigned int downscaleLevels = firstLookupLevel - lookupLevel;

                            // Make a new view of this tile with a format that downscaleMipMap understands
                            // The copy will not actually copy the pixels, just the buffer memory pointer
                            ImagePtr fullScaleImage;
                            {
                                Image::InitStorageArgs tmpArgs;
                                tmpArgs.bounds = tile.tileBounds;
                                tmpArgs.renderArgs = renderArgs;
                                tmpArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
                                tmpArgs.layer = channelIndices.size() > 1 ? ImagePlaneDesc::getAlphaComponents() : layer;
                                tmpArgs.bitdepth = args.bitdepth;
                                tmpArgs.proxyScale = args.proxyScale;
                                tmpArgs.mipMapLevel = args.mipMapLevel;
                                tmpArgs.externalBuffer = thisChannelTile.buffer;
                                tmpArgs.nodeTimeInvariantHash = args.nodeTimeInvariantHash;
                                tmpArgs.time = args.time;
                                tmpArgs.view = args.view;
                                fullScaleImage = Image::create(tmpArgs);
                            }

                            ImagePtr downscaledImage = fullScaleImage->downscaleMipMap(tile.tileBounds, downscaleLevels);

                            assert(downscaledImage->_imp->tiles.size() == 1);
                            assert(downscaledImage->_imp->tiles[0].perChannelTile.size() == 1);

                            // Since we downscaled a single tile of the same size and same number of components and same bitdepth
                            // as this tile, we can just copy the pointer
                            thisChannelTile.buffer = downscaledImage->_imp->tiles[0].perChannelTile[0].buffer;

                        } // must downscale
                    }
                    break;
                } // isCached
            } // for each mip map lvel to check
            if (!isCached) {
                assert(requestedScaleLocker);
                cachedBuffer->setKey(requestedScaleKey);
                thisChannelTile.entryLocker = requestedScaleLocker;
            }
        } // useCache

    } // for each channel

} // initTileAndFetchFromCache


void
ImagePrivate::initFromExternalBuffer(const Image::InitStorageArgs& args)
{
    assert(args.externalBuffer);

    if (tiles.size() != 1) {
        // When providing an external buffer, there must be a single tile!
        throw std::bad_alloc();
    }
    if (args.bitdepth != args.externalBuffer->getBitDepth()) {
        // When providing an external buffer, the bitdepth must be the same as the requested depth
        throw std::bad_alloc();
    }

    tiles[0].perChannelTile.resize(1);
    tiles[0].tileBounds = args.bounds;

    Image::MonoChannelTile& perChannelTile = tiles[0].perChannelTile[0];

    GLImageStoragePtr isGLBuffer = toGLImageStorage(args.externalBuffer);
    CacheImageTileStoragePtr isMMAPBuffer = toCacheImageTileStorage(args.externalBuffer);
    RAMImageStoragePtr isRAMBuffer = toRAMImageStorage(args.externalBuffer);
    if (isGLBuffer) {
        if (args.storage != eStorageModeGLTex) {
            throw std::bad_alloc();
        }
        if (isGLBuffer->getBounds() != args.bounds) {
            throw std::bad_alloc();
        }
        perChannelTile.buffer = isGLBuffer;
    } else if(isMMAPBuffer) {
        if (args.storage != eStorageModeDisk) {
            throw std::bad_alloc();
        }
        if (isMMAPBuffer->getBounds() != args.bounds) {
            throw std::bad_alloc();
        }
        // Mmap tiles are mono channel
        if (args.layer.getNumComponents() != 1) {
            throw std::bad_alloc();
        }
        perChannelTile.buffer = isMMAPBuffer;
    } else if (isRAMBuffer) {
        if (args.storage != eStorageModeRAM) {
            throw std::bad_alloc();
        }
        if (isRAMBuffer->getBounds() != args.bounds) {
            throw std::bad_alloc();
        }
        if (isRAMBuffer->getNumComponents() != (std::size_t)args.layer.getNumComponents()) {
            throw std::bad_alloc();
        }
        perChannelTile.buffer = isRAMBuffer;
    } else {
        // Unrecognized storage
        throw std::bad_alloc();
    }

} // initFromExternalBuffer

int
ImagePrivate::getNTilesPerLine() const
{
    if (tiles.empty()) {
        return 0;
    }
    int tileSizeX = tiles[0].tileBounds.width();
    return bounds.width() / tileSizeX;
}


void
ImagePrivate::insertTilesInCache()
{
    // The image must have cache enabled, otherwise don't call this function.
    assert(cachePolicy == eCacheAccessModeWriteOnly ||
           cachePolicy == eCacheAccessModeReadWrite);

    CachePtr cache = appPTR->getCache();

    bool renderAborted = renderArgs->isRenderAborted();

    for (std::size_t tile_i = 0; tile_i < tiles.size(); ++tile_i) {
        Image::Tile& tile = tiles[tile_i];

        for (std::size_t c = 0; c < tile.perChannelTile.size(); ++c) {

            Image::MonoChannelTile& thisChannelTile = tile.perChannelTile[c];

            // If the tile is already cached, don't push it to the cache
            if (!thisChannelTile.entryLocker) {
                continue;
            }
            CacheEntryLocker::CacheEntryStatusEnum status = thisChannelTile.entryLocker->getStatus();
            if (status == CacheEntryLocker::eCacheEntryStatusMustCompute && !renderAborted) {
                thisChannelTile.entryLocker->insertInCache();
            }
            thisChannelTile.entryLocker.reset();
        }
        
    } // for each tile
} // insertTilesInCache

const Image::Tile*
ImagePrivate::getTile(int x, int y) const
{
    if (!bounds.contains(x, y)) {
        // Out of bounds
        return 0;
    }
    if (tiles.size() == 1) {
        // Single tiled image
        return &tiles[0];
    }

    int tileSizeX = tiles[0].tileBounds.width();
    int tileSizeY = tiles[0].tileBounds.height();

    // Tiles must be aligned
    assert(bounds.width() % tileSizeX == 0);
    assert(bounds.height() % tileSizeY == 0);

    int nTilesPerLine = bounds.width() / tileSizeX;
    int tileX = std::floor((double)(x - bounds.x1) / tileSizeX);
    int tileY = std::floor((double)(y - bounds.y1) / tileSizeY);

    int tile_i = tileY * nTilesPerLine + tileX;
    assert(tile_i >= 0 && tile_i < (int)tiles.size());
    return &tiles[tile_i];
    
} // getTile

RectI
ImagePrivate::getTilesCoordinates(const RectI& pixelCoordinates) const
{
    if (tiles.empty()) {
        return RectI();
    }
    int tileSizeX = tiles[0].tileBounds.width();
    int tileSizeY = tiles[0].tileBounds.height();

    // Round the pixel coords to the tile size
    RectI roundedPixelCoords;
    roundedPixelCoords.x1 = (int)std::floor( ( (double)pixelCoordinates.x1 ) / tileSizeX ) * tileSizeX;
    roundedPixelCoords.y1 = (int)std::floor( ( (double)pixelCoordinates.y1 ) / tileSizeY ) * tileSizeY;
    roundedPixelCoords.x2 = (int)std::ceil( ( (double)pixelCoordinates.x2 ) / tileSizeX ) * tileSizeX;
    roundedPixelCoords.y2 = (int)std::ceil( ( (double)pixelCoordinates.y2 ) / tileSizeY ) * tileSizeY;

    // Ensure the tiles are aligned
    assert(((roundedPixelCoords.x1 - bounds.x1) % tileSizeX) == 0);
    assert(((roundedPixelCoords.y1 - bounds.y1) % tileSizeY) == 0);
    assert(((roundedPixelCoords.x2 - bounds.x1) % tileSizeX) == 0);
    assert(((roundedPixelCoords.y2 - bounds.y1) % tileSizeY) == 0);

    RectI tilesRect;
    tilesRect.x1 = (roundedPixelCoords.x1 - bounds.x1) / tileSizeX;
    tilesRect.y1 = (roundedPixelCoords.y1 - bounds.y1) / tileSizeY;
    tilesRect.x2 = (roundedPixelCoords.x2 - bounds.x1) / tileSizeX;
    tilesRect.y2 = (roundedPixelCoords.y2 - bounds.y1) / tileSizeY;
    return tilesRect;
} // getTilesCoordinates


/**
 * @brief If copying pixels from fromImage to toImage cannot be copied directly, this function
 * returns a temporary image that is suitable to copy then to the toImage.
 **/
ImagePtr
ImagePrivate::checkIfCopyToTempImageIsNeeded(const Image& fromImage, const Image& toImage, const RectI& roi)
{
    // Copying from a tiled buffer is not trivial unless we are not tiled.
    // If both are tiled, convert the original image to a packed format first
    if (fromImage._imp->bufferFormat == eImageBufferLayoutMonoChannelTiled && toImage._imp->bufferFormat == eImageBufferLayoutMonoChannelTiled) {
        ImagePtr tmpImage;
        Image::InitStorageArgs args;
        args.renderArgs = fromImage._imp->renderArgs;
        args.bounds = roi;
        args.layer = fromImage._imp->layer;
        tmpImage = Image::create(args);

        Image::CopyPixelsArgs copyArgs;
        copyArgs.roi = roi;
        tmpImage->copyPixels(fromImage, copyArgs);
        return tmpImage;
    }

    // OpenGL textures may only be read from a RGBA packed buffer
    if (fromImage.getStorageMode() == eStorageModeGLTex) {

        // If this is also an OpenGL texture, check they have the same context otherwise first bring back
        // the image to CPU
        if (toImage.getStorageMode() == eStorageModeGLTex) {

            GLImageStoragePtr isGlEntry = toGLImageStorage(toImage._imp->tiles[0].perChannelTile[0].buffer);
            GLImageStoragePtr otherIsGlEntry = toGLImageStorage(fromImage._imp->tiles[0].perChannelTile[0].buffer);
            assert(isGlEntry && otherIsGlEntry);
            if (isGlEntry->getOpenGLContext() != otherIsGlEntry->getOpenGLContext()) {
                ImagePtr tmpImage;
                Image::InitStorageArgs args;
                args.renderArgs = fromImage._imp->renderArgs;
                args.bounds = fromImage.getBounds();
                args.layer = ImagePlaneDesc::getRGBAComponents();
                tmpImage = Image::create(args);

                Image::CopyPixelsArgs copyArgs;
                copyArgs.roi = roi;
                tmpImage->copyPixels(fromImage, copyArgs);
                return tmpImage;
            }
        }

        // Converting from OpenGL to CPU requires a RGBA buffer with the same bounds
        if (toImage._imp->bufferFormat != eImageBufferLayoutRGBAPackedFullRect || toImage.getComponentsCount() != 4 || toImage.getBounds() != fromImage.getBounds()) {
            ImagePtr tmpImage;
            Image::InitStorageArgs args;
            args.renderArgs = fromImage._imp->renderArgs;
            args.bounds = fromImage.getBounds();
            args.layer = ImagePlaneDesc::getRGBAComponents();
            tmpImage = Image::create(args);

            Image::CopyPixelsArgs copyArgs;
            copyArgs.roi = roi;
            tmpImage->copyPixels(fromImage, copyArgs);
            return tmpImage;

        }

        // All other cases can copy fine
        return ImagePtr();
    }

    // OpenGL textures may only be written from a RGBA packed buffer
    if (toImage.getStorageMode() == eStorageModeGLTex) {

        // Converting to OpenGl requires an RGBA buffer
        if (fromImage._imp->bufferFormat != eImageBufferLayoutRGBAPackedFullRect || fromImage.getComponentsCount() != 4) {
            ImagePtr tmpImage;
            Image::InitStorageArgs args;
            args.renderArgs = fromImage._imp->renderArgs;
            args.bounds = fromImage.getBounds();
            args.layer = ImagePlaneDesc::getRGBAComponents();
            tmpImage = Image::create(args);

            Image::CopyPixelsArgs copyArgs;
            copyArgs.roi = roi;
            tmpImage->copyPixels(fromImage, copyArgs);
            return tmpImage;
        }
    }

    // All other cases can copy fine
    return ImagePtr();
} // checkIfCopyToTempImageIsNeeded

class CopyUntiledToTileProcessor : public MultiThreadProcessorBase
{

    std::vector<int> _tileIndices;
    ImagePrivate* _imp;
    StorageModeEnum _toStorage;
    ImageBufferLayoutEnum _toBufferFormat;
    ImagePrivate* _fromImage;
    StorageModeEnum _fromStorage;
    ImageBufferLayoutEnum _fromBufferFormat;
    const Image::CopyPixelsArgs* _originalArgs;

public:

    CopyUntiledToTileProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : MultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~CopyUntiledToTileProcessor()
    {

    }

    void setData(const Image::CopyPixelsArgs* args, ImagePrivate* imp, StorageModeEnum toStorage, ImageBufferLayoutEnum toBufferFormat, ImagePrivate* fromImage, ImageBufferLayoutEnum fromBufferFormat, StorageModeEnum fromStorage, const std::vector<int>& tileIndices)
    {
        _tileIndices = tileIndices;
        _imp = imp;
        _toStorage = toStorage;
        _toBufferFormat= toBufferFormat;
        _fromImage = fromImage;
        _originalArgs = args;
        _fromStorage = fromStorage;
        _fromBufferFormat = fromBufferFormat;
    }

    virtual ActionRetCodeEnum launchThreads(unsigned int nCPUs = 0) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return MultiThreadProcessorBase::launchThreads(nCPUs);
    }

    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads,
                                                  const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        // Each threads get a rectangular portion but full scan-lines
        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tileIndices.size(), &fromIndex, &toIndex);

        if ( (toIndex - fromIndex) <= 0 ) {
            return eActionStatusOK;
        }

        Image::CopyPixelsArgs argsCpy = *_originalArgs;

        for (int i = fromIndex; i < toIndex; ++i) {
            // This is the tile to write to
            const Image::Tile& thisTile = _imp->tiles[_tileIndices[i]];

            thisTile.tileBounds.intersect(_originalArgs->roi, &argsCpy.roi);

            ImagePrivate::copyRectangle(_fromImage->tiles[0], _fromStorage, _fromBufferFormat, thisTile, _toStorage, _toBufferFormat, argsCpy, renderArgs);
        }
        return eActionStatusOK;
    }
};

void
ImagePrivate::copyUntiledImageToTiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args)
{
    assert(bufferFormat == eImageBufferLayoutMonoChannelTiled);
    assert(bounds.contains(args.roi) && fromImage._imp->bounds.contains(args.roi));

    // If this image is tiled, the other image must not be tiled
    assert(fromImage._imp->bufferFormat != eImageBufferLayoutMonoChannelTiled);

    assert(fromImage._imp->tiles[0].perChannelTile[0].channelIndex == -1);

    const int nTilesPerLine = getNTilesPerLine();
    const RectI tilesRect = getTilesCoordinates(args.roi);

    const StorageModeEnum fromStorage = fromImage.getStorageMode();
    const StorageModeEnum toStorage = tiles[0].perChannelTile[0].buffer->getStorageMode();




    std::vector<int> tileIndices;
    // Copy each tile individually
    for (int ty = tilesRect.y1; ty < tilesRect.y2; ++ty) {
        for (int tx = tilesRect.x1; tx < tilesRect.x2; ++tx) {
            int tile_i = tx + ty * nTilesPerLine;
            assert(tile_i >= 0 && tile_i < (int)tiles.size());
            tileIndices.push_back(tile_i);
        } // for all tiles horizontally
    } // for all tiles vertically


    if ((fromStorage == eStorageModeRAM || fromStorage == eStorageModeDisk) &&
        (toStorage == eStorageModeRAM || toStorage == eStorageModeDisk)) {
        CopyUntiledToTileProcessor processor(renderArgs);
        processor.setData(&args, this, toStorage, bufferFormat, fromImage._imp.get(), fromImage._imp->bufferFormat, fromStorage, tileIndices);
        ActionRetCodeEnum stat = processor.launchThreads();
        (void)stat;
    } else {
        for (std::size_t i = 0; i < tileIndices.size(); ++i) {
            Image::CopyPixelsArgs argsCpy = args;

            // This is the tile to write to
            const Image::Tile& thisTile = tiles[tileIndices[i]];
            thisTile.tileBounds.intersect(args.roi, &argsCpy.roi);

            ImagePrivate::copyRectangle(fromImage._imp->tiles[0], fromStorage, fromImage._imp->bufferFormat, thisTile, toStorage, bufferFormat, argsCpy, renderArgs);
        }
    }
    
} // copyUntiledImageToTiledImage


class CopyTiledToUntiledProcessor : public MultiThreadProcessorBase
{

    std::vector<int> _tileIndices;
    ImagePrivate* _imp;
    StorageModeEnum _toStorage;
    ImageBufferLayoutEnum _toBufferFormat;
    ImagePrivate* _fromImage;
    StorageModeEnum _fromStorage;
    ImageBufferLayoutEnum _fromBufferFormat;
    const Image::CopyPixelsArgs* _originalArgs;

public:

    CopyTiledToUntiledProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : MultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~CopyTiledToUntiledProcessor()
    {

    }

    void setData(const Image::CopyPixelsArgs* args, ImagePrivate* imp, StorageModeEnum toStorage, ImageBufferLayoutEnum toBufferFormat, ImagePrivate* fromImage, ImageBufferLayoutEnum fromBufferFormat, StorageModeEnum fromStorage, const std::vector<int>& tileIndices)
    {
        _tileIndices = tileIndices;
        _imp = imp;
        _toStorage = toStorage;
        _toBufferFormat= toBufferFormat;
        _fromImage = fromImage;
        _originalArgs = args;
        _fromStorage = fromStorage;
        _fromBufferFormat = fromBufferFormat;
    }

    virtual ActionRetCodeEnum launchThreads(unsigned int nCPUs = 0) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return MultiThreadProcessorBase::launchThreads(nCPUs);
    }

    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads,
                                                  const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        // Each threads get a rectangular portion but full scan-lines
        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tileIndices.size(), &fromIndex, &toIndex);

        if ( (toIndex - fromIndex) <= 0 ) {
            return eActionStatusOK;
        }

        Image::CopyPixelsArgs argsCpy = *_originalArgs;

        for (int i = fromIndex; i < toIndex; ++i) {
            // This is the tile to write to
            const Image::Tile& fromTile = _fromImage->tiles[_tileIndices[i]];

            fromTile.tileBounds.intersect(_originalArgs->roi, &argsCpy.roi);

            ImagePrivate::copyRectangle(fromTile, _fromStorage, _fromBufferFormat, _imp->tiles[0], _toStorage, _toBufferFormat, argsCpy, renderArgs);

        }
        return eActionStatusOK;
    }
};

void
ImagePrivate::copyTiledImageToUntiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args)
{
    // The input image may or may not be tiled, but we surely are not.
    assert(bufferFormat != eImageBufferLayoutMonoChannelTiled);
    assert(bounds.contains(args.roi) && fromImage._imp->bounds.contains(args.roi));
    assert(tiles[0].perChannelTile.size() == 1 && tiles[0].perChannelTile[0].channelIndex == -1);
    assert(tiles[0].perChannelTile[0].channelIndex == -1);

    const int nTilesPerLine = fromImage._imp->getNTilesPerLine();
    const RectI tilesRect = fromImage._imp->getTilesCoordinates(args.roi);

    const StorageModeEnum fromStorage = fromImage.getStorageMode();
    const StorageModeEnum toStorage = tiles[0].perChannelTile[0].buffer->getStorageMode();
    Image::CopyPixelsArgs argsCpy = args;

    std::vector<int> tileIndices;
    // Copy each tile individually
    for (int ty = tilesRect.y1; ty < tilesRect.y2; ++ty) {
        for (int tx = tilesRect.x1; tx < tilesRect.x2; ++tx) {
            int tile_i = tx + ty * nTilesPerLine;
            assert(tile_i >= 0 && tile_i < (int)fromImage._imp->tiles.size());
            tileIndices.push_back(tile_i);
        } // for all tiles horizontally
    } // for all tiles vertically


    if ((fromStorage == eStorageModeRAM || fromStorage == eStorageModeDisk) &&
        (toStorage == eStorageModeRAM || toStorage == eStorageModeDisk)) {
        CopyTiledToUntiledProcessor processor(renderArgs);
        processor.setData(&args, this, toStorage, bufferFormat, fromImage._imp.get(), fromImage._imp->bufferFormat, fromStorage, tileIndices);
        ActionRetCodeEnum stat = processor.launchThreads();
        (void)stat;
    } else {
        for (std::size_t i = 0; i < tileIndices.size(); ++i) {
            Image::CopyPixelsArgs argsCpy = args;

            // This is the tile to write to
            const Image::Tile& fromTile = fromImage._imp->tiles[tileIndices[i]];
            fromTile.tileBounds.intersect(args.roi, &argsCpy.roi);

            ImagePrivate::copyRectangle(fromTile, fromStorage, fromImage._imp->bufferFormat, tiles[0], toStorage, bufferFormat, argsCpy, renderArgs);
        }
    }

} // copyTiledImageToUntiledImage

void
ImagePrivate::copyUntiledImageToUntiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args)
{
    // The input image may or may not be tiled, but we surely are not.
    assert(bufferFormat != eImageBufferLayoutMonoChannelTiled);
    assert(bounds.contains(args.roi) && fromImage._imp->bounds.contains(args.roi));
    assert(fromImage._imp->tiles.size() == 1 && tiles.size() == 1);
    assert(tiles[0].perChannelTile.size() == 1 && tiles[0].perChannelTile[0].channelIndex == -1);
    assert(fromImage._imp->tiles[0].perChannelTile.size() == 1 && fromImage._imp->tiles[0].perChannelTile[0].channelIndex == -1);

    const StorageModeEnum fromStorage = fromImage.getStorageMode();
    const StorageModeEnum toStorage = tiles[0].perChannelTile[0].buffer->getStorageMode();

    ImagePrivate::copyRectangle(fromImage._imp->tiles[0], fromStorage, fromImage._imp->bufferFormat, tiles[0], toStorage, bufferFormat, args, renderArgs);

} // copyUntiledImageToUntiledImage

template <typename PIX, int maxValue, int nComps>
static void
halveImageForInternal(const void* srcPtrs[4],
                      const RectI& srcBounds,
                      void* dstPtrs[4],
                      const RectI& dstBounds)
{

    PIX* dstPixelPtrs[4];
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)dstPtrs, dstBounds.x1, dstBounds.y1, dstBounds, (PIX**)dstPixelPtrs, &dstPixelStride);

    const PIX* srcPixelPtrs[4];
    int srcPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)srcPtrs, srcBounds.x1, srcBounds.y1, srcBounds, (PIX**)srcPixelPtrs, &srcPixelStride);

    const int dstRowElementsCount = dstBounds.width() * dstPixelStride;
    const int srcRowElementsCount = srcBounds.width() * srcPixelStride;


    for (int y = 0; y < dstBounds.height(); ++y) {

        // The current dst row, at y, covers the src rows y*2 (thisRow) and y*2+1 (nextRow).
        const int srcy = y * 2;

        // Check that we are within srcBounds.
        const bool pickThisRow = srcBounds.y1 <= (srcy + 0) && (srcy + 0) < srcBounds.y2;
        const bool pickNextRow = srcBounds.y1 <= (srcy + 1) && (srcy + 1) < srcBounds.y2;

        const int sumH = (int)pickNextRow + (int)pickThisRow;
        assert(sumH == 1 || sumH == 2);

        for (int x = 0; x < dstBounds.width(); ++x) {

            // The current dst col, at y, covers the src cols x*2 (thisCol) and x*2+1 (nextCol).
            const int srcx = x * 2;

            // Check that we are within srcBounds.
            const bool pickThisCol = srcBounds.x1 <= (srcx + 0) && (srcx + 0) < srcBounds.x2;
            const bool pickNextCol = srcBounds.x1 <= (srcx + 1) && (srcx + 1) < srcBounds.x2;

            const int sumW = (int)pickThisCol + (int)pickNextCol;
            assert(sumW == 1 || sumW == 2);

            const int sum = sumW * sumH;
            assert(0 < sum && sum <= 4);

            for (int k = 0; k < nComps; ++k) {

                // Averaged pixels are as such:
                // a b
                // c d

                const PIX a = (pickThisCol && pickThisRow) ? *(srcPixelPtrs[k]) : 0;
                const PIX b = (pickNextCol && pickThisRow) ? *(srcPixelPtrs[k] + srcPixelStride) : 0;
                const PIX c = (pickThisCol && pickNextRow) ? *(srcPixelPtrs[k] + srcRowElementsCount) : 0;
                const PIX d = (pickNextCol && pickNextRow) ? *(srcPixelPtrs[k] + srcRowElementsCount + srcPixelStride)  : 0;

                assert( sumW == 2 || ( sumW == 1 && ( (a == 0 && c == 0) || (b == 0 && d == 0) ) ) );
                assert( sumH == 2 || ( sumH == 1 && ( (a == 0 && b == 0) || (c == 0 && d == 0) ) ) );

                *dstPixelPtrs[k] = (a + b + c + d) / sum;

                srcPixelPtrs[k] += srcPixelStride * 2;
                dstPixelPtrs[k] += dstPixelStride;
            } // for each component
            
        } // for each pixels on the line

        // Remove what was offset to the pointers during this scan-line and offset to the next
        for (int k = 0; k < nComps; ++k) {
            dstPixelPtrs[k] += (dstRowElementsCount - dstBounds.width() * dstPixelStride);
            srcPixelPtrs[k] += (srcRowElementsCount * 2 - dstBounds.width() * srcPixelStride);
        }
    }  // for each scan line
} // halveImageForInternal


template <typename PIX, int maxValue>
static void
halveImageForDepth(const void* srcPtrs[4],
                   int nComps,
                   const RectI& srcBounds,
                   void* dstPtrs[4],
                   const RectI& dstBounds)
{
    switch (nComps) {
        case 1:
            halveImageForInternal<PIX, maxValue, 1>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        case 2:
            halveImageForInternal<PIX, maxValue, 2>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        case 3:
            halveImageForInternal<PIX, maxValue, 3>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        case 4:
            halveImageForInternal<PIX, maxValue, 4>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        default:
            break;
    }
}


void
ImagePrivate::halveImage(const void* srcPtrs[4],
                         int nComps,
                         ImageBitDepthEnum bitDepth,
                         const RectI& srcBounds,
                         void* dstPtrs[4],
                         const RectI& dstBounds)
{
    switch ( bitDepth ) {
        case eImageBitDepthByte:
            halveImageForDepth<unsigned char, 255>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds);
            break;
        case eImageBitDepthShort:
            halveImageForDepth<unsigned short, 65535>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthFloat:
            halveImageForDepth<float, 1>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds);
            break;
        case eImageBitDepthNone:
            break;
    }
} // halveImage

template <typename PIX, int maxValue, int nComps>
bool
checkForNaNsInternal(void* ptrs[4],
                     const RectI& bounds,
                     const RectI& roi)
{

    PIX* dstPixelPtrs[4];
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)ptrs, roi.x1, roi.y1, bounds, (PIX**)dstPixelPtrs, &dstPixelStride);
    const int rowElementsCount = bounds.width() * dstPixelStride;

    bool hasnan = false;
    for (int y = roi.y1; y < roi.y2; ++y) {
        for (int x = roi.x1; x < roi.x2; ++x) {
            for (int k = 0; k < nComps; ++k) {
                // we remove NaNs, but infinity values should pose no problem
                // (if they do, please explain here which ones)
                if (*dstPixelPtrs[k] != *dstPixelPtrs[k]) { // check for NaN
                    *dstPixelPtrs[k] = 1.;
                    ++dstPixelPtrs[k];
                    hasnan = true;
                }
            }
        }
        // Remove what was done at the previous scan-line and got to the next
        for (int k = 0; k < nComps; ++k) {
            dstPixelPtrs[k] += (rowElementsCount - roi.width() * dstPixelStride);
        }
    } // for each scan-line

    return hasnan;
} // checkForNaNsInternal

template <typename PIX, int maxValue>
bool
checkForNaNsForDepth(void* ptrs[4],
                     int nComps,
                     const RectI& bounds,
                     const RectI& roi)
{
    switch (nComps) {
        case 1:
            return checkForNaNsInternal<PIX, maxValue, 1>(ptrs, bounds, roi);
            break;
        case 2:
            return checkForNaNsInternal<PIX, maxValue, 2>(ptrs, bounds, roi);
            break;
        case 3:
            return checkForNaNsInternal<PIX, maxValue, 3>(ptrs, bounds, roi);
            break;
        case 4:
            return checkForNaNsInternal<PIX, maxValue, 4>(ptrs, bounds, roi);
            break;
        default:
            break;
    }
    return false;

}


bool
ImagePrivate::checkForNaNs(void* ptrs[4],
                           int nComps,
                           ImageBitDepthEnum bitdepth,
                           const RectI& bounds,
                           const RectI& roi)
{
    switch ( bitdepth ) {
        case eImageBitDepthByte:
            return checkForNaNsForDepth<unsigned char, 255>(ptrs, nComps, bounds, roi);
            break;
        case eImageBitDepthShort:
            return checkForNaNsForDepth<unsigned short, 65535>(ptrs, nComps, bounds, roi);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthFloat:
            return checkForNaNsForDepth<float, 1>(ptrs, nComps, bounds, roi);
            break;
        case eImageBitDepthNone:
            break;
    }
    return false;
}

NATRON_NAMESPACE_EXIT;
