#pragma once
#include "stdafx.h"
#include "TextureManager.h"
#include "Q3BSPStructures.h"
#include "HL1BSPStructures.h"

class CHL1BSP
{
public:
    CHL1BSP();
    ~CHL1BSP();

    bool Load(btDynamicsWorld* dynamicsWorld, const std::wstring& filename);
    D3DXVECTOR3 GetScaledOrigin(const std::string& originString) const;
    // Convert generic HL1 structures to D3D buffers
    bool InitGraphics(LPDIRECT3DDEVICE9 pDevice);
    void Render();
    void OnLostDevice();
    void OnResetDevice();
    void PreloadTextures();
    void LoadEmbeddedTextures(FILE* f, const hl1_dheader_t& h);
    void SetTextureManager(TextureManager* mgr) { m_pTextureMgr = mgr; }
    const std::vector<HL1Entity>& GetEntities() const { return m_entities; }

    const float SCALE_FACTOR = 0.03f;
    const int ATLAS_SIZE = 1024; // 1024x1024 Lightmap Texture
private:
    void Clear();
    void InitPhysics(btDynamicsWorld* dynamicsWorld);
    void CleanupPhysics();

    // Internal generic loader
    template <typename T>
    bool LoadLump(FILE* f, const hl1_dheader_t& h, int lumpIndex, std::vector<T>& dest);
    bool LoadEntities(FILE* f, const hl1_dheader_t& h);
    // Geometry Generation Helper
    void GenerateGeometry();
    void CreateLightmaps();

    // Helper to manage packing (Simple "Scanline" allocator)
    int m_packX, m_packY, m_packRowHeight;
    bool AllocateLightmapBlock(int w, int h, int& outX, int& outY);

    // Raw Data
    std::vector<hl1_dvertex_t>  m_rawVerts;
    std::vector<hl1_dedge_t>    m_rawEdges;
    std::vector<int>            m_rawSurfEdges; // Note: HL1 uses int for surfedges
    std::vector<hl1_dface_t>    m_rawFaces;
    std::vector<hl1_texinfo_t>  m_rawTexInfo;
    std::vector<HL1Entity>      m_entities;
    std::vector<BYTE>           m_rawLighting; // Raw data from LUMP 8
    std::vector<DWORD>          m_atlasPixels; // Intermediate CPU buffer (ARGB)
    std::vector<HL1RenderFace> m_renderFaces;
    // Texture Data
    struct TextureInfo {
        std::string name;
        int width, height;
    };
    std::vector<TextureInfo> m_textures;
    LPDIRECT3DTEXTURE9          m_pLightmapAtlas; // The one big texture we will build
    TextureManager* m_pTextureMgr;
    // Generated D3D Data
    std::vector<Q3BSPVertex>      m_renderVerts;
    std::vector<int>            m_renderIndices;

    // DX9 Resources
    LPDIRECT3DDEVICE9           m_pDevice;
    LPDIRECT3DVERTEXBUFFER9     m_pVB;
    LPDIRECT3DINDEXBUFFER9      m_pIB;

    // Bullet Collision Objects
    btDynamicsWorld* m_pdworld;
    btTriangleMesh* m_pTriangleMesh;
    btBvhTriangleMeshShape* m_pCollisionShape;
    btCollisionObject* m_pLevelObject;


};
