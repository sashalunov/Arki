#include "stdafx.h"
#include "TextureManager.h"
#include <windows.h> // For MultiByteToWideChar

TextureManager::TextureManager(LPDIRECT3DDEVICE9 pDevice) : m_pDevice(pDevice) 
{
}

TextureManager::~TextureManager() 
{ Clear(); }

void TextureManager::AddTexture(const std::wstring& name, LPDIRECT3DTEXTURE9 pTex)
{
    std::wstring key = name;
    //std::transform(key.begin(), key.end(), key.begin(), std::tolower);
    // Convert to lower case manually
    for (auto& c : key) {
        c = (TCHAR)tolower((unsigned char)c);
    }
    // If it exists, release the old one
    if (m_textures.find(key) != m_textures.end()) {
        m_textures[key]->Release();
    }

    // Add new one (AddRef if you want to share ownership, 
    // but usually we transfer ownership here so no AddRef needed if created fresh)
    m_textures[key] = pTex;
}

void TextureManager::Clear() 
{
    for (auto& pair : m_textures) 
    {
        if (pair.second) pair.second->Release();
    }
    m_textures.clear();
    for (auto& w : m_wads) 
    {
        if (w.fileHandle) fclose(w.fileHandle);
    }
    m_wads.clear();
}

// Helper to check if string ends with suffix (case insensitive)
bool EndsWith(const std::wstring& str, const std::wstring& suffix)
{
    if (str.length() < suffix.length()) return false;
    return _wcsicmp(str.c_str() + str.length() - suffix.length(), suffix.c_str()) == 0;
}

std::wstring TextureManager::FindWadRecursively(const std::wstring& rootDir, const std::wstring& wadName)
{
    std::wstring searchPath = rootDir + L"\\*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return L"";

    std::wstring result = L"";
    do
    {
        // Skip "." and ".."
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // --- IT IS A FOLDER: RECURSE ---
            std::wstring subDir = rootDir + L"\\" + findData.cFileName;
            result = FindWadRecursively(subDir, wadName);

            // If we found it in the subdirectory, stop searching and return
            if (!result.empty()) break;
        }
        else
        {
            // --- IT IS A FILE: CHECK NAME ---
            if (_wcsicmp(findData.cFileName, wadName.c_str()) == 0)
            {
                // Found exact match!
                result = rootDir + L"\\" + findData.cFileName;
                break;
            }
        }

    } while (FindNextFile(hFind, &findData) != 0);

    FindClose(hFind);
    return result;
}

BOOL TextureManager::LoadWAD(const std::wstring& path)
{
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;

    wadheader_t h;
    fread(&h, sizeof(wadheader_t), 1, f);

    if (h.ident != WAD3_ID) { fclose(f); return false; }

    WADEntry entry;
    entry.path = path;
    entry.fileHandle = f; // Keep open for reading textures later

    // Read Directory
    fseek(f, h.diroffset, SEEK_SET);
    entry.directory.resize(h.numlumps);
    fread(entry.directory.data(), sizeof(wadlump_t), h.numlumps, f);

    m_wads.push_back(entry);
    return true;
}

LPDIRECT3DTEXTURE9 TextureManager::GetTexture(const std::wstring& name)
{
    // 1. Check Cache
    std::wstring key = name;
    for (auto& c : key)
    {
        c = (TCHAR)tolower((unsigned char)c);
    }

    if (m_textures.find(key) != m_textures.end())
        return m_textures[key];

    // 2. Search WADs
    for (const auto& wad : m_wads)
    {
        for (const auto& lump : wad.directory)
        {
            // Convert lump.name (char[16]) to std::wstring
            wchar_t lumpNameW[17] = {0};
            MultiByteToWideChar(CP_ACP, 0, lump.name, -1, lumpNameW, 16);
            // Case-insensitive comparison
            if (_wcsicmp(lumpNameW, key.c_str()) == 0)
            {
                // Found it! Load and Cache.
                LPDIRECT3DTEXTURE9 tex = LoadFromWAD(wad, lump);
                if (tex) m_textures[key] = tex;
                return tex;
            }
        }
    }

    return NULL; // Not found (purple checkerboard placeholder?)
}

LPDIRECT3DTEXTURE9 TextureManager::LoadFromWAD(const WADEntry& wad, const wadlump_t& lump)
{
    FILE* f = wad.fileHandle;
    fseek(f, lump.filepos, SEEK_SET);

    // 1. Read MipTex Header
    bspmiptex_t mt;
    fread(&mt, sizeof(bspmiptex_t), 1, f);

    int w = mt.width;
    int h = mt.height;

    // 2. Seek to Palette (Skip header + 4 mip levels)
    // Size of mips = w*h + (w/2)*(h/2) + (w/4)*(h/4) + (w/8)*(h/8) roughly
    // Or simpler: offsets[3] + (w/8)*(h/8)
    int mipSize = (w * h) + (w * h / 4) + (w * h / 16) + (w * h / 64);

    // 3. Read Indices (Mip Level 0)
    std::vector<BYTE> indices(w * h);
    fseek(f, lump.filepos + mt.offsets[0], SEEK_SET);
    fread(indices.data(), 1, w * h, f);

    // 4. Read Palette
    // Palette is located after the last mip byte + 2 bytes for size
    fseek(f, lump.filepos + mt.offsets[3] + (w * h / 64) + 2, SEEK_SET);

    BYTE palette[256 * 3];
    fread(palette, 1, 768, f);

    // 5. Create D3D Texture
    LPDIRECT3DTEXTURE9 pTex = NULL;
    if (FAILED(m_pDevice->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTex, NULL)))
        return NULL;

    // 6. Fill Texture (Convert 8-bit -> 32-bit)
    D3DLOCKED_RECT rect;
    pTex->LockRect(0, &rect, NULL, 0);

    BYTE* dest = (BYTE*)rect.pBits;

    bool isTransparent = (lump.name[0] == '{'); // Special HL1 convention

    for (int i = 0; i < w * h; i++)
    {
        int idx = indices[i];
        BYTE r = palette[idx * 3 + 0];
        BYTE g = palette[idx * 3 + 1];
        BYTE b = palette[idx * 3 + 2];
        BYTE a = 255;

        // Chroma Key for '{' textures: Pure Blue (0,0,255) is invisible
        if (isTransparent && r == 0 && g == 0 && b == 255)
            a = 0;

        // Write ARGB (Little Endian: B, G, R, A)
        DWORD* pPixel = (DWORD*)dest;
        *pPixel = (a << 24) | (r << 16) | (g << 8) | b;

        dest += 4;
    }
    pTex->UnlockRect(0);

    return pTex;
}

