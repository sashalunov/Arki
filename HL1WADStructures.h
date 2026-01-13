#pragma once

#define WAD3_ID (('3'<<24)+('D'<<16)+('A'<<8)+'W') // "WAD3"

#pragma pack(push, 1)

struct wadheader_t {
    int ident;      // "WAD3"
    int numlumps;
    int diroffset;
};

struct wadlump_t {
    int filepos;
    int disksize;
    int size;       // uncompressed size
    char type;      // 0x43 = Miptex
    char compression;
    char pad[2];
    char name[16];  // Case insensitive
};

struct bspmiptex_t {
    char name[16];
    unsigned int width, height;
    unsigned int offsets[4]; // Offsets to 4 mip levels
};

struct HL1RenderFace
{
    int textureID;      // Index into m_textures (to find name)
    LPDIRECT3DTEXTURE9 pTexture;
    int startIndex;     // Start index in m_pIB
    int primCount;      // Number of triangles
    bool isTransparent; // Does it start with '{'?
};
#pragma pack(pop)
