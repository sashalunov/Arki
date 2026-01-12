#pragma once
#pragma pack(push, 1)
//==============================================================================
// GoldSrc .BSP file format
//==============================================================================
#define HL1_BSP_VERSION 30

struct hl1_lump_t {
    int fileofs;
    int filelen;
};

struct hl1_dheader_t {
    int version;
    hl1_lump_t lumps[15];
};

// LUMP INDICES
#define HL1_LUMP_ENTITIES       0
#define HL1_LUMP_PLANES         1
#define HL1_LUMP_TEXTURES       2  // <--- Here it is (Index 2)
#define HL1_LUMP_VERTEXES       3
#define HL1_LUMP_VISIBILITY     4
#define HL1_LUMP_NODES          5
#define HL1_LUMP_TEXINFO        6
#define HL1_LUMP_FACES          7
#define HL1_LUMP_LIGHTING       8
#define HL1_LUMP_CLIPNODES      9
#define HL1_LUMP_LEAFS          10
#define HL1_LUMP_MARKSURFACES   11
#define HL1_LUMP_EDGES          12
#define HL1_LUMP_SURFEDGES      13
#define HL1_LUMP_MODELS         14

#define HL1_HEADER_LUMPS        15

struct hl1_dvertex_t {
    float point[3];
};

struct hl1_dedge_t {
    unsigned short v[2]; // Start/End vertex indices
};

struct hl1_dface_t {
    unsigned short planenum;
    short side;
    int firstedge;  // Index into surfedges
    short numedges;
    short texinfo;  // Index into texture info
    BYTE styles[4];
    int lightofs;   // Offset into lightmap lump
};

struct hl1_texinfo_t {
    float s[4]; // S vector (x, y, z, offset)
    float t[4]; // T vector (x, y, z, offset)
    int miptex; // Index into miptex array
    int flags;
};

// Texture header (Embedded textures)
struct hl1_dmiptexlump_t {
    int nummiptex;
    int dataofs[4]; // Array of offsets (variable size)
};

struct hl1_miptex_t {
    char name[16];
    unsigned int width, height;
    unsigned int offsets[4]; // Mip levels
};

// ----------------------------------------------------------------------------
// ENTITY STRUCT
// ----------------------------------------------------------------------------
struct HL1Entity
{
    std::map<std::string, std::string> properties;

    // Helper: Get value by key (returns empty string if missing)
    std::string Get(const std::string& key) const
    {
        auto it = properties.find(key);
        return (it != properties.end()) ? it->second : "";
    }

    // Helper: Parse "origin" or any "x y z" string
    // NOTE: This returns RAW coordinates. You must apply HL1BSP::SCALE_FACTOR manually
    // or use a helper that does the swizzle.
    D3DXVECTOR3 GetVector(const std::string& key) const
    {
        std::string val = Get(key);
        if (val.empty()) return D3DXVECTOR3(0, 0, 0);
        float x, y, z;
        sscanf_s(val.c_str(), "%f %f %f", &x, &y, &z);
        return D3DXVECTOR3(x, y, z);
    }
};
#pragma pack(pop)