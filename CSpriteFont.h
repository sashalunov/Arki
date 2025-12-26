#pragma once
#include "Logger.h"


// The vertex format for our 2D text
struct FontVertex2D {
    float x, y, z, rhw; // Screen position
    D3DCOLOR color;     // Color
    float u, v;         // Texture coordinates
};

#define FVF_FONT_VERTEX2D (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)


struct FontVertex3D {
    float x, y, z;      // REMOVED 'rhw'
    D3DCOLOR color;
    float u, v;
};
// Use D3DFVF_XYZ instead of D3DFVF_XYZRHW
#define FVF_FONT_VERTEX3D (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct CharDesc {
    int id;
    int x, y;
    int width, height;
    int xoffset, yoffset;
    int xadvance;
    int page;
};

class CSpriteFont {
private:
    IDirect3DTexture9* m_pTexture;
    std::vector<IDirect3DTexture9*> m_pages;
    int m_currentPage = -1;
    IDirect3DDevice9* m_pDev;

    int m_texWidth;  // Texture width from file
    int m_texHeight; // Texture height from file

    // Map of ASCII ID -> Character Data
    std::map<int, CharDesc> m_chars;

    // The batch buffer
    std::vector<FontVertex3D> m_batchVertices;

    // Helper to parse a single line from the .fnt file
    void ParseLine(const std::string& line);

public:
    CSpriteFont();
    ~CSpriteFont();

    // The Master Init Function (Generic)
    bool InitFromMemory(IDirect3DDevice9* pDevice,
        const void* pFontData, int fontSize,
        const void* pTextureData, int textureSize);

    // Initialize: path to .fnt file and .png texture
    bool Init(IDirect3DDevice9* pDevice, const TCHAR* fontFilePath);

    // Queue text to be drawn. Does NOT render immediately.
    void DrawString(float x, float y, const std::string& text, D3DCOLOR color);
    void DrawString3D(D3DXVECTOR3 pos, float scale, const std::string& text, D3DCOLOR color);

    // Flushes the batch and renders everything to the screen.
    // Call this before EndScene()
    void RenderBatch(IDirect3DDevice9* pDevice);

    // Cleanup
    void Shutdown();
};