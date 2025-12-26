#pragma once
#include "stdafx.h"

// Custom vertex format for the grid (Position + Color)
struct GridVertex
{
    float x, y, z;
    D3DCOLOR color;

    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};

class CGrid
{
private:
    IDirect3DDevice9* m_device;
    IDirect3DVertexBuffer9* m_pVB;
    int m_numVertices;
    int m_numLines;

    // Grid Settings
    float m_gridSize;    // Total size (e.g., 100.0f means -100 to +100)
    float m_stepSize;    // Distance between lines (e.g., 1.0f)
    int   m_subDiv;      // How many "minor" lines make a "major" line (e.g., 10)

    // Colors
    D3DCOLOR m_colMinor; // Standard grid lines
    D3DCOLOR m_colMajor; // Every 10th line (darker/prominent)
    D3DCOLOR m_colXAxis; // X Axis color (Red)
    D3DCOLOR m_colZAxis; // Z Axis color (Blue)

public:
    CGrid();
    ~CGrid();

    // Initialize or Re-create the grid geometry
    // size: Extents of the grid (from -size to +size)
    // step: Size of a single square
    void Create(IDirect3DDevice9* device, float size = 50.0f, float step = 1.0f);


    // Render the grid to the screen
    void Render(IDirect3DDevice9* device);

    // --- Configuration Setters (Call Create() after changing these) ---
    void SetColors(D3DCOLOR minor, D3DCOLOR major, D3DCOLOR xAxis, D3DCOLOR zAxis);

    // Call this BEFORE device->Reset()
    void OnLostDevice()
    {
        SAFE_RELEASE(m_pVB);
    }

    // Call this AFTER device->Reset()
    void OnResetDevice()
    {
		Create(m_device, m_gridSize, m_stepSize);
    }
};

