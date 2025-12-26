#pragma once

// Helper function to interpolate between two values
inline  BYTE LerpByte(BYTE a, BYTE b, float t) {
    return (BYTE)(a + (b - a) * t);
}

// Helper to interpolate two D3DCOLORs
inline  D3DCOLOR InterpolateColor(D3DCOLOR c1, D3DCOLOR c2, float t) {
    // Extract ARGB components from the first color
    BYTE a1 = (c1 >> 24) & 0xFF;
    BYTE r1 = (c1 >> 16) & 0xFF;
    BYTE g1 = (c1 >> 8) & 0xFF;
    BYTE b1 = c1 & 0xFF;

    // Extract ARGB components from the second color
    BYTE a2 = (c2 >> 24) & 0xFF;
    BYTE r2 = (c2 >> 16) & 0xFF;
    BYTE g2 = (c2 >> 8) & 0xFF;
    BYTE b2 = c2 & 0xFF;

    // Interpolate each component
    BYTE a = LerpByte(a1, a2, t);
    BYTE r = LerpByte(r1, r2, t);
    BYTE g = LerpByte(g1, g2, t);
    BYTE b = LerpByte(b1, b2, t);

    // Reconstruct the color
    return D3DCOLOR_ARGB(a, r, g, b);
}

// Main function to generate the texture
inline  HRESULT CreateHorizontalGradientTexture(
    IDirect3DDevice9* pDevice,
    UINT width,
    UINT height,
    D3DCOLOR startColor,
    D3DCOLOR endColor,
    IDirect3DTexture9** ppTexture)
{
    if (!pDevice || !ppTexture) return E_INVALIDARG;

    // 1. Create the texture
    // Usage: 0 (default), Format: A8R8G8B8 (Standard 32-bit), Pool: Managed (Handled by DX)
    HRESULT hr = pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    // 2. Lock the texture to access memory
    D3DLOCKED_RECT lockedRect;
    // Lock the entire surface (0 as the rect pointer)
    hr = (*ppTexture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*ppTexture)->Release();
        return hr;
    }

    // 3. Write pixel data
    // cast the bits to BYTE* for easy pointer arithmetic
    BYTE* pBits = (BYTE*)lockedRect.pBits;
    // We cast to DWORD* to write 4 bytes (one pixel) at a time
    // NOTE: lockedRect.Pitch is the width of a row in BYTES, not pixels.
    for (UINT y = 0; y < height; ++y) {
        // Get the pointer to the start of the current row
        DWORD* pRow = (DWORD*)(pBits + y * lockedRect.Pitch);

        for (UINT x = 0; x < width; ++x) {
            // Calculate horizontal progress (0.0 to 1.0)
            float t = (float)x / (float)(width - 1);

            // Calculate and set the color
            pRow[x] = InterpolateColor(startColor, endColor, t);
        }
    }
    // 4. Unlock the texture
    (*ppTexture)->UnlockRect(0);

    return S_OK;
}


inline  HRESULT CreateRadialGradientTexture(
    IDirect3DDevice9* pDevice,
    UINT width,
    UINT height,
    D3DCOLOR innerColor,
    D3DCOLOR outerColor,
    IDirect3DTexture9** ppTexture)
{
    if (!pDevice || !ppTexture) return E_INVALIDARG;

    HRESULT hr = pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture, NULL);
    if (FAILED(hr)) return hr;

    D3DLOCKED_RECT lockedRect;
    hr = (*ppTexture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*ppTexture)->Release();
        return hr;
    }

    BYTE* pBits = (BYTE*)lockedRect.pBits;

    // Define center point
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;

    // Define max radius (distance from center to edge). 
    // Using min(width, height) ensures a perfect circle fits.
    float maxRadius = (std::min(width, height) / 2.0f);

    for (UINT y = 0; y < height; ++y) {
        DWORD* pRow = (DWORD*)(pBits + y * lockedRect.Pitch);

        for (UINT x = 0; x < width; ++x) {
            // Calculate distance from center
            float dx = x - centerX;
            float dy = y - centerY;
            float distance = sqrtf(dx * dx + dy * dy);

            // Normalize distance to range 0.0 - 1.0
            float t = distance / maxRadius;

            // Clamp t to 1.0 to ensure corners don't "wrap around" or glitch color
            if (t > 1.0f) t = 1.0f;

            pRow[x] = InterpolateColor(innerColor, outerColor, t);
        }
    }

    (*ppTexture)->UnlockRect(0);
    return S_OK;
}


inline  HRESULT CreateVerticalGradientTexture(
    IDirect3DDevice9* pDevice,
    UINT width,
    UINT height,
    D3DCOLOR topColor,
    D3DCOLOR bottomColor,
    IDirect3DTexture9** ppTexture)
{
    if (!pDevice || !ppTexture) return E_INVALIDARG;

    HRESULT hr = pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture, NULL);
    if (FAILED(hr)) return hr;

    D3DLOCKED_RECT lockedRect;
    hr = (*ppTexture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*ppTexture)->Release();
        return hr;
    }

    BYTE* pBits = (BYTE*)lockedRect.pBits;

    for (UINT y = 0; y < height; ++y) {
        // Calculate vertical progress (0.0 at top, 1.0 at bottom)
        float t = (float)y / (float)(height - 1);

        // Calculate the color for the entire row once, since it doesn't change horizontally
        D3DCOLOR rowColor = InterpolateColor(topColor, bottomColor, t);

        // Get pointer to the start of the row
        DWORD* pRow = (DWORD*)(pBits + y * lockedRect.Pitch);

        for (UINT x = 0; x < width; ++x) {
            pRow[x] = rowColor;
        }
    }

    (*ppTexture)->UnlockRect(0);
    return S_OK;
}

inline  HRESULT CreateCheckeredTexture(
    IDirect3DDevice9* pDevice,
    UINT width,
    UINT height,
    D3DCOLOR color1,
    D3DCOLOR color2,
    UINT tileSize,      // Size of one square in pixels
    IDirect3DTexture9** ppTexture)
{
    if (!pDevice || !ppTexture || tileSize == 0) return E_INVALIDARG;

    // 1. Create the texture
    HRESULT hr = pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture, NULL);
    if (FAILED(hr)) return hr;

    // 2. Lock the texture
    D3DLOCKED_RECT lockedRect;
    hr = (*ppTexture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*ppTexture)->Release();
        return hr;
    }

    BYTE* pBits = (BYTE*)lockedRect.pBits;

    // 3. Write pixel data
    for (UINT y = 0; y < height; ++y) {
        DWORD* pRow = (DWORD*)(pBits + y * lockedRect.Pitch);

        for (UINT x = 0; x < width; ++x) {
            // Determine the coordinates of the tile this pixel belongs to
            UINT tileX = x / tileSize;
            UINT tileY = y / tileSize;

            // Decision logic: Is (tileX + tileY) Even or Odd?
            // Even = Color1, Odd = Color2
            if ((tileX + tileY) % 2 == 0) {
                pRow[x] = color1;
            }
            else {
                pRow[x] = color2;
            }
        }
    }

    // 4. Unlock
    (*ppTexture)->UnlockRect(0);

    return S_OK;
}