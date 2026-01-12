#pragma once
#include "stdafx.h"
#include "Q3BSPStructures.h"

class BezierTessellator
{
public:
    // Output containers
    //std::vector<drawVert_t> m_outVerts;
    std::vector<int>        m_outIndices;

    std::vector<Q3BSPVertex> m_outVerts;
    // level = Level of Detail (e.g., 5 to 10)
    void TessellatePatch(const drawVert_t* controls, int cpWidth, int cpHeight, int level);

private:
    // Interpolates a single vertex between 3 control points at value t (0.0 to 1.0)
    drawVert_t InterpolateD(const drawVert_t& p0, const drawVert_t& p1, const drawVert_t& p2, float t);
    Q3BSPVertex Interpolate(const drawVert_t& p0, const drawVert_t& p1, const drawVert_t& p2, float t);    // Helper to linear interpolate basic types
    float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    // Q3 uses 3x3 control point grids. We split large patches into 3x3 chunks.
    void Tessellate3x3(const drawVert_t cp[9], int level);
};