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

#include "Image.h"

#include <algorithm> // min, max
#include <cassert>
#include <cstring> // for std::memcpy, std::memset
#include <stdexcept>

#include <QtCore/QDebug>

#include "Engine/ImagePrivate.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

NATRON_NAMESPACE_ENTER;

Image::Image()
: _imp(new ImagePrivate())
{

}

ImagePtr
Image::create(const InitStorageArgs& args)
{
    ImagePtr ret(new Image);
    ret->init(args);
    return ret;
}

Image::~Image()
{

    pushTilesToCacheIfNotAborted();
    
    // If this image is the last image holding a pointer to memory buffers, ensure these buffers
    // gets deallocated in a specific thread and not a render thread
    std::list<ImageStorageBasePtr> toDeleteInDeleterThread;
    for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
        for (std::size_t c = 0;  c < _imp->tiles[i].perChannelTile.size(); ++c) {
            toDeleteInDeleterThread.push_back(_imp->tiles[i].perChannelTile[c].buffer);
        }
    }

    _imp->tiles.clear();
    
    if (!toDeleteInDeleterThread.empty()) {
        appPTR->deleteCacheEntriesInSeparateThread(toDeleteInDeleterThread);
    }

}

void
Image::discardTiles()
{
    _imp->cachePolicy = eCacheAccessModeNone;
}

void
Image::pushTilesToCacheIfNotAborted()
{
   
    // Push tiles to cache if needed
    if (_imp->cachePolicy == eCacheAccessModeReadWrite ||
        _imp->cachePolicy == eCacheAccessModeWriteOnly) {
        assert(_imp->renderArgs);
        _imp->insertTilesInCache();
    }

}

bool
Image::waitForPendingTiles()
{
    if (_imp->cachePolicy == eCacheAccessModeNone) {
        return true;
    }
    bool hasStuffToRender = false;
    for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
        for (std::size_t c = 0; c < _imp->tiles[i].perChannelTile.size(); ++c) {
            if (_imp->tiles[i].perChannelTile[c].entryLocker) {
                if (_imp->tiles[i].perChannelTile[c].entryLocker->getStatus() == CacheEntryLocker::eCacheEntryStatusComputationPending) {
                    _imp->tiles[i].perChannelTile[c].entryLocker->waitForPendingEntry();
                }
                CacheEntryLocker::CacheEntryStatusEnum status = _imp->tiles[i].perChannelTile[c].entryLocker->getStatus();
                assert(status == CacheEntryLocker::eCacheEntryStatusCached || status == CacheEntryLocker::eCacheEntryStatusMustCompute);

                if (status == CacheEntryLocker::eCacheEntryStatusMustCompute) {
                    hasStuffToRender = true;
                }
            }
        }
    }
    return !hasStuffToRender;
} // waitForPendingTiles

Image::InitStorageArgs::InitStorageArgs()
: bounds()
, storage(eStorageModeRAM)
, bitdepth(eImageBitDepthFloat)
, layer(ImagePlaneDesc::getRGBAComponents())
, components()
, cachePolicy(eCacheAccessModeNone)
, bufferFormat(eImageBufferLayoutRGBAPackedFullRect)
, proxyScale(1.)
, mipMapLevel(0)
, isDraft(false)
, nodeTimeInvariantHash(0)
, time(0)
, view(0)
, glContext()
, textureTarget(GL_TEXTURE_2D)
, externalBuffer()
, delayAllocation(false)
{
    // By default make all channels
    components[0] = components[1] = components[2] = components[3] = 1;
}


class TileFetcherProcessor : public MultiThreadProcessorBase
{

    std::vector<std::pair<int,int> > _tileIndices;
    int _nTilesWidth, _nTilesHeight;
    int _tileSizeX, _tileSizeY;
    ImagePrivate* _imp;
    const Image::InitStorageArgs* _args;

public:

    TileFetcherProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : MultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~TileFetcherProcessor()
    {

    }

    void setData(ImagePrivate* imp, const Image::InitStorageArgs* args, int nTilesWidth, int nTilesHeight, int tileSizeX, int tileSizeY)
    {
        // Initialize each tile
        int tx = 0, ty = 0;
        int nTiles = nTilesWidth * nTilesHeight;
        _tileIndices.resize(nTiles);
        for (int tile_i = 0; tile_i < nTiles; ++tile_i) {

            _tileIndices[tile_i].first = tx;
            _tileIndices[tile_i].second = ty;

            // Increment tile coords
            if (tx == nTilesWidth - 1) {
                tx = 0;
                ++ty;
            } else {
                ++tx;
            }
        } // for each tile


        _nTilesWidth = nTilesWidth;
        _nTilesHeight = nTilesHeight;
        _tileSizeX = tileSizeX;
        _tileSizeY = tileSizeY;
        _imp = imp;
        _args = args;
    }

    virtual ActionRetCodeEnum launchThreads(unsigned int nCPUs = 0) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return MultiThreadProcessorBase::launchThreads(nCPUs);
    }

    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads,
                                                  const TreeRenderNodeArgsPtr& /*renderArgs*/) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        // Each threads get a rectangular portion but full scan-lines
        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tileIndices.size(), &fromIndex, &toIndex);

        if ( (toIndex - fromIndex) <= 0 ) {
            return eActionStatusOK;
        }

        for (int i = fromIndex; i < toIndex; ++i) {
            int tx = _tileIndices[i].first;
            int ty = _tileIndices[i].second;
            _imp->initTileAndFetchFromCache(*_args, tx, ty, _nTilesWidth, _tileSizeX, _tileSizeY);
        }
        return eActionStatusOK;
    }
};

void
Image::init(const Image::InitStorageArgs& args)
{

    // Should be initialized once!
    assert(_imp->tiles.empty());
    if (!_imp->tiles.empty()) {
        throw std::bad_alloc();
    }

    // The bounds of the image must not be empty
    if (args.bounds.isNull()) {
        throw std::bad_alloc();
    }

    RenderScale proxyPlusMipMapScale = args.proxyScale;
    {
        double mipMapScale = getScaleFromMipMapLevel(args.mipMapLevel);
        proxyPlusMipMapScale.x *= mipMapScale;
        proxyPlusMipMapScale.y *= mipMapScale;
    }

    _imp->bounds = args.bounds;
    _imp->cachePolicy = args.cachePolicy;
    if (args.storage != eStorageModeDisk) {
        // We can only cache stuff on disk.
        _imp->cachePolicy = eCacheAccessModeNone;
    }
    _imp->bufferFormat = args.bufferFormat;
    _imp->layer = args.layer;
    _imp->proxyScale = args.proxyScale;
    _imp->mipMapLevel = args.mipMapLevel;
    _imp->renderArgs = args.renderArgs;

    // OpenGL texture back-end only supports 32-bit float RGBA packed format.
    assert(args.storage != eStorageModeGLTex || (args.bufferFormat == eImageBufferLayoutRGBAPackedFullRect && args.bitdepth == eImageBitDepthFloat));
    if (args.storage == eStorageModeGLTex && (args.bufferFormat != eImageBufferLayoutRGBAPackedFullRect || args.bitdepth != eImageBitDepthFloat)) {
        throw std::bad_alloc();
    }

    // MMAP storage only supports mono channel tiles.
    assert(args.storage != eStorageModeDisk || args.bufferFormat == eImageBufferLayoutMonoChannelTiled);
    if (args.storage == eStorageModeDisk && args.bufferFormat != eImageBufferLayoutMonoChannelTiled) {
        throw std::bad_alloc();
    }

    // If allocating OpenGL textures, ensure the context is current
    OSGLContextAttacherPtr contextLocker;
    if (args.storage == eStorageModeGLTex) {
        contextLocker = OSGLContextAttacher::create(args.glContext);
        contextLocker->attach();
    }


    // For tiled layout, get the number of tiles in X and Y depending on the bounds and the tile zie.
    int nTilesHeight,nTilesWidth;
    int tileSizeX = 0, tileSizeY = 0;
    switch (args.bufferFormat) {
        case eImageBufferLayoutMonoChannelTiled: {
            // The size of a tile depends on the bitdepth
            Cache::getTileSizePx(args.bitdepth, &tileSizeX, &tileSizeY);
            nTilesHeight = std::ceil((double)_imp->bounds.height() / tileSizeY);
            nTilesWidth = std::ceil((double)_imp->bounds.width() / tileSizeX);
        }   break;
        case eImageBufferLayoutRGBACoplanarFullRect:
        case eImageBufferLayoutRGBAPackedFullRect:
            nTilesHeight = 1;
            nTilesWidth = 1;
            break;
    }


    int nTiles = nTilesWidth * nTilesHeight;
    assert(nTiles > 0);
    _imp->tiles.resize(nTiles);

    if (args.externalBuffer) {
        _imp->initFromExternalBuffer(args);
    } else {
        TileFetcherProcessor processor(args.renderArgs);
        processor.setData(_imp.get(), &args, nTilesWidth, nTilesHeight, tileSizeX, tileSizeY);
        ActionRetCodeEnum stat = processor.launchThreads();
        (void)stat;
    }

} // init

Image::CopyPixelsArgs::CopyPixelsArgs()
: roi()
, conversionChannel(0)
, alphaHandling(Image::eAlphaChannelHandlingFillFromChannel)
, monoConversion(Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers)
, srcColorspace(eViewerColorSpaceLinear)
, dstColorspace(eViewerColorSpaceLinear)
, unPremultIfNeeded(false)
, skipDestinationTilesMarkedCached(false)
, forceCopyEvenIfBuffersHaveSameLayout(false)
{

}


void
Image::copyPixels(const Image& other, const CopyPixelsArgs& args)
{
    if (_imp->tiles.empty() || other._imp->tiles.empty()) {
        // Nothing to copy
        return;
    }

    // Roi must intersect both images bounds
    RectI roi;
    if (!other._imp->bounds.intersect(args.roi, &roi)) {
        return;
    }
    if (!_imp->bounds.intersect(args.roi, &roi)) {
        return;
    }


    // Optimize: try to just copy the memory buffer pointers instead of copying the memory itself
    if (!args.forceCopyEvenIfBuffersHaveSameLayout && roi == _imp->bounds) {

        bool copyPointers = true;
        if (_imp->tiles.size() != other._imp->tiles.size()) {
            copyPointers = false;
        }
        if (copyPointers) {
            StorageModeEnum thisStorage = getStorageMode();
            StorageModeEnum otherStorage = other.getStorageMode();
            if ((thisStorage == eStorageModeGLTex || otherStorage == eStorageModeGLTex)) {
                copyPointers = false;
            }
        }
        if (copyPointers && _imp->bounds != other._imp->bounds) {
            copyPointers = false;
        }
        if (copyPointers && getBitDepth() != other.getBitDepth()) {
            copyPointers = false;
        }
        if (copyPointers && _imp->tiles[0].perChannelTile.size() != other._imp->tiles[0].perChannelTile.size()) {
            copyPointers = false;
        }
        if (copyPointers && _imp->layer.getNumComponents() != other._imp->layer.getNumComponents()) {
            copyPointers = false;
        }
        // Only support copying buffers with different layouts if they have 1 component only
        if (copyPointers) {
            ImageBufferLayoutEnum thisLayout = _imp->bufferFormat;
            ImageBufferLayoutEnum otherFormat = other._imp->bufferFormat;
            if (thisLayout != otherFormat && _imp->layer.getNumComponents() != 1) {
                copyPointers = false;
            }
        }

        if (copyPointers) {
            assert(_imp->tiles.size() == other._imp->tiles.size());
            for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
                assert(_imp->tiles[i].perChannelTile.size() == other._imp->tiles[i].perChannelTile.size());
                for (std::size_t c = 0; c < _imp->tiles[c].perChannelTile.size(); ++c) {
                    _imp->tiles[i].perChannelTile[c].buffer = other._imp->tiles[i].perChannelTile[c].buffer;
                }
            }
        }

    } // !args.forceCopyEvenIfBuffersHaveSameLayout

    ImagePtr tmpImage = ImagePrivate::checkIfCopyToTempImageIsNeeded(other, *this, roi);

    const Image* fromImage = tmpImage? tmpImage.get() : &other;

    // Update the roi before calling copyRectangle
    CopyPixelsArgs argsCpy = args;
    argsCpy.roi = roi;

    if (_imp->bufferFormat == eImageBufferLayoutMonoChannelTiled) {
        // UNTILED ---> TILED
        _imp->copyUntiledImageToTiledImage(*fromImage, argsCpy);

    } else {

        // Optimize the case where nobody is tiled
        if (fromImage->_imp->bufferFormat != eImageBufferLayoutMonoChannelTiled) {

            // UNTILED ---> UNTILED
            _imp->copyUntiledImageToUntiledImage(*fromImage, argsCpy);

        } else {
            // TILED ---> UNTILED
            _imp->copyTiledImageToUntiledImage(*fromImage, argsCpy);
        }

    } // isTiled
} // copyPixels

void
Image::ensureBuffersAllocated()
{
    for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
        for (std::size_t c = 0; c < _imp->tiles[i].perChannelTile.size(); ++c) {
            if (_imp->tiles[i].perChannelTile[c].buffer->hasAllocateMemoryArgs()) {
                _imp->tiles[i].perChannelTile[c].buffer->allocateMemoryFromSetArgs();
            }
        }
    }
} // ensureBuffersAllocated

ImageBufferLayoutEnum
Image::getBufferFormat() const
{
    return _imp->bufferFormat;
}

StorageModeEnum
Image::getStorageMode() const
{
    if (_imp->tiles.empty()) {
        return eStorageModeNone;
    }
    assert(!_imp->tiles[0].perChannelTile.empty() && _imp->tiles[0].perChannelTile[0].buffer);
    return _imp->tiles[0].perChannelTile[0].buffer->getStorageMode();
}

const RectI&
Image::getBounds() const
{
    return _imp->bounds;
}

const RenderScale&
Image::getProxyScale() const
{
    return _imp->proxyScale;
}

unsigned int
Image::getMipMapLevel() const
{
    return _imp->mipMapLevel;
}


double
Image::getScaleFromMipMapLevel(unsigned int level)
{
    return 1. / (1 << level);
}


unsigned int
Image::getLevelFromScale(double s)
{
    assert(0. < s && s <= 1.);
    int retval = -std::floor(std::log(s) / M_LN2 + 0.5);
    assert(retval >= 0);
    return retval;
}

unsigned int
Image::getComponentsCount() const
{
    return _imp->layer.getNumComponents();
}


const ImagePlaneDesc&
Image::getLayer() const
{
    return _imp->layer;
}


ImageBitDepthEnum
Image::getBitDepth() const
{
    if (_imp->tiles.empty()) {
        return eImageBitDepthNone;
    }
    assert(!_imp->tiles[0].perChannelTile.empty() && _imp->tiles[0].perChannelTile[0].buffer);
    return _imp->tiles[0].perChannelTile[0].buffer->getBitDepth();
}

GLImageStoragePtr
Image::getGLImageStorage() const
{
    if (_imp->tiles.empty()) {
        return GLImageStoragePtr();
    }
    if (_imp->tiles[0].perChannelTile.empty()) {
        return GLImageStoragePtr();
    }
    GLImageStoragePtr isGLEntry = toGLImageStorage(_imp->tiles[0].perChannelTile[0].buffer);
    return isGLEntry;
}

void
Image::getCPUTileData(const Tile& tile, ImageBufferLayoutEnum layout, CPUTileData* data)
{
    memset(data->ptrs, 0, sizeof(void*) * 4);
    data->nComps = 0;
    data->bitDepth = eImageBitDepthNone;

    for (std::size_t i = 0; i < tile.perChannelTile.size(); ++i) {
        RAMImageStoragePtr fromIsRAMBuffer = toRAMImageStorage(tile.perChannelTile[i].buffer);
        CacheImageTileStoragePtr fromIsMMAPBuffer = toCacheImageTileStorage(tile.perChannelTile[i].buffer);

        if (!fromIsMMAPBuffer && !fromIsRAMBuffer) {
            continue;
        }
        if (i == 0) {
            if (fromIsRAMBuffer) {
                data->ptrs[0] = fromIsRAMBuffer->getData();
                data->tileBounds = fromIsRAMBuffer->getBounds();
                data->bitDepth = fromIsRAMBuffer->getBitDepth();
                data->nComps = fromIsRAMBuffer->getNumComponents();

                if (layout == eImageBufferLayoutRGBACoplanarFullRect) {
                    // Coplanar requires offsetting
                    assert(tile.perChannelTile.size() == 1);
                    std::size_t planeSize = data->nComps * data->tileBounds.area() * getSizeOfForBitDepth(data->bitDepth);
                    if (data->nComps > 1) {
                        data->ptrs[1] = (char*)data->ptrs[0] + planeSize;
                        if (data->nComps > 2) {
                            data->ptrs[2] = (char*)data->ptrs[1] + planeSize;
                            if (data->nComps > 3) {
                                data->ptrs[3] = (char*)data->ptrs[2] + planeSize;
                            }
                        }
                    }
                }
            } else {
                data->ptrs[0] = fromIsMMAPBuffer->getData();
                data->tileBounds = fromIsMMAPBuffer->getBounds();
                data->bitDepth = fromIsMMAPBuffer->getBitDepth();
                data->nComps = tile.perChannelTile.size();
            }
        } else {
            assert(layout == eImageBufferLayoutMonoChannelTiled);
            int channelIndex = tile.perChannelTile[i].channelIndex;
            assert(channelIndex >= 0 && channelIndex < 4);
            if (fromIsRAMBuffer) {
                data->ptrs[channelIndex] = fromIsRAMBuffer->getData();
            } else {
                data->ptrs[channelIndex] = fromIsMMAPBuffer->getData();
            }
        }
    } // for each channel
} // getCPUTileData

void
Image::getCPUTileData(const Image::Tile& tile,
                      Image::CPUTileData* data) const
{
    getCPUTileData(tile, _imp->bufferFormat, data);
}

int
Image::getNumTiles() const
{
    return (int)_imp->tiles.size();
}


CacheAccessModeEnum
Image::getCachePolicy() const
{
    return _imp->cachePolicy;
}

std::list<RectI>
Image::getRestToRender(bool *hasPendingResults) const
{

    std::list<RectI> ret;
    if (_imp->cachePolicy != eCacheAccessModeReadWrite) {
        return std::list<RectI>();
    }

    for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
        bool hasChannelNotCached = false;
        for (std::size_t c = 0; c < _imp->tiles[i].perChannelTile.size(); ++c) {

            if (!_imp->tiles[i].perChannelTile[c].entryLocker) {
                continue;

            }
            CacheEntryLocker::CacheEntryStatusEnum status = _imp->tiles[i].perChannelTile[c].entryLocker->getStatus();
            if (status != CacheEntryLocker::eCacheEntryStatusCached) {
                hasChannelNotCached = true;
            }
            if (status == CacheEntryLocker::eCacheEntryStatusComputationPending) {
                *hasPendingResults = true;
            }
        }
        if (hasChannelNotCached) {
            if (!_imp->tiles[i].tileBounds.isNull()) {
                ret.push_back(_imp->tiles[i].tileBounds);
            }
        }
    }
    return ret;
} // getRestToRender

bool
Image::getTileAt(int tileIndex, Image::Tile* tile) const
{
    if (!tile || tileIndex < 0 || tileIndex > (int)_imp->tiles.size()) {
        return false;
    }
    *tile = _imp->tiles[tileIndex];
    return true;
}

void
Image::getABCDRectangles(const RectI& srcBounds, const RectI& biggerBounds, RectI& aRect, RectI& bRect, RectI& cRect, RectI& dRect)
{
    /*
     Compute the rectangles (A,B,C,D) where to set the image to 0

     AAAAAAAAAAAAAAAAAAAAAAAAAAAA
     AAAAAAAAAAAAAAAAAAAAAAAAAAAA
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     CCCCCCCCCCCCCCCCCCCCCCCCCCCC
     CCCCCCCCCCCCCCCCCCCCCCCCCCCC
     */
    aRect.x1 = biggerBounds.x1;
    aRect.y1 = srcBounds.y2;
    aRect.y2 = biggerBounds.y2;
    aRect.x2 = biggerBounds.x2;

    bRect.x1 = srcBounds.x2;
    bRect.y1 = srcBounds.y1;
    bRect.x2 = biggerBounds.x2;
    bRect.y2 = srcBounds.y2;

    cRect.x1 = biggerBounds.x1;
    cRect.y1 = biggerBounds.y1;
    cRect.x2 = biggerBounds.x2;
    cRect.y2 = srcBounds.y1;

    dRect.x1 = biggerBounds.x1;
    dRect.y1 = srcBounds.y1;
    dRect.x2 = srcBounds.x1;
    dRect.y2 = srcBounds.y2;

} // getABCDRectangles

class FillProcessor : public ImageMultiThreadProcessorBase
{
    void* _ptrs[4];
    RectI _tileBounds;
    ImageBitDepthEnum _bitDepth;
    int _nComps;
    RGBAColourF _color;

public:

    FillProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~FillProcessor()
    {
    }

    void setValues(void* ptrs[4], const RectI& tileBounds, ImageBitDepthEnum bitDepth, int nComps, const RGBAColourF& color)
    {
        memcpy(_ptrs, ptrs, sizeof(void*) * 4);
        _tileBounds = tileBounds;
        _bitDepth = bitDepth;
        _nComps = nComps;
        _color = color;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        ImagePrivate::fillCPU(_ptrs, _color.r, _color.g, _color.b, _color.a, _nComps, _bitDepth, _tileBounds, renderWindow, renderArgs);
        return eActionStatusOK;
    }
};

void
Image::fill(const RectI & roi,
            float r,
            float g,
            float b,
            float a)
{
    if (_imp->tiles.empty()) {
        return;
    }

    if (getStorageMode() == eStorageModeGLTex) {
        GLImageStoragePtr glEntry = toGLImageStorage(_imp->tiles[0].perChannelTile[0].buffer);
        _imp->fillGL(roi, r, g, b, a, glEntry);
        return;
    }



    for (std::size_t tile_i = 0; tile_i < _imp->tiles.size(); ++tile_i) {

        Image::CPUTileData tileData;
        getCPUTileData(_imp->tiles[tile_i], &tileData);
        RectI tileRoI;
        roi.intersect(tileData.tileBounds, &tileRoI);

        RGBAColourF color = {r, g, b, a};

        FillProcessor processor(_imp->renderArgs);
        processor.setValues(tileData.ptrs, tileData.tileBounds, tileData.bitDepth, tileData.nComps, color);
        processor.setRenderWindow(tileRoI);
        processor.process();

    }

} // fill

void
Image::fillZero(const RectI& roi)
{
    fill(roi, 0., 0., 0., 0.);
}

void
Image::fillBoundsZero()
{
    fillZero(getBounds());
}

void
Image::ensureBounds(const RectI& roi)
{
    if (_imp->bounds.contains(roi)) {
        return;
    }

    ImagePtr tmpImage;
    {
        Image::InitStorageArgs initArgs;
        initArgs.bounds = _imp->bounds;
        initArgs.bounds.merge(roi);
        initArgs.layer = getLayer();
        initArgs.bitdepth = getBitDepth();
        initArgs.bufferFormat = getBufferFormat();
        initArgs.storage = getStorageMode();
        initArgs.mipMapLevel = getMipMapLevel();
        initArgs.proxyScale = getProxyScale();
        GLImageStoragePtr isGlEntry = getGLImageStorage();
        if (isGlEntry) {
            initArgs.textureTarget = isGlEntry->getGLTextureTarget();
            initArgs.glContext = isGlEntry->getOpenGLContext();
        }
        tmpImage = Image::create(initArgs);
    }
    Image::CopyPixelsArgs cpyArgs;
    cpyArgs.roi = _imp->bounds;
    tmpImage->copyPixels(*this, cpyArgs);

    // Swap images so that this image becomes the resized one.
    _imp.swap(tmpImage->_imp);
} // ensureBounds

void
Image::getChannelPointers(const void* ptrs[4],
                          int x, int y,
                          const RectI& bounds,
                          int nComps,
                          ImageBitDepthEnum bitdepth,
                          void* outPtrs[4],
                          int* pixelStride)
{
    switch (bitdepth) {
        case eImageBitDepthByte:
            getChannelPointers<unsigned char>((const unsigned char**)ptrs, x, y, bounds, nComps, (unsigned char**)outPtrs, pixelStride);
            break;
        case eImageBitDepthShort:
            getChannelPointers<unsigned short>((const unsigned short**)ptrs, x, y, bounds, nComps, (unsigned short**)outPtrs, pixelStride);
            break;
        case eImageBitDepthFloat:
            getChannelPointers<float>((const float**)ptrs, x, y, bounds, nComps, (float**)outPtrs, pixelStride);
            break;
        default:
            memset(outPtrs, 0, sizeof(void*) * 4);
            *pixelStride = 0;
            break;
    }
}


const unsigned char*
Image::pixelAtStatic(int x,
                     int y,
                     const RectI& bounds,
                     int nComps,
                     int dataSizeOf,
                     const unsigned char* buf)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 ) ) {
        return NULL;
    } else {
        const unsigned char* ret = buf;
        if (!ret) {
            return 0;
        }
        std::size_t compDataSize = dataSizeOf * nComps;
        ret = ret + (std::size_t)( y - bounds.y1 ) * compDataSize * bounds.width()
        + (std::size_t)( x - bounds.x1 ) * compDataSize;

        return ret;
    }
}

unsigned char*
Image::pixelAtStatic(int x,
                     int y,
                     const RectI& bounds,
                     int nComps,
                     int dataSizeOf,
                     unsigned char* buf)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 ) ) {
        return NULL;
    } else {
        unsigned char* ret = buf;
        if (!ret) {
            return 0;
        }
        std::size_t compDataSize = dataSizeOf * nComps;
        ret = ret + (std::size_t)( y - bounds.y1 ) * compDataSize * bounds.width()
              + (std::size_t)( x - bounds.x1 ) * compDataSize;

        return ret;
    }
}

std::string
Image::getFormatString(const ImagePlaneDesc& comps,
                       ImageBitDepthEnum depth)
{
    std::string s = comps.getPlaneLabel() + '.' + comps.getChannelsLabel();

    s.append( getDepthString(depth) );

    return s;
}

std::string
Image::getDepthString(ImageBitDepthEnum depth)
{
    std::string s;

    switch (depth) {
    case eImageBitDepthByte:
        s += "8u";
        break;
    case eImageBitDepthShort:
        s += "16u";
        break;
    case eImageBitDepthHalf:
        s += "16f";
        break;
    case eImageBitDepthFloat:
        s += "32f";
        break;
    case eImageBitDepthNone:
        break;
    }

    return s;
}

bool
Image::isBitDepthConversionLossy(ImageBitDepthEnum from,
                                 ImageBitDepthEnum to)
{
    int sizeOfFrom = getSizeOfForBitDepth(from);
    int sizeOfTo = getSizeOfForBitDepth(to);

    return sizeOfTo < sizeOfFrom;
}


ImagePtr
Image::downscaleMipMap(const RectI & roi, unsigned int downscaleLevels) const
{
    // If we don't have to downscale or this is an OpenGL texture there's nothing to do
    if (downscaleLevels == 0 || getStorageMode() == eStorageModeGLTex) {
        return boost::const_pointer_cast<Image>(shared_from_this());
    }

    if (_imp->tiles.empty()) {
        return ImagePtr();
    }

    // The roi must be contained in the bounds of the image
    assert(_imp->bounds.contains(roi));
    if (!_imp->bounds.contains(roi)) {
        return ImagePtr();
    }

    RectI dstRoI  = roi.downscalePowerOfTwoSmallestEnclosing(downscaleLevels);
    ImagePtr mipmapImage;

    RectI previousLevelRoI = roi;
    ImageConstPtr previousLevelImage = shared_from_this();

    // The downscaling function only supports full rect format.
    // Copy this image to the appropriate format if necessary.
    if (previousLevelImage->_imp->bufferFormat == eImageBufferLayoutMonoChannelTiled) {
        InitStorageArgs args;
        args.bounds = roi;
        args.renderArgs = _imp->renderArgs;
        args.layer = previousLevelImage->_imp->layer;
        args.bitdepth = previousLevelImage->getBitDepth();
        args.proxyScale = previousLevelImage->getProxyScale();
        args.mipMapLevel = previousLevelImage->getMipMapLevel();
        ImagePtr tmpImg = Image::create(args);

        CopyPixelsArgs cpyArgs;
        cpyArgs.roi = roi;
        tmpImg->copyPixels(*this, cpyArgs);

        previousLevelImage = tmpImg;
    }


    // Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 0; i < downscaleLevels; ++i) {

        // Halve the smallest enclosing po2 rect as we need to render a minimum of the renderWindow
        RectI halvedRoI = previousLevelRoI.downscalePowerOfTwoSmallestEnclosing(1);

        // Allocate an image with half the size of the source image
        {
            InitStorageArgs args;
            args.bounds = halvedRoI;
            args.renderArgs = _imp->renderArgs;
            args.layer = previousLevelImage->_imp->layer;
            args.bitdepth = previousLevelImage->getBitDepth();
            args.proxyScale = previousLevelImage->getProxyScale();
            args.mipMapLevel = previousLevelImage->getMipMapLevel() + 1;
            mipmapImage = Image::create(args);
        }

        Image::CPUTileData srcTileData;
        getCPUTileData(previousLevelImage->_imp->tiles[0], &srcTileData);

        Image::CPUTileData dstTileData;
        getCPUTileData(mipmapImage->_imp->tiles[0], &dstTileData);

        ImagePrivate::halveImage((const void**)srcTileData.ptrs, srcTileData.nComps, srcTileData.bitDepth, srcTileData.tileBounds, dstTileData.ptrs, dstTileData.tileBounds);

        // Switch for next pass
        previousLevelRoI = halvedRoI;
        previousLevelImage = mipmapImage;
    } // for all downscale levels
    return mipmapImage;

} // downscaleMipMap

bool
Image::checkForNaNs(const RectI& roi)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return false;
    }
    if (getStorageMode() == eStorageModeGLTex) {
        return false;
    }

    bool hasNan = false;

    for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
        Image::CPUTileData tileData;
        getCPUTileData(_imp->tiles[i], &tileData);

        RectI tileRoi;
        roi.intersect(tileData.tileBounds, &tileRoi);
        hasNan |= _imp->checkForNaNs(tileData.ptrs, tileData.nComps, tileData.bitDepth, tileData.tileBounds, tileRoi);
    }
    return hasNan;

} // checkForNaNs


class MaskMixProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUTileData _srcTileData, _maskTileData, _dstTileData;
    double _mix;
    bool _maskInvert;
public:

    MaskMixProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    , _srcTileData()
    , _maskTileData()
    , _dstTileData()
    , _mix(0)
    , _maskInvert(false)
    {

    }

    virtual ~MaskMixProcessor()
    {
    }

    void setValues(const Image::CPUTileData& srcTileData,
                   const Image::CPUTileData& maskTileData,
                   const Image::CPUTileData& dstTileData,
                   double mix,
                   bool maskInvert)
    {
        _srcTileData = srcTileData;
        _maskTileData = maskTileData;
        _dstTileData = dstTileData;
        _mix = mix;
        _maskInvert = maskInvert;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        ImagePrivate::applyMaskMixCPU((const void**)_srcTileData.ptrs, _srcTileData.tileBounds, _srcTileData.nComps, (const void**)_maskTileData.ptrs, _maskTileData.tileBounds, _dstTileData.ptrs, _dstTileData.bitDepth, _dstTileData.nComps, _mix, _maskInvert, _dstTileData.tileBounds, renderWindow, renderArgs);
        return eActionStatusOK;
    }
};

void
Image::applyMaskMix(const RectI& roi,
                    const ImagePtr& maskImg,
                    const ImagePtr& originalImg,
                    bool masked,
                    bool maskInvert,
                    float mix)
{
    // !masked && mix == 1: nothing to do
    if ( !masked && (mix == 1) ) {
        return;
    }

    // Mask must be alpha
    assert( !masked || !maskImg || maskImg->getLayer().getNumComponents() == 1 );

    if (getStorageMode() == eStorageModeGLTex) {

        GLImageStoragePtr originalImageTexture, maskTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLImageStorage(originalImg->_imp->tiles[0].perChannelTile[0].buffer);
        }
        if (maskImg && masked) {
            assert(maskImg->getStorageMode() == eStorageModeGLTex);
            maskTexture = toGLImageStorage(maskImg->_imp->tiles[0].perChannelTile[0].buffer);
        }
        dstTexture = toGLImageStorage(_imp->tiles[0].perChannelTile[0].buffer);
        ImagePrivate::applyMaskMixGL(originalImageTexture, maskTexture, dstTexture, mix, maskInvert, roi);
        return;
    }

    // This function only works if original image and mask image are in full rect formats with the same bitdepth as output
    assert(!originalImg || (originalImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && originalImg->getBitDepth() == getBitDepth()));
    assert(!maskImg || (maskImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && maskImg->getBitDepth() == getBitDepth()));

    Image::CPUTileData srcImgData, maskImgData;
    if (originalImg) {
        getCPUTileData(originalImg->_imp->tiles[0], &srcImgData);
    }

    if (maskImg) {
        getCPUTileData(maskImg->_imp->tiles[0], &maskImgData);
        assert(maskImgData.nComps == 1);
    }


    for (std::size_t tile_i = 0; tile_i < _imp->tiles.size(); ++tile_i) {

        Image::CPUTileData dstImgData;
        getCPUTileData(_imp->tiles[tile_i], &dstImgData);

        RectI tileRoI;
        roi.intersect(dstImgData.tileBounds, &tileRoI);

        MaskMixProcessor processor(_imp->renderArgs);
        processor.setValues(srcImgData, maskImgData, dstImgData, mix, maskInvert);
        processor.setRenderWindow(tileRoI);
        processor.process();

    } // for each tile
} // applyMaskMix

bool
Image::canCallCopyUnProcessedChannels(const std::bitset<4> processChannels) const
{
    int numComp = getLayer().getNumComponents();

    if (numComp == 0) {
        return false;
    }
    if ( (numComp == 1) && processChannels[3] ) { // 1 component is alpha
        return false;
    } else if ( (numComp == 2) && processChannels[0] && processChannels[1] ) {
        return false;
    } else if ( (numComp == 3) && processChannels[0] && processChannels[1] && processChannels[2] ) {
        return false;
    } else if ( (numComp == 4) && processChannels[0] && processChannels[1] && processChannels[2] && processChannels[3] ) {
        return false;
    }

    return true;
}


class CopyUnProcessedProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUTileData _srcImgData, _dstImgData;
    std::bitset<4> _processChannels;
public:

    CopyUnProcessedProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~CopyUnProcessedProcessor()
    {
    }

    void setValues(const Image::CPUTileData& srcImgData,
                   const Image::CPUTileData& dstImgData,
                   std::bitset<4> processChannels)
    {
        _srcImgData = srcImgData;
        _dstImgData = dstImgData;
        _processChannels = processChannels;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        ImagePrivate::copyUnprocessedChannelsCPU((const void**)_srcImgData.ptrs, _srcImgData.tileBounds, _srcImgData.nComps, (void**)_dstImgData.ptrs, _dstImgData.bitDepth, _dstImgData.nComps, _dstImgData.tileBounds, _processChannels, renderWindow, renderArgs);
        return eActionStatusOK;
    }
};


void
Image::copyUnProcessedChannels(const RectI& roi,
                               const std::bitset<4> processChannels,
                               const ImagePtr& originalImg)
{

    if (!canCallCopyUnProcessedChannels(processChannels)) {
        return;
    }

    if (getStorageMode() == eStorageModeGLTex) {

        GLImageStoragePtr originalImageTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLImageStorage(originalImg->_imp->tiles[0].perChannelTile[0].buffer);
        }

        dstTexture = toGLImageStorage(_imp->tiles[0].perChannelTile[0].buffer);

        RectI realRoi;
        roi.intersect(dstTexture->getBounds(), &realRoi);
        ImagePrivate::copyUnprocessedChannelsGL(originalImageTexture, dstTexture, processChannels, realRoi);
        return;
    }

    // This function only works if original  image is in full rect format with the same bitdepth as output
    assert(!originalImg || (originalImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && originalImg->getBitDepth() == getBitDepth()));

    Image::CPUTileData srcImgData;
    if (originalImg) {
        getCPUTileData(originalImg->_imp->tiles[0], &srcImgData);
    }


    for (std::size_t tile_i = 0; tile_i < _imp->tiles.size(); ++tile_i) {

        Image::CPUTileData dstImgData;
        getCPUTileData(_imp->tiles[tile_i], &dstImgData);

        RectI tileRoI;
        roi.intersect(dstImgData.tileBounds, &tileRoI);

        CopyUnProcessedProcessor processor(_imp->renderArgs);
        processor.setValues(srcImgData, dstImgData, processChannels);
        processor.setRenderWindow(tileRoI);
        processor.process();

    } // for each tile
    
} // copyUnProcessedChannels

NATRON_NAMESPACE_EXIT;
