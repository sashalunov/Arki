#pragma once
#include "Logger.h"
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

// Helper to save texture
inline static void SaveTexture(IDirect3DTexture9* pTexture, const TCHAR* filename)
{
    if (!pTexture) return;

    // D3DXIFF_PNG, D3DXIFF_BMP, D3DXIFF_JPG, etc.
    HRESULT hr = D3DXSaveTextureToFile(
        filename,       // File path
        D3DXIFF_PNG,    // Format
        pTexture,       // The texture interface
        nullptr         // Palette (null for standard RGB)
    );

    if (FAILED(hr)) {
        _log(L"Saving failed %s", filename);
    }
}


#include <d3dx9.h>
#include <cmath>
#include <algorithm>

class CTextureGenerator
{
public:
    static IDirect3DTexture9* CreateCyberPanelTexture(IDirect3DDevice9* device, int width, int height)
    {
        IDirect3DTexture9* pTexture = nullptr;
        if (FAILED(D3DXCreateTexture(device, width, height, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pTexture)))
            return nullptr;

        D3DLOCKED_RECT rect;
        pTexture->LockRect(0, &rect, nullptr, 0);
        DWORD* pData = (DWORD*)rect.pBits;

        // CONFIGURATION
        int panelSize = 64; // Size of one big plate (must divide width/height)
        //int subPanelSize = 16; // Small details

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                // 1. GRID COORDINATES
                // Which panel are we in?
                int pX = x / panelSize;
                int pY = y / panelSize;

                // Local coordinates inside the panel (0 to panelSize-1)
                int localX = x % panelSize;
                int localY = y % panelSize;

                // Normalized local coordinates (0.0 to 1.0)
                float u = (float)localX / panelSize;
                float v = (float)localY / panelSize;

                // 2. RANDOMIZE PANEL TYPE
                // We use the panel coordinates to pick a "random" but constant type for this cell
                float randVal = PseudoRandom(pX, pY); // Returns 0.0 to 1.0

                // Base Colors (Sci-fi Blue/Grey Scheme)
                int r = 40, g = 45, b = 55; // Dark Metal

                // --- GEOMETRY GENERATION ---

                // A. SEAMS (The black gaps between panels)
                int seamWidth = 2;
                bool isSeam = (localX < seamWidth || localX >= panelSize - seamWidth ||
                    localY < seamWidth || localY >= panelSize - seamWidth);

                if (isSeam)
                {
                    r = 10; g = 10; b = 15; // Almost black
                }
                else
                {
                    // B. BEVELS (3D Edge Highlight)
                    // Top/Left = Light, Bottom/Right = Dark
                    if (localX < 4 || localY < 4) {
                        r += 40; g += 40; b += 50; // Highlight
                    }
                    else if (localX > panelSize - 5 || localY > panelSize - 5) {
                        r -= 20; g -= 20; b -= 20; // Shadow
                    }

                    // C. INNER DETAILS (Greebles) based on Random Value

                    // VARIATION 1: "VENTS" (Horizontal Lines)
                    if (randVal > 0.7f)
                    {
                        // Create stripes every 8 pixels
                        if ((localY % 8) < 3 && localX > 8 && localX < panelSize - 8) {
                            r = 20; g = 20; b = 25; // Dark Vent Slot
                        }
                    }
                    // VARIATION 2: "TECH GLOW" (Glowing Sub-panel)
                    else if (randVal < 0.2f)
                    {
                        // A small glowing box in the center
                        //if (u > 0.3f && u < 0.7f && v > 0.3f && v < 0.7f) {
                        //    r = 0; g = 200; b = 255; // Cyan Glow
                       // }
                        // A small red light in the corner
                        if (u > 0.8f && v > 0.8f) {
                            r = 255; g = 50; b = 0; // Red LED
                        }
                    }
                    else if (randVal < 0.1f)
                    {
                        // A small glowing box in the center
                        if (u > 0.3f && u < 0.7f && v > 0.3f && v < 0.7f) {
                            r = 0; g = 200; b = 255; // Cyan Glow
                        }
                        // A small red light in the corner
                        //if (u > 0.8f && v > 0.8f) {
                        //    r = 255; g = 50; b = 0; // Red LED
                        //}
                    }
                    // VARIATION 3: "BOLTS" (Rivets in corners)
                    else
                    {
                        // Simple noise grain for metal texture
                        float grain = PseudoRandom(x, y) * 20.0f;
                        r += (int)grain; g += (int)grain; b += (int)grain;

                        // Bolts at (10,10) and (54,54) etc
                        bool isBolt = (abs(localX - 10) < 2 && abs(localY - 10) < 2) ||
                            (abs(localX - 54) < 2 && abs(localY - 10) < 2) ||
                            (abs(localX - 10) < 2 && abs(localY - 54) < 2) ||
                            (abs(localX - 54) < 2 && abs(localY - 54) < 2);
                        if (isBolt) {
                            r = 150; g = 150; b = 160; // Silver Bolt
                        }
                    }
                }

                // Clamp
                r = std::min(255, std::max(0, r));
                g = std::min(255, std::max(0, g));
                b = std::min(255, std::max(0, b));

                // Write Pixel
                int index = (y * (rect.Pitch / 4)) + x;
                pData[index] = D3DCOLOR_XRGB(r, g, b);
            }
        }

        pTexture->UnlockRect(0);
        return pTexture;
    }

    static IDirect3DTexture9* CreateRockTexture(IDirect3DDevice9* device, int width, int height)
    {
        IDirect3DTexture9* pTexture = nullptr;
        if (FAILED(D3DXCreateTexture(device, width, height, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pTexture)))
            return nullptr;

        D3DLOCKED_RECT rect;
        pTexture->LockRect(0, &rect, nullptr, 0);
        DWORD* pData = (DWORD*)rect.pBits;

        // Base Scale (Higher = more zoomed out)
        float scale = 4.0f;

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                float u = (float)x / width;
                float v = (float)y / height;

                float noise = 0.0f;
                // Layer 1: Large Shapes (Base structure)
                noise += GetSeamlessNoise(u * scale, v * scale, scale) * 1.0f;

                // Layer 2: Medium Details (2x freq, 0.5x strength)
                noise += GetSeamlessNoise(u * scale * 2, v * scale * 2, scale * 2) * 0.5f;

                // Layer 3: Small Grit
                noise += GetSeamlessNoise(u * scale * 4, v * scale * 4, scale * 4) * 0.25f;

                // Layer 4: Fine Texture
                noise += GetSeamlessNoise(u * scale * 8, v * scale * 8, scale * 8) * 0.125f;

                noise /= 1.875f; // Normalize

                // --- COLORING ---
                // Add contrast: Darken the lows to make "cracks"
               // if (noise < 0.4f) noise *= noise;

                // Rock Color Logic
                int r1 = 50, g1 = 45, b1 = 40;
                int r2 = 160, g2 = 155, b2 = 150;

                // Simple contrast curve
                if (noise < 0.2f) noise *= noise;

                int r = (int)(r1 + (r2 - r1) * noise);
                int g = (int)(g1 + (g2 - g1) * noise);
                int b = (int)(b1 + (b2 - b1) * noise);

                // Clamp
                r = std::min(255, std::max(0, r));
                g = std::min(255, std::max(0, g));
                b = std::min(255, std::max(0, b));

                int index = (y * (rect.Pitch / 4)) + x;
                pData[index] = D3DCOLOR_XRGB(r, g, b);
            }
        }
        pTexture->UnlockRect(0);
        return pTexture;
    }

private:
    // WRAPPING NOISE FUNCTION
    // wrapSize: How many cells wide/tall the grid is. 
    // If x reaches wrapSize, it wraps back to 0.
    static float GetSeamlessNoise(float x, float y, float wrapSize)
    {
        int ix = (int)floor(x);
        int iy = (int)floor(y);

        float fx = x - ix;
        float fy = y - iy;

        // Smoothstep
        float ux = fx * fx * (3.0f - 2.0f * fx);
        float uy = fy * fy * (3.0f - 2.0f * fy);

        // MODULO WRAPPING (The Magic Part)
        // We ensure indices stay within [0, wrapSize-1]
        int w = (int)wrapSize;

        // Safety check to prevent divide by zero
        if (w == 0) w = 1;

        int x0 = ix % w;
        int x1 = (ix + 1) % w;  // If ix is at the edge, x1 becomes 0!
        int y0 = iy % w;
        int y1 = (iy + 1) % w;

        // Sample random values at wrapped coordinates
        float v00 = PseudoRandom(x0, y0);
        float v10 = PseudoRandom(x1, y0);
        float v01 = PseudoRandom(x0, y1);
        float v11 = PseudoRandom(x1, y1);

        float top = Lerp(v00, v10, ux);
        float bot = Lerp(v01, v11, ux);
        return Lerp(top, bot, uy);
    }

    static float Lerp(float a, float b, float t) { return a + t * (b - a); }

    static float PseudoRandom(int x, int y)
    {
        int n = x + y * 57;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    }


};

