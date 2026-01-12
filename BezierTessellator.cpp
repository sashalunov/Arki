#include "BezierTessellator.h"

drawVert_t BezierTessellator::InterpolateD(const drawVert_t& p0, const drawVert_t& p1, const drawVert_t& p2, float t)
{
    drawVert_t out;
    float a = (1.0f - t) * (1.0f - t);
    float b = 2.0f * (1.0f - t) * t;
    float c = t * t;

    // 1. Position
    out.xyz[0] = a * p0.xyz[0] + b * p1.xyz[0] + c * p2.xyz[0];
    out.xyz[1] = a * p0.xyz[1] + b * p1.xyz[1] + c * p2.xyz[1];
    out.xyz[2] = a * p0.xyz[2] + b * p1.xyz[2] + c * p2.xyz[2];

    // 2. Texture Coords (UV)
    out.st[0] = a * p0.st[0] + b * p1.st[0] + c * p2.st[0];
    out.st[1] = a * p0.st[1] + b * p1.st[1] + c * p2.st[1];

    // 3. Lightmap UV
    out.lightmap[0] = a * p0.lightmap[0] + b * p1.lightmap[0] + c * p2.lightmap[0];
    out.lightmap[1] = a * p0.lightmap[1] + b * p1.lightmap[1] + c * p2.lightmap[1];

    // 4. Normals
    out.normal[0] = a * p0.normal[0] + b * p1.normal[0] + c * p2.normal[0];
    out.normal[1] = a * p0.normal[1] + b * p1.normal[1] + c * p2.normal[1];
    out.normal[2] = a * p0.normal[2] + b * p1.normal[2] + c * p2.normal[2];
    D3DXVec3Normalize(&out.normal, &out.normal);

    // 5. Colors (Linear interpolation approximation)
    for (int i = 0; i < 4; i++) 
    {
        out.color[i] = (BYTE)(a * p0.color[i] + b * p1.color[i] + c * p2.color[i]);
    }

    return out;
}
Q3BSPVertex BezierTessellator::Interpolate(const drawVert_t& p0, const drawVert_t& p1, const drawVert_t& p2, float t)
{
    Q3BSPVertex out;

    // 1. Calculate Basis Functions (Quadratic Bezier)
    // Formula: (1-t)^2 * P0 + 2(1-t)t * P1 + t^2 * P2
    float a = (1.0f - t) * (1.0f - t);
    float b = 2.0f * (1.0f - t) * t;
    float c = t * t;

    // 2. Interpolate Position (XYZ)
    out.pos.x = a * p0.xyz[0] + b * p1.xyz[0] + c * p2.xyz[0];
    out.pos.y = a * p0.xyz[1] + b * p1.xyz[1] + c * p2.xyz[1];
    out.pos.z = a * p0.xyz[2] + b * p1.xyz[2] + c * p2.xyz[2];

    // 3. Interpolate Normal
    out.normal.x = a * p0.normal[0] + b * p1.normal[0] + c * p2.normal[0];
    out.normal.y = a * p0.normal[1] + b * p1.normal[1] + c * p2.normal[1];
    out.normal.z = a * p0.normal[2] + b * p1.normal[2] + c * p2.normal[2];
    D3DXVec3Normalize(&out.normal, &out.normal);

    // 4. Interpolate Texture Coordinates (st / uv0)
    out.uv0[0] = a * p0.st[0] + b * p1.st[0] + c * p2.st[0];
    out.uv0[1] = a * p0.st[1] + b * p1.st[1] + c * p2.st[1];

    // 5. Interpolate Lightmap Coordinates (lightmap / uv1)
    out.uv1[0] = a * p0.lightmap[0] + b * p1.lightmap[0] + c * p2.lightmap[0];
    out.uv1[1] = a * p0.lightmap[1] + b * p1.lightmap[1] + c * p2.lightmap[1];

    // 6. Interpolate Color (RGBA Bytes -> DWORD)
    // We must interpolate components separately as floats to avoid overflow/underflow, then cast back.
    BYTE red = (BYTE)(a * p0.color[0] + b * p1.color[0] + c * p2.color[0]);
    BYTE green = (BYTE)(a * p0.color[1] + b * p1.color[1] + c * p2.color[1]);
    BYTE blue = (BYTE)(a * p0.color[2] + b * p1.color[2] + c * p2.color[2]);
    BYTE alpha = (BYTE)(a * p0.color[3] + b * p1.color[3] + c * p2.color[3]);

    // Pack into D3D compatible DWORD (ARGB usually, D3DCOLOR_RGBA handles the shift)
    out.color = D3DCOLOR_RGBA(red, green, blue, alpha);

    return out;
}
void BezierTessellator::TessellatePatch(const drawVert_t* controls, int cpWidth, int cpHeight, int level)
{
    // Q3 patches are grids of (width * height) control points.
    // However, they are composed of fused 3x3 bezier surfaces.
    // Example: A 3x5 patch is actually TWO 3x3 patches sharing the middle row.
    // Step size is 2.

    m_outVerts.clear();
    m_outIndices.clear();

    // Loop through the patch in 3x3 blocks
    for (int i = 0; i < cpWidth - 1; i += 2) {
        for (int j = 0; j < cpHeight - 1; j += 2) {

            drawVert_t chunk[9];

            // Extract the 9 control points for this sub-patch
            for (int x = 0; x < 3; x++) {
                for (int y = 0; y < 3; y++) {
                    int index = (j + y) * cpWidth + (i + x);
                    chunk[y * 3 + x] = controls[index];
                }
            }
            Tessellate3x3(chunk, level);
        }
    }
}

void BezierTessellator::Tessellate3x3(const drawVert_t cp[9], int level)
{
    int startVertIndex = (int)m_outVerts.size();
    int L = level;

    // 1. Generate Vertices
    // We compute rows, then columns (Bi-quadratic)
    for (int i = 0; i <= L; ++i)
    {
        float a = (float)i / L;

        // Calculate 3 points along the vertical columns
        drawVert_t temp[3];
        temp[0] = InterpolateD(cp[0], cp[3], cp[6], a);
        temp[1] = InterpolateD(cp[1], cp[4], cp[7], a);
        temp[2] = InterpolateD(cp[2], cp[5], cp[8], a);

        // Now interpolate across those 3 to get the row
        for (int j = 0; j <= L; ++j)
        {
            float b = (float)j / L;
            m_outVerts.push_back(Interpolate(temp[0], temp[1], temp[2], b));
        }
    }

    // 2. Generate Indices (Triangle List)
    // Grid size is (L+1) * (L+1)
    int rowLen = L + 1;

    for (int i = 0; i < L; ++i)
    {
        for (int j = 0; j < L; ++j)
        {
            int v0 = startVertIndex + i * rowLen + j;
            int v1 = startVertIndex + (i + 1) * rowLen + j;
            int v2 = startVertIndex + i * rowLen + (j + 1);
            int v3 = startVertIndex + (i + 1) * rowLen + (j + 1);

            // Triangle 1
            m_outIndices.push_back(v0);
            m_outIndices.push_back(v1); // Q3 Winding might need swap depending on cull mode
            m_outIndices.push_back(v2);

            // Triangle 2
            m_outIndices.push_back(v1);
            m_outIndices.push_back(v3);
            m_outIndices.push_back(v2);
        }
    }
}