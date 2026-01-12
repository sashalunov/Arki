#pragma once
#include "HL1WADStructures.h"

class TextureManager
{
public:
    TextureManager(LPDIRECT3DDEVICE9 pDevice);
    ~TextureManager();

    // 1. Load a WAD file into memory (or keep handle open)
    BOOL LoadWAD(const std::wstring& path);
    std::wstring FindWadRecursively(const std::wstring& rootDir, const std::wstring& wadName);
    // 2. Get a Texture (Loads from WAD if not already cached)
    LPDIRECT3DTEXTURE9 GetTexture(const std::wstring& name);

    void Clear();

private:
    struct WADEntry {
        std::wstring path;
        std::vector<wadlump_t> directory;
        FILE* fileHandle;
    };

    // Helper to read raw pixels and convert to D3D Texture
    LPDIRECT3DTEXTURE9 LoadFromWAD(const WADEntry& wad, const wadlump_t& lump);

    LPDIRECT3DDEVICE9 m_pDevice;

    // Cache: Name -> Texture Pointer
    std::map<std::wstring, LPDIRECT3DTEXTURE9> m_textures;

    // Loaded WADs
    std::vector<WADEntry> m_wads;
};


