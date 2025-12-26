#include "CGrid.h"

#include "StdAfx.h"

CGrid::CGrid() : m_pVB(NULL), m_numVertices(0), m_numLines(0)
{
    // Default "Blender-like" Dark Theme Colors
    m_colMinor = D3DCOLOR_ARGB(255, 60, 60, 60);   // Subtle Grey
    m_colMajor = D3DCOLOR_ARGB(255, 20, 20, 20);   // Darker Black/Grey
    m_colXAxis = D3DCOLOR_ARGB(255, 180, 50, 50);  // Red
    m_colZAxis = D3DCOLOR_ARGB(255, 50, 50, 180);  // Blue

    m_gridSize = 50.0f;
    m_stepSize = 1.0f;
    m_subDiv = 10;
}

CGrid::~CGrid()
{
	SAFE_RELEASE(m_pVB);
}



void CGrid::SetColors(D3DCOLOR minor, D3DCOLOR major, D3DCOLOR xAxis, D3DCOLOR zAxis)
{
    m_colMinor = minor;
    m_colMajor = major;
    m_colXAxis = xAxis;
    m_colZAxis = zAxis;
}

void CGrid::Create(IDirect3DDevice9* device, float size, float step)
{
	m_device = device;

    SAFE_RELEASE(m_pVB); // Clear old buffer if exists

    m_gridSize = size;
    m_stepSize = step;

    std::vector<GridVertex> verts;

    // We build lines along X and lines along Z
    // Loop goes from -size to +size
    int lineIndex = 0; // Used to track major/minor lines

    // ---------------------------------------------------------
    // 1. Generate Lines Parallel to Z-Axis (varying X)
    // ---------------------------------------------------------
    for (float x = -m_gridSize; x <= m_gridSize + 0.001f; x += m_stepSize)
    {
        D3DCOLOR color = m_colMinor;

        // Check if this is the Center Axis
        if (fabs(x) < 0.001f)
            color = m_colZAxis; // The Z-Axis line runs along X=0

        // Check if this is a Major Subdivision (e.g., every 10 units)
        // We use an integer check to avoid float precision issues
        else if (lineIndex % m_subDiv == 0)
            color = m_colMajor;

        // Line Start
        GridVertex v1 = { x, 0.0f, -m_gridSize, color };
        // Line End
        GridVertex v2 = { x, 0.0f,  m_gridSize, color };

        verts.push_back(v1);
        verts.push_back(v2);

        lineIndex++;
    }

    // Reset index for the other direction so the pattern matches
    lineIndex = 0;

    // ---------------------------------------------------------
    // 2. Generate Lines Parallel to X-Axis (varying Z)
    // ---------------------------------------------------------
    for (float z = -m_gridSize; z <= m_gridSize + 0.001f; z += m_stepSize)
    {
        D3DCOLOR color = m_colMinor;

        if (fabs(z) < 0.001f)
            color = m_colXAxis; // The X-Axis line runs along Z=0
        else if (lineIndex % m_subDiv == 0)
            color = m_colMajor;

        GridVertex v1 = { -m_gridSize, 0.0f, z, color };
        GridVertex v2 = { m_gridSize, 0.0f, z, color };

        verts.push_back(v1);
        verts.push_back(v2);

        lineIndex++;
    }

    // ---------------------------------------------------------
    // 3. Create Vertex Buffer
    // ---------------------------------------------------------
    m_numVertices = (int)verts.size();
    m_numLines = m_numVertices / 2;

    if (FAILED(device->CreateVertexBuffer(
        m_numVertices * sizeof(GridVertex),
        D3DUSAGE_WRITEONLY,
        GridVertex::FVF,
        D3DPOOL_MANAGED,
        &m_pVB,
        NULL)))
    {
        // Handle Error (Log it)
        return;
    }

    // Copy data
    void* pData;
    if (SUCCEEDED(m_pVB->Lock(0, 0, &pData, 0)))
    {
        memcpy(pData, &verts[0], m_numVertices * sizeof(GridVertex));
        m_pVB->Unlock();
    }
}

void CGrid::Render(IDirect3DDevice9* device)
{
    if (!m_pVB) return;

    // Disable lighting so the grid lines shine with their true colors
    device->SetRenderState(D3DRS_LIGHTING, FALSE);

    // Optional: Enable Alpha Blending if you want transparent grids
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    device->SetStreamSource(0, m_pVB, 0, sizeof(GridVertex));
    device->SetFVF(GridVertex::FVF);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha

    device->DrawPrimitive(D3DPT_LINELIST, 0, m_numLines);

    // Restore Lighting (Good practice)
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);

}