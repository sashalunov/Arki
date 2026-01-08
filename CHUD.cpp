#include "StdAfx.h"
#include "CHUD.h"

CHUD::CHUD() : m_pFont(NULL), m_pTexUI(NULL) {}

CHUD::~CHUD() { Invalidate(); }

void CHUD::Init(IDirect3DDevice9* device, CSpriteFont* fnt, int width, int height)
{
    m_screenWidth = (float)width;
    m_screenHeight = (float)height;

    // 1. Create a Font
    D3DXCreateFont(device, 24, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Arial", &m_pFont);

    // 2. Load UI Texture (Ensure you have a white texture or specific UI asset)
    // For this example, we assume you have a generic particle or white block texture
    D3DXCreateTextureFromFile(device, L"uv.png", &m_pTexUI);
	//m_font = new CSpriteFont();
	m_font = fnt;
}

void CHUD::Invalidate()
{
    if (m_pFont) { m_pFont->Release(); m_pFont = NULL; }
    if (m_pTexUI) { m_pTexUI->Release(); m_pTexUI = NULL; }
	//m_font->Shutdown();
	//delete m_font;
}

void CHUD::OnResize(int width, int height)
{
    m_screenWidth = (float)width;
    m_screenHeight = (float)height;
    if (m_pFont) m_pFont->OnResetDevice();
}
void CHUD::OnLostDevice()
{
    if (m_pFont) m_pFont->OnLostDevice();
}
void CHUD::Render(IDirect3DDevice9* device, CSpriteBatch* spriteBatch,   float healthPercent, int score)
{
    // --- STEP 1: Backup 3D Matrices ---
    D3DXMATRIX matProjOld, matViewOld;
    device->GetTransform(D3DTS_PROJECTION, &matProjOld);
    device->GetTransform(D3DTS_VIEW, &matViewOld);

    // --- STEP 2: Setup 2D Orthographic Matrix ---
    // This maps coordinates 0..Width and 0..Height to the screen
    D3DXMATRIX matOrtho;
    D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, m_screenWidth, m_screenHeight, 0.0f, 0.0f, 1.0f);
    device->SetTransform(D3DTS_PROJECTION, &matOrtho);

    // View Matrix is Identity (Camera is at 0,0 looking at Z)
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    device->SetTransform(D3DTS_VIEW, &matIdentity);
    device->SetTransform(D3DTS_WORLD, &matIdentity);

    // --- STEP 3: Disable Z-Buffer ---
    // HUD must always draw ON TOP of the 3D world
    device->SetRenderState(D3DRS_ZENABLE, FALSE);

    // --- STEP 4: Draw Sprites using CSpriteBatch ---
    // Note: We pass Identity view matrix so billboards face "forward" (Screen)
    spriteBatch->Begin(matIdentity, SORT_IMMEDIATE);

    // --- CALCULATE PIXEL DIMENSIONS ---
    float pixelX = healthBarFrame.x * m_screenWidth;
    float pixelY = healthBarFrame.y * m_screenHeight;
    float pixelW = healthBarFrame.w * m_screenWidth;
    float pixelH = healthBarFrame.h * m_screenHeight;
    // A. Draw Crosshair (Center of Screen)
    // We assume the texture has a crosshair. 
    // SPRITE_BILLBOARD works fine here because View is Identity.
	//DrawCrosshair(spriteBatch);
    // --- DRAW BACKGROUND (Grey Bar) ---
// Pass the calculated pixel coordinates
    DrawProgressBar(spriteBatch, pixelX, pixelY,  pixelW, pixelH, 1.0f, D3DCOLOR_ARGB(205, 50, 50, 50), true);

    // --- DRAW FOREGROUND (Red Health) ---
    DrawProgressBar(spriteBatch, pixelX, pixelY, pixelW, pixelH, healthPercent, D3DCOLOR_ARGB(255, 255, 50, 50), true);

    spriteBatch->End();

    std::wstring scoreText = L"Score: " + std::to_wstring(score);
    std::wstring healthText = std::to_wstring(static_cast<int>(healthPercent * 100)) + L"%";
    // --- DRAW TEXT USING CSpriteFont ---

    m_font->DrawString(pixelX, pixelY, pixelW, L"Health", D3DCOLOR_ARGB(255, 255, 155, 255));
    m_font->DrawString(pixelX, m_screenHeight * 0.1f, pixelW, healthText, D3DCOLOR_ARGB(255, 255, 155, 255));

    m_font->DrawString(m_screenWidth*0.80f, m_screenHeight*0.05f, m_screenWidth*0.1f, scoreText, D3DCOLOR_ARGB(255, 155, 255, 255));

	m_font->RenderBatch(device);
    // --- STEP 5: Draw Text ---
    // D3DXFont handles its own Sprite batching internally
   // DrawTextStr(scoreText, m_screenWidth - 150, 50, D3DCOLOR_XRGB(255, 255, 0));

    // --- STEP 6: Restore 3D State ---
    device->SetTexture(0, NULL);

    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetTransform(D3DTS_VIEW, &matViewOld);
    device->SetTransform(D3DTS_PROJECTION, &matProjOld);
    device->SetTransform(D3DTS_WORLD, &matIdentity);

}

void CHUD::DrawProgressBar(CSpriteBatch* batch, float x, float y,  float w, float h, float percentage, D3DCOLOR color, bool isVertical)
{
    // Clamp percentage to safe 0.0 - 1.0 range
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 1.0f) percentage = 1.0f;

    float currentW, currentH;
    float centerX, centerY;

    if (isVertical)
    {
        // --- VERTICAL (Bottom-to-Top) ---
        currentW = w;
        currentH = h * percentage;

        // X stays centered relative to the full container
        centerX = x + (w * 0.5f);

        // Y needs to be aligned to the BOTTOM.
        // Screen Y grows downwards. So 'y + h' is the bottom pixel.
        // To find the center of our new shorter bar:
        // Start at Bottom (y + h) -> Move UP by half the new height.
        centerY = (y + h ) - (currentH * 0.5f);
    }
    else
    {
        // --- HORIZONTAL (Left-to-Right) ---
        currentW = w * percentage;
        currentH = h;

        // X aligns to the LEFT.
        // Start at Left (x) -> Move RIGHT by half the new width.
        centerX = x + (currentW * 0.5f);

        // Y stays centered relative to the full container
        centerY = y + (h * 0.5f);
    }

    // Draw the calculated sprite
    batch->Draw(m_pTexUI,
        btVector3(centerX, centerY, 0),
        btVector2(currentW, currentH),
        color,
        SPRITE_BILLBOARD);
}

void CHUD::DrawTextStr(const std::wstring& text, float x, float y, D3DCOLOR color)
{
    RECT rect;
    rect.left = (long)x;
    rect.top = (long)y;
    rect.right = (long)(x + 300);
    rect.bottom = (long)(y + 50);

    m_pFont->DrawTextW(NULL, text.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, color);
}

void CHUD::DrawCrosshair(CSpriteBatch* batch)
{
    // Position: Center (50%)
    float x = 0.5f * m_screenWidth;
    float y = 0.5f * m_screenHeight;

    // Size: Fixed percentage of HEIGHT to keep it square
    // e.g., 5% of screen height
    float size = 0.25f * m_screenHeight;

    batch->Draw(m_pTexUI,
        btVector3(x, y, 0),
        btVector2(size, size), // Use same size for W and H
        D3DCOLOR_XRGB(255, 255, 255),
        SPRITE_BILLBOARD);
}