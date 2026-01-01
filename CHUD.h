#pragma once
#include "CSpriteBatch.h" // Reuse your existing batcher!
#include "CSpriteFont.h"

// Struct to define a UI element in percentages
struct UIFrame {
    float x, y; // 0.0 to 1.0 (Position relative to screen size)
    float w, h; // 0.0 to 1.0 (Size relative to screen size)
};


class CHUD
{
private:
    ID3DXFont* m_pFont;
    IDirect3DTexture9* m_pTexUI; // Atlas or generic UI texture

    float m_screenWidth;
    float m_screenHeight;
    // Health Bar: Start at 5% from left, 5% from bottom. Width is 30% of screen.
    UIFrame healthBarFrame = { 0.05f, 0.05f, 0.05f, 0.45f };
	CSpriteFont* m_font;
public:
    CHUD();
    ~CHUD();

    void Init(IDirect3DDevice9* device, CSpriteFont* fnt, int width, int height);
    void OnResize(int width, int height);
	void OnLostDevice();
    void Invalidate();

    // The Main Draw Command
    void Render(IDirect3DDevice9* device, CSpriteBatch* spriteBatch, float healthPercent, int score);

private:
    // Helper to draw a progress bar (health/stamina)
    void DrawProgressBar(CSpriteBatch* batch, float x, float y, float w, float h, float percentage, D3DCOLOR color, bool isVertical);
    void DrawCrosshair(CSpriteBatch* batch);

    // Helper to draw text
    void DrawTextStr(const std::wstring& text, float x, float y, D3DCOLOR color);
};

