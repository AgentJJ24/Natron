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

#include "RotoBezierTriangulation.h"

#include <cmath>
#include <boost/cstdint.hpp> // uintptr_t
#include <cstddef> // size_t

#include "libtess.h"

NATRON_NAMESPACE_ENTER;

NATRON_NAMESPACE_ANONYMOUS_ENTER;

using boost::uintptr_t;
using std::size_t;
using std::vector;

static void tess_begin_primitive_callback(unsigned int which,
                                          void *polygonData)
{

    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    switch (which) {
        case LIBTESS_GL_TRIANGLE_STRIP:
            assert(!myData->stripsBeingEdited);
            myData->stripsBeingEdited.reset(new RotoBezierTriangulation::RotoTriangleStrips);
            break;
        case LIBTESS_GL_TRIANGLE_FAN:
            assert(!myData->fanBeingEdited);
            myData->fanBeingEdited.reset(new RotoBezierTriangulation::RotoTriangleFans);
            break;
        case LIBTESS_GL_TRIANGLES:
            assert(!myData->trianglesBeingEdited);
            myData->trianglesBeingEdited.reset(new RotoBezierTriangulation::RotoTriangles);
            break;
        default:
            assert(false);
            break;
    }
}

static void tess_end_primitive_callback(void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    if (myData->stripsBeingEdited) {
        myData->internalStrips.push_back(*myData->stripsBeingEdited);
        myData->stripsBeingEdited.reset();
    } else if (myData->fanBeingEdited) {
        myData->internalFans.push_back(*myData->fanBeingEdited);
        myData->fanBeingEdited.reset();
    } else if (myData->trianglesBeingEdited) {
        myData->internalTriangles.push_back(*myData->trianglesBeingEdited);
        myData->trianglesBeingEdited.reset();
    }

}

static void tess_vertex_callback(void* data /*per-vertex client data*/,
                                 void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    uintptr_t index = reinterpret_cast<uintptr_t>(data);
    assert(/**p >= 0 &&*/ index < myData->bezierPolygonJoined.size());
#ifndef NDEBUG
    assert(myData->bezierPolygonJoined[index].x >= myData->bezierBbox.x1 && myData->bezierPolygonJoined[index].x <= myData->bezierBbox.x2 &&
           myData->bezierPolygonJoined[index].y >= myData->bezierBbox.y1 && myData->bezierPolygonJoined[index].y <= myData->bezierBbox.y2);
#endif
    if (myData->stripsBeingEdited) {
        myData->stripsBeingEdited->indices.push_back(index);
    } else if (myData->fanBeingEdited) {
        myData->fanBeingEdited->indices.push_back(index);
    } else if (myData->trianglesBeingEdited) {
        myData->trianglesBeingEdited->indices.push_back(index);
    }

}

static void tess_error_callback(unsigned int error,
                                void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);
    myData->error = error;
}

static void tess_intersection_combine_callback(double coords[3],
                                               void */*data*/[4] /*4 original vertices*/,
                                               double /*w*/[4] /*weights*/,
                                               void **dataOut,
                                               void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    ParametricPoint v;
    v.x = coords[0];
    v.y = coords[1];
    v.t = 0;

    uintptr_t index = myData->bezierPolygonJoined.size();
    myData->bezierPolygonJoined.push_back(v);

    assert(index < myData->bezierPolygonJoined.size());
    *dataOut = reinterpret_cast<void*>(index);
}

NATRON_NAMESPACE_ANONYMOUS_EXIT;

void
RotoBezierTriangulation::computeTriangles(const BezierPtr& bezier,
                                          TimeValue time,
                                          ViewIdx view,
                                          const RenderScale& scale,
                                          double featherDistPixel_x,
                                          double featherDistPixel_y,
                                            PolygonData* outArgs)
{
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.

    assert(outArgs);
    outArgs->error = 0;

    bool clockWise = bezier->isFeatherPolygonClockwiseOriented(time, view);

    const double absFeatherDist_x = std::abs(featherDistPixel_x);
    const double absFeatherDist_y = std::abs(featherDistPixel_y);

    RectD featherPolyBBox;
    featherPolyBBox.setupInfinity();

#ifdef ROTO_BEZIER_EVAL_ITERATIVE
    int error = -1;
#else
    double error = 1;
#endif

    bezier->evaluateFeatherPointsAtTime_DeCasteljau(time, view, scale, error, true, &outArgs->featherPolygon, &featherPolyBBox);
    bezier->evaluateAtTime_DeCasteljau(time, view, scale, error,&outArgs->bezierPolygon,
#ifndef NDEBUG
                                       &outArgs->bezierBbox
#else
                                       0
#endif
                                       );


    // First compute the mesh composed of triangles of the feather
    assert( !outArgs->featherPolygon.empty() && !outArgs->bezierPolygon.empty() && outArgs->featherPolygon.size() == outArgs->bezierPolygon.size());

    vector<vector<ParametricPoint> >::const_iterator fIt = outArgs->featherPolygon.begin();
    vector<vector<ParametricPoint> > ::const_iterator prevFSegmentIt = outArgs->featherPolygon.end();
    --prevFSegmentIt;
    vector<vector<ParametricPoint> > ::const_iterator nextFSegmentIt = outArgs->featherPolygon.begin();
    ++nextFSegmentIt;
    for (vector<vector<ParametricPoint> > ::const_iterator it = outArgs->bezierPolygon.begin(); it != outArgs->bezierPolygon.end(); ++it, ++fIt) {

        if (prevFSegmentIt == outArgs->featherPolygon.end()) {
            prevFSegmentIt = outArgs->featherPolygon.begin();
        }
        if (nextFSegmentIt == outArgs->featherPolygon.end()) {
            nextFSegmentIt = outArgs->featherPolygon.begin();
        }
        // Iterate over each bezier segment.
        // There are the same number of bezier segments for the feather and the internal bezier. Each discretized segment is a contour (list of vertices)

        vector<ParametricPoint>::const_iterator bSegmentIt = it->begin();
        vector<ParametricPoint>::const_iterator fSegmentIt = fIt->begin();

        assert(!it->empty() && !fIt->empty());


        // prepare iterators to compute derivatives for feather distance
        vector<ParametricPoint>::const_iterator fnext = fSegmentIt;
        ++fnext;  // can only be valid since we assert the list is not empty
        if ( fnext == fIt->end() ) {
            fnext = fIt->begin();
        }
        vector<ParametricPoint>::const_iterator fprev = prevFSegmentIt->end();
        --fprev; // can only be valid since we assert the list is not empty
        --fprev;


        // initialize the state with a segment between the first inner vertex and first outter vertex
        RotoFeatherVertex lastInnerVert,lastOutterVert;
        if ( bSegmentIt != it->end() ) {
            lastInnerVert.x = bSegmentIt->x;
            lastInnerVert.y = bSegmentIt->y;
            lastInnerVert.isInner = true;
            outArgs->featherMesh.push_back(lastInnerVert);
            ++bSegmentIt;
        }
        if ( fSegmentIt != fIt->end() ) {
            lastOutterVert.x = fSegmentIt->x;
            lastOutterVert.y = fSegmentIt->y;

            {
                double diffx = fnext->x - fprev->x;
                double diffy = fnext->y - fprev->y;
                double norm = std::sqrt( diffx * diffx + diffy * diffy );
                double dx = (norm != 0) ? -( diffy / norm ) : 0;
                double dy = (norm != 0) ? ( diffx / norm ) : 1;

                if (!clockWise) {
                    lastOutterVert.x -= dx * absFeatherDist_x;
                    lastOutterVert.y -= dy * absFeatherDist_y;
                } else {
                    lastOutterVert.x += dx * absFeatherDist_x;
                    lastOutterVert.y += dy * absFeatherDist_y;
                }
            }

            lastOutterVert.isInner = false;
            outArgs->featherMesh.push_back(lastOutterVert);
            ++fSegmentIt;
        }

        if ( fprev != prevFSegmentIt->end() ) {
            ++fprev;
        }
        if ( fnext != fIt->end() ) {
            ++fnext;
        }


        for (;;) {

            if ( fnext == fIt->end() ) {
                fnext = nextFSegmentIt->begin();
                ++fnext;
            }
            if ( fprev == prevFSegmentIt->end() ) {
                fprev = fIt->begin();
                ++fprev;
            }

            double inner_t = (double)INT_MAX;
            double outter_t = (double)INT_MAX;
            bool gotOne = false;
            if (bSegmentIt != it->end()) {
                inner_t = bSegmentIt->t;
                gotOne = true;
            }
            if (fSegmentIt != fIt->end()) {
                outter_t = fSegmentIt->t;
                gotOne = true;
            }

            if (!gotOne) {
                break;
            }

            // Pick the point with the minimum t
            if (inner_t <= outter_t) {
                if ( bSegmentIt != fIt->end() ) {
                    lastInnerVert.x = bSegmentIt->x;
                    lastInnerVert.y = bSegmentIt->y;
                    lastInnerVert.isInner = true;
                    outArgs->featherMesh.push_back(lastInnerVert);
                    ++bSegmentIt;
                }
            } else {
                if ( fSegmentIt != fIt->end() ) {
                    lastOutterVert.x = fSegmentIt->x;
                    lastOutterVert.y = fSegmentIt->y;

                    {
                        double diffx = fnext->x - fprev->x;
                        double diffy = fnext->y - fprev->y;
                        double norm = std::sqrt( diffx * diffx + diffy * diffy );
                        double dx = (norm != 0) ? -( diffy / norm ) : 0;
                        double dy = (norm != 0) ? ( diffx / norm ) : 1;

                        if (!clockWise) {
                            lastOutterVert.x -= dx * absFeatherDist_x;
                            lastOutterVert.y -= dy * absFeatherDist_y;
                        } else {
                            lastOutterVert.x += dx * absFeatherDist_x;
                            lastOutterVert.y += dy * absFeatherDist_y;
                        }
                    }
                    lastOutterVert.isInner = false;
                    outArgs->featherMesh.push_back(lastOutterVert);
                    ++fSegmentIt;
                }
                
                if ( fprev != fIt->end() ) {
                    ++fprev;
                }
                if ( fnext != fIt->end() ) {
                    ++fnext;
                }
            }

            // Initialize the first segment of the next triangle if we did not reach the end
            if (fSegmentIt == fIt->end() && bSegmentIt == it->end()) {
                break;
            }
            outArgs->featherMesh.push_back(lastOutterVert);
            outArgs->featherMesh.push_back(lastInnerVert);


        } // for(;;)
        if (prevFSegmentIt != outArgs->featherPolygon.end()) {
            ++prevFSegmentIt;
        }
        if (nextFSegmentIt != outArgs->featherPolygon.end()) {
            ++nextFSegmentIt;
        }
    }



    // Now tesselate the internal bezier using glu
    libtess_GLUtesselator* tesselator = libtess_gluNewTess();

    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())tess_begin_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())tess_vertex_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_END_DATA, (void (*)())tess_end_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())tess_error_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())tess_intersection_combine_callback);

    // From README: if you know that all polygons lie in the x-y plane, call
    // gluTessNormal(tess, 0.0, 0.0, 1.0) before rendering any polygons.
    libtess_gluTessNormal(tesselator, 0, 0, 1);
    libtess_gluTessBeginPolygon(tesselator, outArgs);
    libtess_gluTessBeginContour(tesselator);

    // Join the internal polygon into a single vector of vertices now that we don't need per-bezier segments separation.
    // we will need the indices for libtess
    for (vector<vector<ParametricPoint> >::const_iterator it = outArgs->bezierPolygon.begin(); it != outArgs->bezierPolygon.end(); ++it) {

        // don't add the first vertex which is the same as the last vertex of the last segment
        vector<ParametricPoint>::const_iterator start = it->begin();
        ++start;
        outArgs->bezierPolygonJoined.insert(outArgs->bezierPolygonJoined.end(), start, it->end());


    }
    outArgs->bezierPolygon.clear();


    for (size_t i = 0; i < outArgs->bezierPolygonJoined.size(); ++i) {
        double coords[3] = {outArgs->bezierPolygonJoined[i].x, outArgs->bezierPolygonJoined[i].y, 1.};
        uintptr_t index = i;
        assert(index < outArgs->bezierPolygonJoined.size());
        libtess_gluTessVertex(tesselator, coords, reinterpret_cast<void*>(index) /*per-vertex client data*/);
    }

    libtess_gluTessEndContour(tesselator);
    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);

    // check for errors
    assert(outArgs->error == 0);
    
} // RotoBezierTriangulation::computeTriangles

NATRON_NAMESPACE_EXIT;
