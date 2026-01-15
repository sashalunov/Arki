#pragma once
#include "TextureManager.h"

class CWADViewer
{
public:
    CWADViewer(TextureManager* pTexMgr);
    ~CWADViewer();
    // Main draw function to call inside your ImGui loop
    void Draw(BOOL* pOpen);

    // Open a specific WAD file
    void OpenWAD(const std::wstring& path);

private:
	BOOL m_isOpen;
    TextureManager* m_pTexMgr;
    std::wstring m_currentWadPath;

    struct TextureEntry {
        std::wstring name;
        int width;
        int height;
        LPDIRECT3DTEXTURE9 pTexture; // Cached pointer (starts NULL)
    };
    std::vector<TextureEntry> m_entries;

    // Helper to read the WAD header/directory without loading pixels yet
    void ScanWadContent();
};
