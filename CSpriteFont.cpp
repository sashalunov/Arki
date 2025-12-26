#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <iostream>

#include "CSpriteFont.h"

// Helper to extract value from string like "x=5"
int GetValue(const std::string& line, const std::string& key) {
    size_t keyPos = line.find(key + "=");
    if (keyPos == std::string::npos) return 0;

    size_t valPos = keyPos + key.length() + 1;
    return std::stoi(line.substr(valPos));
}

std::string GetValueS(const std::string& line, const std::string& key) {
    // 1. Find the key (e.g., "file")
    size_t keyPos = line.find(key + "=");
    if (keyPos == std::string::npos) return "";
    // 2. Find the opening quote after the key
    // We start searching from the end of "key="
    size_t startQuote = line.find("\"", keyPos + key.length() + 1);
    if (startQuote == std::string::npos) return "";
    // 3. Find the closing quote
    size_t endQuote = line.find("\"", startQuote + 1);
    if (endQuote == std::string::npos) return "";
    // 4. Extract the substring between the quotes
    return line.substr(startQuote + 1, endQuote - startQuote - 1);
}

CSpriteFont::CSpriteFont() : m_pTexture(nullptr), m_texWidth(0), m_texHeight(0) {}

CSpriteFont::~CSpriteFont() {
    Shutdown();
}

void CSpriteFont::Shutdown() {
    if (m_pTexture) {
        m_pTexture->Release();
        m_pTexture = nullptr;
    }
    m_chars.clear();
    m_batchVertices.clear();
}

bool CSpriteFont::InitFromMemory(IDirect3DDevice9* pDevice, const void* pFontData, int fontSize, const void* pTextureData, int textureSize) {
    m_pDev = pDevice;

    m_pages.resize(1); // Ensure size is correct
    // 1. Load Texture from Memory Pointer
    if (FAILED(D3DXCreateTextureFromFileInMemory(m_pDev, pTextureData, textureSize, &m_pages[0]))) {
        return false;
    }

    // 2. Parse Font Data from Memory Pointer
    std::string dataString((char*)pFontData, fontSize);
    std::stringstream ss(dataString);

    std::string line;
    while (std::getline(ss, line)) {
        // Skip external page loading
        if (line.find("page") == 0) continue;
        ParseLine(line);
    }

    m_currentPage = -1;
    return true;
}

bool CSpriteFont::Init(IDirect3DDevice9* pDevice, const TCHAR* fontFilePath) {
    // 1. Load the Texture
    //if (FAILED(D3DXCreateTextureFromFile(pDevice, texturePath, &m_pTexture))) {
    //    return false; // Texture failed to load
    //}
    m_pDev = pDevice;
    // 2. Open the Font Data File
    std::ifstream file(fontFilePath);
    if (!file.is_open()) {
        return false; // Font file not found
    }

    // 3. Parse the File Line by Line
    std::string line;
    while (std::getline(file, line)) {
        ParseLine(line);
    }

    file.close();
    m_currentPage = -1;
    return true;
}

void CSpriteFont::ParseLine(const std::string& line) {
    // We only care about "common" (texture size) and "char" (letter data)
    if (line.find("common") == 0) {
        m_texWidth = GetValue(line, "scaleW");
        m_texHeight = GetValue(line, "scaleH");
    }
    if (line.find("char ") == 0) { // ensuring it's not "chars count"
        CharDesc c;
        c.id = GetValue(line, "id");
        c.x = GetValue(line, "x");
        c.y = GetValue(line, "y");
        c.width = GetValue(line, "width");
        c.height = GetValue(line, "height");
        c.xoffset = GetValue(line, "xoffset");
        c.yoffset = GetValue(line, "yoffset");
        c.xadvance = GetValue(line, "xadvance");
		c.page = GetValue(line, "page");

        m_chars[c.id] = c;
    }
    if (line.find("page") == 0) {
        // If we already loaded textures (via InitFromResource), SKIP THIS.
        if (!m_pages.empty() && m_pages[0] != nullptr) {
            return;
        }


        //// Parse: page id=0 file="arial_0.png"
        //int id = GetValue(line, "id");
        //std::string filename = GetValueS(line, "file"); // You'll need a helper for strings

        //// Construct full path (textureDir + filename)
        //std::string fullPath = std::string(".\\") +  filename;

        //IDirect3DTexture9* tex = nullptr;
        //if (FAILED(D3DXCreateTextureFromFileA(m_pDev, fullPath.c_str(), &tex))) {
        //    _log(L"Failed to load font");
        //}

        //// Ensure vector is big enough
        //if (m_pages.size() <= id) m_pages.resize(id + 1);
        //m_pages[id] = tex;
    }
}

void CSpriteFont::DrawString(float startX, float startY, const std::string& text, D3DCOLOR color) {
    float cursorX = startX;
    float cursorY = startY;

    for (char charCode : text) {
        // Handle newlines
        if (charCode == '\n') {
            cursorX = startX;
            cursorY += m_chars['A'].height + 5; // Rough line height estimation
            continue;
        }

        if (m_chars.find(charCode) == m_chars.end()) continue;

        CharDesc& cd = m_chars[charCode];

        // Screen Coordinates
        float screenLeft = cursorX + cd.xoffset;
        float screenTop = cursorY + cd.yoffset;
        float screenRight = screenLeft + cd.width;
        float screenBottom = screenTop + cd.height;

        // UV Coordinates
        float u1 = (float)cd.x / m_texWidth;
        float v1 = (float)cd.y / m_texHeight;
        float u2 = (float)(cd.x + cd.width) / m_texWidth;
        float v2 = (float)(cd.y + cd.height) / m_texHeight;

        // Create 2 Triangles (6 Vertices) for the Quad
        // Triangle 1
        //m_batchVertices.push_back({ screenLeft,  screenTop,    0.0f, 1.0f, color, u1, v1 });
        //m_batchVertices.push_back({ screenRight, screenTop,    0.0f, 1.0f, color, u2, v1 });
        //m_batchVertices.push_back({ screenLeft,  screenBottom, 0.0f, 1.0f, color, u1, v2 });

        //// Triangle 2
        //m_batchVertices.push_back({ screenRight, screenTop,    0.0f, 1.0f, color, u2, v1 });
        //m_batchVertices.push_back({ screenRight, screenBottom, 0.0f, 1.0f, color, u2, v2 });
        //m_batchVertices.push_back({ screenLeft,  screenBottom, 0.0f, 1.0f, color, u1, v2 });

        // Advance cursor
        cursorX += cd.xadvance;
    }
}

// New signature: 3D position (pos), scaling factor (scale), and billboard mode option
void CSpriteFont::DrawString3D(D3DXVECTOR3 pos, float scale, const std::string& text, D3DCOLOR color) {
    // --- STEP 1: Calculate the total width of the text first ---
    float totalWidth = 0.0f;
    for (char c : text) {
        if (m_chars.find(c) != m_chars.end()) {
            totalWidth += m_chars[c].xadvance;
        }
    }
    
    float cursorX = -totalWidth / 2.0f;
    float cursorY = 0.0f;

    for (char charCode : text) {
        if (m_chars.find(charCode) == m_chars.end()) continue;
        CharDesc& cd = m_chars[charCode];

        // CHECK: Did the page change?
        if (m_currentPage != -1 && m_currentPage != cd.page) {
            // We switched from Page 0 to Page 1.
            // We MUST render what we have so far, then switch textures.
            RenderBatch(m_pDev);
        }

        m_currentPage = cd.page; // Update current page
        // 1. Calculate Local Offsets (Scaled)
        // Note: We invert Y because in 3D +Y is UP, but in font files +Y is DOWN
        float localLeft = (cursorX + cd.xoffset) * scale;
        float localTop = (cursorY - cd.yoffset) * scale;
        float localRight = (cursorX + cd.xoffset + cd.width) * scale;
        float localBottom = (cursorY - cd.yoffset - cd.height) * scale;

        // 2. Add World Position
        float x1 = pos.x + localLeft;
        float y1 = pos.y + localTop;
        float x2 = pos.x + localRight;
        float y2 = pos.y + localBottom;
        float z = pos.z;

        // 3. Texture Coordinates (Same as before)
        float u1 = (float)cd.x / m_texWidth;
        float v1 = (float)cd.y / m_texHeight;
        float u2 = (float)(cd.x + cd.width) / m_texWidth;
        float v2 = (float)(cd.y + cd.height) / m_texHeight;

        // 4. Push Vertices (Standard Quad)
        m_batchVertices.push_back({ x1, y1, z, color, u1, v1 }); // Top-Left
        m_batchVertices.push_back({ x2, y1, z, color, u2, v1 }); // Top-Right
        m_batchVertices.push_back({ x1, y2, z, color, u1, v2 }); // Bottom-Left

        m_batchVertices.push_back({ x2, y1, z, color, u2, v1 }); // Top-Right
        m_batchVertices.push_back({ x2, y2, z, color, u2, v2 }); // Bottom-Right
        m_batchVertices.push_back({ x1, y2, z, color, u1, v2 }); // Bottom-Left

        cursorX += cd.xadvance;
    }
}

void CSpriteFont::RenderBatch(IDirect3DDevice9* pDevice) {
    if (m_batchVertices.empty()) return;

    // Set the texture for the CURRENT batch
    if (m_currentPage >= 0 && m_currentPage < m_pages.size()) {
        pDevice->SetTexture(0, m_pages[m_currentPage]);
    }
    // Set Render States
    pDevice->SetFVF(FVF_FONT_VERTEX3D);
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Enable Alpha Blending for transparent text
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	//pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    // Draw everything in one call
    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_batchVertices.size() / 3, m_batchVertices.data(), sizeof(FontVertex3D));
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    //pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // Clear batch for next frame
    m_batchVertices.clear();
}

