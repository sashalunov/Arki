#include "stdafx.h"
#include "D3DRender.h"
#include "CQ3BSP.h" // Include this to access BSPVertex definition
#include "CHL1BSP.h"


CHL1BSP::CHL1BSP() : m_pDevice(NULL), m_pVB(NULL), m_pIB(NULL) {
	m_pdworld = NULL;
	m_pTriangleMesh = NULL;
	m_pCollisionShape = NULL;
	m_pLevelObject = NULL;
	m_pTextureMgr = NULL;

}
CHL1BSP::~CHL1BSP() 
{
    m_pTextureMgr = NULL;
    CleanupPhysics();
    OnLostDevice(); 
}

D3DXVECTOR3 CHL1BSP::GetScaledOrigin(const std::string& originString) const
{
    if (originString.empty()) return D3DXVECTOR3(0, 0, 0);

    float x, y, z;
    if (sscanf_s(originString.c_str(), "%f %f %f", &x, &y, &z) == 3)
    {
        // Apply SAME scaling/swizzling as geometry
        return D3DXVECTOR3(
            x * SCALE_FACTOR,
            z * SCALE_FACTOR,  // Z -> Y
            y * -SCALE_FACTOR   // Y -> Z
        );
    }
    return D3DXVECTOR3(0, 0, 0);
}

void CHL1BSP::OnLostDevice() {
    if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
    if (m_pIB) { m_pIB->Release(); m_pIB = NULL; }
}
void CHL1BSP::OnResetDevice()
{
    if (!m_pDevice)
        return;
    InitGraphics(m_pDevice);
}

void CHL1BSP::Clear() {
    m_rawVerts.clear(); m_rawEdges.clear(); m_rawSurfEdges.clear();
    m_rawFaces.clear(); m_rawTexInfo.clear(); m_textures.clear();
	m_renderVerts.clear(); m_renderIndices.clear(); m_renderFaces.clear();
    m_entities.clear();
	m_atlasPixels.clear();
	m_rawLighting.clear();
}

template <typename T>
bool CHL1BSP::LoadLump(FILE* f, const hl1_dheader_t& h, int lumpIndex, std::vector<T>& dest)
{
    if (h.lumps[lumpIndex].filelen == 0) return true;
    int num = h.lumps[lumpIndex].filelen / sizeof(T);
    dest.resize(num);
    fseek(f, h.lumps[lumpIndex].fileofs, SEEK_SET);
    return fread(dest.data(), sizeof(T), num, f) == num;
}
void removeFirstOccurrence(std::wstring& mainStr, const std::wstring& subStr) {
    // Find the first occurrence of the substring
    size_t pos = mainStr.find(subStr);

    // Check if the substring was found
    if (pos != std::string::npos) {
        // Erase the substring at the found position
        mainStr.erase(pos, subStr.length());
    }
}
bool CHL1BSP::Load(btDynamicsWorld* dynamicsWorld, const std::wstring& filename)
{

    size_t lastSlash = filename.find_last_of(L"\\/");
    std::wstring hlpath = filename.substr(0,lastSlash);
    lastSlash = hlpath.find_last_of(L"\\/");
	hlpath = hlpath.substr(0, lastSlash);
    lastSlash = hlpath.find_last_of(L"\\/");
    hlpath = hlpath.substr(0, lastSlash);
    lastSlash = hlpath.find_last_of(L"\\/");
    hlpath = hlpath.substr(0, lastSlash);

    Clear();
    OnLostDevice();
    std::wstring wadfilename;
    FILE* file = _wfopen(filename.c_str(), L"rb");
    if (!file) return false;

    hl1_dheader_t header;
    fread(&header, sizeof(hl1_dheader_t), 1, file);

    if (header.version != HL1_BSP_VERSION) { fclose(file); return false; }
    // --- ADD THIS CALL HERE ---
    if (!LoadEntities(file, header)) {
        fclose(file);
        return false;
    }

    if (m_pTextureMgr && !m_entities.empty())
    {
        // Entity 0 is worldspawn
        std::string wadString = m_entities[0].Get("wad");

        // Parse semicolon separated list
        std::wstring currentWad;
        for (char c : wadString)
        {
            if (c == ';')
            {
                // Extract filename from full path (e.g. "C:\stuff\halflife.wad" -> "halflife.wad")
                size_t lastSlash = currentWad.find_last_of(L"\\/");
                wadfilename = (lastSlash != std::wstring::npos) ? currentWad.substr(lastSlash + 1) : currentWad;
                // Tell manager to load it
                // 2. Try loading directly first (fastest)
                if (!m_pTextureMgr->LoadWAD(wadfilename))
                {
                    // 3. If failed, search recursively starting from your game folder
                    // Change "." to your asset folder like "C:\\MyGame\\Assets" if needed
                    std::wstring fullPath = m_pTextureMgr->FindWadRecursively(hlpath, wadfilename);

                    if (!fullPath.empty()) 
                    {
                        m_pTextureMgr->LoadWAD(fullPath);
                    }
                    else {
                        // Log error: "WAD not found anywhere!"
                    }
                }
                currentWad = L"";
            }
            else {
                currentWad += c;
            }
        }
        // Load last one if no trailing semicolon
        if (!currentWad.empty()) 
        { 
           size_t lastSlash = currentWad.find_last_of(L"\\/");

            wadfilename = (lastSlash != std::wstring::npos) ? currentWad.substr(lastSlash + 1) : currentWad;
            if (!m_pTextureMgr->LoadWAD(wadfilename))
            {
                // 3. If failed, search recursively starting from your game folder
                // Change "." to your asset folder like "C:\\MyGame\\Assets" if needed
                std::wstring fullPath = m_pTextureMgr->FindWadRecursively(hlpath, wadfilename);

                if (!fullPath.empty())
                {
                    m_pTextureMgr->LoadWAD(fullPath);
                }
                else {
                    // Log error: "WAD not found anywhere!"
                }
            }

        }
    }

    // Load Geometry Lumps
    LoadLump(file, header, HL1_LUMP_VERTEXES, m_rawVerts);
    LoadLump(file, header, HL1_LUMP_EDGES, m_rawEdges);
    LoadLump(file, header, HL1_LUMP_SURFEDGES, m_rawSurfEdges);
    LoadLump(file, header, HL1_LUMP_FACES, m_rawFaces);
    LoadLump(file, header, HL1_LUMP_TEXINFO, m_rawTexInfo);
    LoadLump(file, header, HL1_LUMP_LIGHTING, m_rawLighting);

    // Load Texture Names (Slightly complex due to variable size)
    if (header.lumps[HL1_LUMP_TEXINFO].filelen > 0)
    {
        // 1. Read the Miptex Lump Header
        fseek(file, header.lumps[HL1_LUMP_TEXTURES].fileofs, SEEK_SET); // NOTE: LUMP_TEXTURES is actually index 15 in some docs, but usually 2 in Q1? 
        // Correction: HL1 stores textures in a specific way. 
        // Let's assume standard behavior:
        int numMipTex;
        fread(&numMipTex, sizeof(int), 1, file);

        std::vector<int> offsets(numMipTex);
        fread(offsets.data(), sizeof(int), numMipTex, file);

        for (int i = 0; i < numMipTex; i++) {
            if (offsets[i] == -1) continue; // External WAD texture

            fseek(file, header.lumps[HL1_LUMP_TEXTURES].fileofs + offsets[i], SEEK_SET);
            hl1_miptex_t mt;
            fread(&mt, sizeof(hl1_miptex_t), 1, file);

            TextureInfo ti;
            ti.width = mt.width;
            ti.height = mt.height;
            ti.name = mt.name;

            m_textures.push_back(ti);
        }
    }
    LoadEmbeddedTextures(file, header);

    fclose(file);

    // Convert raw data to triangles for D3D
    GenerateGeometry();
    InitPhysics(dynamicsWorld);

    return InitGraphics(d3d9->GetDevice());
}

void CHL1BSP::PreloadTextures()
{
    if (!m_pTextureMgr) return;

    for (auto& face : m_renderFaces)
    {
        // 1. Get the texture name from the BSP data
        // m_textures was populated during Load() from the MIP lump
        if (face.textureID >= 0 && face.textureID < (int)m_textures.size())
        {
            std::wstring texName = std::wstring(m_textures[face.textureID].name.begin(), m_textures[face.textureID].name.end());

            // 2. Force load (or retrieve from cache)
            face.pTexture = m_pTextureMgr->GetTexture(texName);
        }
        else
        {
            face.pTexture = NULL;
        }
    }
}
void CHL1BSP::GenerateGeometry()
{
    m_renderVerts.clear();
    m_renderIndices.clear();
    m_renderFaces.clear();

    // 1. Initialize Lightmap Atlas (CPU side)
    m_atlasPixels.assign(ATLAS_SIZE * ATLAS_SIZE, 0xFF000000); // Fill with black
    m_packX = 0; m_packY = 0; m_packRowHeight = 0;

    for (const auto& face : m_rawFaces)
    {
        if (face.numedges < 3) continue;
        if (face.texinfo < 0 || face.texinfo >= m_rawTexInfo.size()) continue;

        const hl1_texinfo_t& ti = m_rawTexInfo[face.texinfo];

        // ---------------------------------------------------------
        // SKIP SKY SURFACES
        // ---------------------------------------------------------
        if (ti.miptex >= 0 && ti.miptex < m_textures.size())
        {
            std::string texName = m_textures[ti.miptex].name;

            // In HL1, the special texture is literally named "sky"
            // We check both cases just to be safe.
            if (texName == "sky" || texName == "SKY")
            {
                continue; // Skip this face entirely
            }
        }
        // ---------------------------------------------------------
        // SKIP INVISIBLE TOOL SURFACES
        // ---------------------------------------------------------
        if (ti.miptex >= 0 && ti.miptex < m_textures.size())
        {
            std::string name = m_textures[ti.miptex].name;

            // Convert to lower case manually
            for (auto& c : name) 
            {
                c = (char)tolower((unsigned char)c);
            }
            // 1. SKYBOX
            if (name == "sky") continue;

            // 2. INVISIBLE CLIP/COLLISION
            if (name == "clip") continue;

            // 3. LOGIC TRIGGERS (Often named 'aaatrigger' or 'trigger')
            if (name == "aaatrigger" || name == "trigger") continue;

            // 4. ORIGIN BRUSHES (Rotation pivots)
            if (name == "origin") continue;

            // 5. OPTIMIZATION & NULL
            if (name == "null" || name == "nodraw" || name == "hint" || name == "skip") continue;
        }
        // --- STEP 1: CALCULATE LIGHTMAP EXTENTS ---
        // We must project the face onto the texture plane to see how big it is.
        float min_u = 999999.0f, min_v = 999999.0f;
        float max_u = -999999.0f, max_v = -999999.0f;

        // Pass 1: Find Min/Max of this polygon in Texture Space
        for (int i = 0; i < face.numedges; i++)
        {
            int edgeIdx = m_rawSurfEdges[face.firstedge + i];
            int vIdx = (edgeIdx >= 0) ? m_rawEdges[edgeIdx].v[0] : m_rawEdges[-edgeIdx].v[1];
            hl1_dvertex_t rawV = m_rawVerts[vIdx];

            // Project: Dot(Point, Axis) + Offset
            float u = rawV.point[0] * ti.s[0] + rawV.point[1] * ti.s[1] + rawV.point[2] * ti.s[2] + ti.s[3];
            float v = rawV.point[0] * ti.t[0] + rawV.point[1] * ti.t[1] + rawV.point[2] * ti.t[2] + ti.t[3];

            if (u < min_u) min_u = u;
            if (u > max_u) max_u = u;
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
        }

        // --- STEP 2: CALCULATE DIMENSIONS ---
        // HL1 lightmaps are sampled every 16 units.
        // We floor/ceil to snap to the grid.
        int texMinU = (int)floor(min_u / 16.0f);
        int texMinV = (int)floor(min_v / 16.0f);
        int texMaxU = (int)ceil(max_u / 16.0f);
        int texMaxV = (int)ceil(max_v / 16.0f);

        int lmWidth = texMaxU - texMinU + 1;
        int lmHeight = texMaxV - texMinV + 1;

        // Safety clamp
        if (lmWidth < 1) lmWidth = 1;
        if (lmHeight < 1) lmHeight = 1;

        // --- STEP 3: ALLOCATE & COPY PIXELS ---
        int allocX = 0, allocY = 0;
        bool hasLightmap = (face.lightofs != -1) && AllocateLightmapBlock(lmWidth, lmHeight, allocX, allocY);

        if (hasLightmap)
        {
            const BYTE* pSrc = &m_rawLighting[face.lightofs];

            for (int y = 0; y < lmHeight; y++)
            {
                for (int x = 0; x < lmWidth; x++)
                {
                    int r = pSrc[0];
                    int g = pSrc[1];
                    int b = pSrc[2];
                    pSrc += 3; // Advance source (RGB)

                    // Optional: Boost brightness (HL1 maps are dark)
                    r = std::min(255, r * 2); g = std::min(255, g * 2); b = std::min(255, b * 2);

                    // Write to Atlas (ARGB) at correct offset
                    int destIndex = (allocY + y) * ATLAS_SIZE + (allocX + x);
                    if (destIndex < m_atlasPixels.size())
                        m_atlasPixels[destIndex] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
        }
        // --- RECORD RENDER INFO START ---
        HL1RenderFace rFace;
        rFace.textureID = ti.miptex;
        rFace.startIndex = (int)m_renderIndices.size(); // Start of this face's indices
        rFace.isTransparent = false;
        // Check transparency
        if (ti.miptex >= 0 && ti.miptex < m_textures.size()) {
            if (m_textures[ti.miptex].name[0] == '{')
                rFace.isTransparent = true;
        }
        // --- STEP 4: GENERATE VERTICES WITH UVs ---
        // Use generic texture sizes for UV0, Atlas size for UV1
        float texW = (ti.miptex >= 0 && ti.miptex < m_textures.size()) ? (float)m_textures[ti.miptex].width : 256.0f;
        float texH = (ti.miptex >= 0 && ti.miptex < m_textures.size()) ? (float)m_textures[ti.miptex].height : 256.0f;

        int firstVertIdx = (int)m_renderVerts.size();

        for (int i = 0; i < face.numedges; i++)
        {
            int edgeIdx = m_rawSurfEdges[face.firstedge + i];
            int vIdx = (edgeIdx >= 0) ? m_rawEdges[edgeIdx].v[0] : m_rawEdges[-edgeIdx].v[1];

            hl1_dvertex_t rawV = m_rawVerts[vIdx];
            D3DXVECTOR3 rawPos(rawV.point[0], rawV.point[1], rawV.point[2]);

            // Projection again for this specific vertex
            float u = rawPos.x * ti.s[0] + rawPos.y * ti.s[1] + rawPos.z * ti.s[2] + ti.s[3];
            float v = rawPos.x * ti.t[0] + rawPos.y * ti.t[1] + rawPos.z * ti.t[2] + ti.t[3];

            Q3BSPVertex vert;

            // 1. POSITION (Swizzled & Scaled)
            vert.pos = D3DXVECTOR3(rawPos.x * SCALE_FACTOR, rawPos.z * SCALE_FACTOR, rawPos.y *-SCALE_FACTOR);
            vert.color = 0xFFFFFFFF; // White base color
            vert.normal = D3DXVECTOR3(0, 1, 0);

            // 2. TEXTURE UV (uv0)
            vert.uv0[0] = u / texW;
            vert.uv0[1] = v / texH;

            // 3. LIGHTMAP UV (uv1)
            if (hasLightmap)
            {
                // Calculate offset from the Top-Left of the lightmap block
                // Note the +0.5f center offset for sampling
                float localU = (u / 16.0f) - texMinU + 0.5f;
                float localV = (v / 16.0f) - texMinV + 0.5f;

                // Normalize to Atlas coords (0.0 to 1.0)
                vert.uv1[0] = (allocX + localU) / (float)ATLAS_SIZE;
                vert.uv1[1] = (allocY + localV) / (float)ATLAS_SIZE;
            }
            else
            {
                // Point to a known white pixel (or just 0,0 if unlit)
                vert.uv1[0] = 0.0f;
                vert.uv1[1] = 0.0f;
            }

            m_renderVerts.push_back(vert);
        }

        // Triangulate
        for (int i = 1; i < face.numedges - 1; i++) {
            m_renderIndices.push_back(firstVertIdx + 0);
            m_renderIndices.push_back(firstVertIdx + i + 1);
            m_renderIndices.push_back(firstVertIdx + i);
        }
        // --- RECORD RENDER INFO END ---
        // Calculate how many indices we just added
        int numIndicesAdded = (int)m_renderIndices.size() - rFace.startIndex;
        rFace.primCount = numIndicesAdded / 3;

        if (rFace.primCount > 0) {
            m_renderFaces.push_back(rFace);
        }
    }
}

bool CHL1BSP::InitGraphics(LPDIRECT3DDEVICE9 pDevice)
{
    m_pDevice = pDevice;
    if (m_renderVerts.empty()) return false;

    // Create Buffers (Standard D3D9)
    if (FAILED(m_pDevice->CreateVertexBuffer(m_renderVerts.size() * sizeof(Q3BSPVertex),
        D3DUSAGE_WRITEONLY, D3DFVF_Q3BSPVERTEX, D3DPOOL_MANAGED, &m_pVB, NULL))) return false;

    void* ptr;
    m_pVB->Lock(0, 0, &ptr, 0);
    memcpy(ptr, m_renderVerts.data(), m_renderVerts.size() * sizeof(Q3BSPVertex));
    m_pVB->Unlock();

    if (FAILED(m_pDevice->CreateIndexBuffer(m_renderIndices.size() * sizeof(int),
        D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_pIB, NULL))) return false;

    m_pIB->Lock(0, 0, &ptr, 0);
    memcpy(ptr, m_renderIndices.data(), m_renderIndices.size() * sizeof(int));
    m_pIB->Unlock();

    // --- CREATE LIGHTMAP TEXTURE ---
    if (FAILED(m_pDevice->CreateTexture(ATLAS_SIZE, ATLAS_SIZE, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pLightmapAtlas, NULL)))
        return false;

    D3DLOCKED_RECT rect;
    if (SUCCEEDED(m_pLightmapAtlas->LockRect(0, &rect, NULL, 0)))
    {
        // Copy m_atlasPixels to GPU
        BYTE* dest = (BYTE*)rect.pBits;
        BYTE* src = (BYTE*)m_atlasPixels.data();

        for (int y = 0; y < ATLAS_SIZE; y++)
        {
            memcpy(dest, src, ATLAS_SIZE * 4); // 4 bytes per pixel
            dest += rect.Pitch;
            src += ATLAS_SIZE * 4;
        }
        m_pLightmapAtlas->UnlockRect(0);
    }

    // Clear CPU RAM (Optional)
    // m_atlasPixels.clear();
    PreloadTextures();

    return true;
}

void CHL1BSP::Render()
{
    if (!m_pDevice || !m_pVB || !m_pIB || !m_pTextureMgr) return;

    // Enable Alpha Testing for '{' textures (Grates/Fences)
    // Pixels with Alpha < 128 will be discarded
    m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_ALPHAREF, 0x80);
    m_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

    m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE); // Q3 maps use lightmaps
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW); // Check winding order
    m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    // -----------------------------------------------------------------------
     // 2. TEXTURE STAGE 0 SETUP (THE WAD TEXTURE)
     // -----------------------------------------------------------------------
     // Operation: Select Texture Color
     // UV Source: Index 0 (uv0)
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); // Use Texture Alpha
    m_pDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    // Filters (Trilinear looks best)
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    m_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP); // Tile textures
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    // -----------------------------------------------------------------------
    // 3. TEXTURE STAGE 1 SETUP (THE LIGHTMAP)
    // -----------------------------------------------------------------------
    // Operation: Multiply Stage 0 Result * Lightmap
    // UV Source: Index 1 (uv1)
    m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE); // The Lightmap
    m_pDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT); // Output of Stage 0
    m_pDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);         // <--- CRITICAL: Use uv1
    // Lightmap Filtering (Bilinear to fix pixelation)
    m_pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); // Don't tile lightmaps
    m_pDevice->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    // Bind the Lightmap Atlas ONCE (Optimization)
    // Since all faces share the same generated atlas, we bind it to Stage 1 globally.
    m_pDevice->SetTexture(1, m_pLightmapAtlas);

    m_pDevice->SetFVF(D3DFVF_Q3BSPVERTEX);
    m_pDevice->SetStreamSource(0, m_pVB, 0, sizeof(Q3BSPVertex));
    m_pDevice->SetIndices(m_pIB);


    // Track last pointer to avoid redundant API calls
    LPDIRECT3DTEXTURE9 pLastTex = (LPDIRECT3DTEXTURE9)0xFFFFFFFF;

    for (const auto& face : m_renderFaces)
    {
        // ZERO OVERHEAD TEXTURE BINDING
        if (face.pTexture != pLastTex)
        {
            m_pDevice->SetTexture(0, face.pTexture);
            pLastTex = face.pTexture;
        }

        m_pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0, 0, m_renderVerts.size(),
            face.startIndex,
            face.primCount
        );
    }

    // -----------------------------------------------------------------------
    // 5. CLEANUP
     // -----------------------------------------------------------------------
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    // Reset Stage 1 to defaults to not mess up other rendering
    m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
    // Reset TexCoordIndex for Stage 0
    m_pDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

}

// ----------------------------------------------------------------------------
// PARSE ENTITIES LUMP
// ----------------------------------------------------------------------------
bool CHL1BSP::LoadEntities(FILE* f, const hl1_dheader_t& h)
{
    m_entities.clear();

    // LUMP_ENTITIES is Index 0 in HL1
    int fileLen = h.lumps[LUMP_ENTITIES].filelen;
    int fileOfs = h.lumps[LUMP_ENTITIES].fileofs;

    if (fileLen == 0) return true;

    // 1. Read buffer
    std::vector<char> buffer(fileLen + 1);
    fseek(f, fileOfs, SEEK_SET);
    if (fread(buffer.data(), 1, fileLen, f) != (size_t)fileLen) return false;
    buffer[fileLen] = '\0'; // Null terminate

    // 2. Parse
    char* p = buffer.data();
    HL1Entity currentEnt;
    bool inEntity = false;

    while (*p)
    {
        // Skip whitespace
        if (*p <= ' ') { p++; continue; }

        // Start Entity
        if (*p == '{') {
            currentEnt.properties.clear();
            inEntity = true;
            p++; continue;
        }

        // End Entity
        if (*p == '}') {
            if (inEntity) {
                m_entities.push_back(currentEnt);
                inEntity = false;
            }
            p++; continue;
        }

        // Key "..."
        if (*p == '\"')
        {
            std::string key, value;

            p++; // Skip quote
            while (*p && *p != '\"') key += *p++;
            if (*p == '\"') p++; // Skip end quote

            // Skip space between key and value
            while (*p && *p <= ' ') p++;

            // Value "..."
            if (*p == '\"') {
                p++; // Skip quote
                while (*p && *p != '\"') value += *p++;
                if (*p == '\"') p++; // Skip end quote
            }

            if (inEntity && !key.empty()) {
                currentEnt.properties[key] = value;
            }
            continue;
        }

        // Skip junk
        p++;
    }

    return true;
}

bool CHL1BSP::AllocateLightmapBlock(int w, int h, int& outX, int& outY)
{
    // If block doesn't fit in current row, move to next row
    if (m_packX + w > ATLAS_SIZE)
    {
        m_packX = 0;
        m_packY += m_packRowHeight;
        m_packRowHeight = 0;
    }

    // Check if texture is full
    if (m_packY + h > ATLAS_SIZE) return false;

    // Allocate
    outX = m_packX;
    outY = m_packY;

    // Advance cursor
    m_packX += w;
    if (h > m_packRowHeight) m_packRowHeight = h;

    return true;
}


void CHL1BSP::InitPhysics(btDynamicsWorld* dynamicsWorld)
{
    CleanupPhysics();

    m_pdworld = dynamicsWorld;
    // 1. Create the Triangle Mesh interface
    m_pTriangleMesh = new btTriangleMesh(true, false);
    int numTriangles = (int)m_renderIndices.size() / 3;
    // -----------------------------------------------------------------------
    // PASS 1: Static World Geometry (Walls/Floors)
    // -----------------------------------------------------------------------
    for (int i = 0; i < numTriangles; i++)
    {
        // Get Indices
        int i0 = m_renderIndices[i * 3 + 0];
        int i1 = m_renderIndices[i * 3 + 1];
        int i2 = m_renderIndices[i * 3 + 2];

        // Get Vertices
        // NOTE: We do NOT apply scale or swizzle here.
        // m_renderVerts were ALREADY scaled and swizzled in GenerateGeometry()!
        const D3DXVECTOR3& p0 = m_renderVerts[i0].pos ;
        const D3DXVECTOR3& p1 = m_renderVerts[i1].pos ;
        const D3DXVECTOR3& p2 = m_renderVerts[i2].pos;

        // Convert to Bullet Vectors
        btVector3 v0(p0.x, p0.y, p0.z);
        btVector3 v1(p1.x, p1.y, p1.z);
        btVector3 v2(p2.x, p2.y, p2.z);

        // Add to Mesh
        m_pTriangleMesh->addTriangle(v0, v1, v2);
    }
   
    // 3. Create the Shape
    // Use Quantized AABB Compression to save memory
    m_pCollisionShape = new btBvhTriangleMeshShape(m_pTriangleMesh, true);

    // 4. Create the Collision Object
    m_pLevelObject = new btCollisionObject();
    m_pLevelObject->setCollisionShape(m_pCollisionShape);

    // Set friction and restitution for the ground
    m_pLevelObject->setFriction(0.5f);
    m_pLevelObject->setRestitution(0.3f);

    // 5. Add to the World
    // Levels are static, so we don't need a MotionState or Mass
    dynamicsWorld->addCollisionObject(m_pLevelObject);
}

void CHL1BSP::CleanupPhysics()
{
    if (m_pdworld && m_pLevelObject) {
        m_pdworld->removeCollisionObject(m_pLevelObject);
        delete m_pLevelObject;
    }
    SAFE_DELETE(m_pCollisionShape);
    SAFE_DELETE(m_pTriangleMesh);
}

void CHL1BSP::LoadEmbeddedTextures(FILE* f, const hl1_dheader_t& h)
{
    if (!m_pTextureMgr || !m_pDevice) return;

    int fileLen = h.lumps[HL1_LUMP_TEXTURES].filelen;
    int fileOfs = h.lumps[HL1_LUMP_TEXTURES].fileofs;

    if (fileLen == 0) return;
    // 1. Read Number of Textures
    fseek(f, fileOfs, SEEK_SET);
    int numMipTex;
    fread(&numMipTex, sizeof(int), 1, f);
    // 2. Read Offsets Array
    std::vector<int> offsets(numMipTex);
    fread(offsets.data(), sizeof(int), numMipTex, f);

    for (int i = 0; i < numMipTex; i++)
    {
        if (offsets[i] == -1) continue; // Skip, this one is in a WAD file
        // Calculate absolute file offset to this Miptex
        int miptexStart = fileOfs + offsets[i];
        fseek(f, miptexStart, SEEK_SET);

        // 3. Read Header
        bspmiptex_t mt;
        fread(&mt, sizeof(bspmiptex_t), 1, f);

        // CHECK: Is it actually embedded?
        // If offsets[0] is 0 or -1, it's external.
        if (mt.offsets[0] <= 0) continue;

        // 4. Read Indices (Mip Level 0 only)
        // Offset is relative to the start of the bspmiptex_t struct
        int w = mt.width;
        int h = mt.height;
        std::vector<BYTE> indices(w * h);

        fseek(f, miptexStart + mt.offsets[0], SEEK_SET);
        fread(indices.data(), 1, w * h, f);

        // 5. Read Palette
        // The palette is located after the 4th mip level.
        // Size of 4 mips = w*h + (w*h)/4 + (w*h)/16 + (w*h)/64
        // Or simpler: offsets[3] + size_of_mip_3 + 2 bytes for count
        int offsetToPalette = mt.offsets[3] + (w * h / 64) + 2;

        fseek(f, miptexStart + offsetToPalette, SEEK_SET);
        BYTE palette[768];
        fread(palette, 1, 768, f);

        // 6. Create D3D Texture
        LPDIRECT3DTEXTURE9 pTex = NULL;
        if (FAILED(m_pDevice->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTex, NULL)))
            continue;

        // 7. Convert 8-bit to 32-bit ARGB
        D3DLOCKED_RECT rect;
        if (SUCCEEDED(pTex->LockRect(0, &rect, NULL, 0)))
        {
            BYTE* dest = (BYTE*)rect.pBits;
            bool isTransparent = (mt.name[0] == '{');

            for (int p = 0; p < w * h; p++)
            {
                int idx = indices[p];
                BYTE r = palette[idx * 3 + 0];
                BYTE g = palette[idx * 3 + 1];
                BYTE b = palette[idx * 3 + 2];
                BYTE a = 255;

                if (isTransparent && r == 0 && g == 0 && b == 255) a = 0;

                DWORD* pPixel = (DWORD*)dest;
                *pPixel = (a << 24) | (r << 16) | (g << 8) | b;
                dest += 4;
            }
            pTex->UnlockRect(0);

            // 8. Inject into Manager
            // So GetTexture("my_embedded_tex") will find this immediately
            std::wstring wname;
            {
                // Ensure null-termination and handle possible non-null-terminated names
                size_t len = strnlen(mt.name, 16);
                wname.assign(mt.name, mt.name + len);
            }
            m_pTextureMgr->AddTexture(wname, pTex);
        }
    }
}

void CHL1BSP::RenderEntities(CGizmo* pGizmo)
{ 
    if (!pGizmo) return;

    for (const auto& ent : m_entities)
    {
        std::string className = ent.Get("classname");
        std::string originStr = ent.Get("origin");

        // Skip entities without a position (like worldspawn logic)
        if (originStr.empty()) continue;

        // 1. Get Raw HL1 Coordinates
        D3DXVECTOR3 rawPos = ent.GetVector("origin");

        // 2. Convert to DirectX World Space (Scale & Swizzle)
        // HL1 (X, Y, Z) -> D3D (X, Z, Y)
        D3DXVECTOR3 pos;
        pos.x = rawPos.x * SCALE_FACTOR;
        pos.y = rawPos.z * SCALE_FACTOR; // Z becomes Y (Up)
        pos.z = rawPos.y * -SCALE_FACTOR; // Y becomes Z (Forward)

        // --------------------------------------------------------
        // VISUALIZE BASED ON TYPE
        // --------------------------------------------------------

        // A. Player Starts (Green Box)
        if (className.find("info_player") != std::string::npos)
        {
            // Draw a box representing the player size (approx 32x32x72 units)
            // Scaled down to match world scale
            float w = 16.0f * SCALE_FACTOR; // Half-width
            float h = 36.0f * SCALE_FACTOR; // Half-height

            // Assuming CGizmo::DrawCube(center, size, color)
            // Color: Green (0xFF00FF00)
            pGizmo->DrawCube(m_pDevice,pos, D3DXVECTOR3(w, h, w), 0xFF00FF00);

            // Optional: Draw Forward Vector based on "angle"
            float angleYaw = GetEntityAngle(ent);
            // Convert Yaw to Vector ...
        }

        // B. Lights (Yellow Sphere/Point)
        else if (className == "light" || className == "light_spot")
        {
            // Color: Yellow (0xFFFFFF00)
            float radius = 16.0f * SCALE_FACTOR;
            pGizmo->DrawGizmo(m_pDevice, pos, radius);

            // If it's a spot light, maybe draw a line showing direction
            // "target" key points to another entity name
        }

        // C. Weapons/Items (Blue Box)
        else if (className.find("weapon_") != std::string::npos ||
            className.find("ammo_") != std::string::npos ||
            className.find("item_") != std::string::npos)
        {
            float s = 16.0f * SCALE_FACTOR;
            pGizmo->DrawCube(m_pDevice,pos, D3DXVECTOR3(s, s, s), 0xFF0000FF);
        }

        // D. Generic/Unknown (Small White Point)
        else
        {
            float s = 8.0f * SCALE_FACTOR;
            pGizmo->DrawCube(m_pDevice,pos, D3DXVECTOR3(s, s, s), 0xFFFFFFFF);
        }
    }
}

float CHL1BSP::GetEntityAngle(const HL1Entity& ent)
{
    // HL1 entities usually use "angle" (scalar yaw) or "angles" (pitch yaw roll)
    std::string val = ent.Get("angle");
    if (!val.empty()) return (float)atof(val.c_str());

    // If "angle" is missing, sometimes "angles" is used
    // "0 90 0"
    val = ent.Get("angles");
    if (!val.empty()) {
        float p, y, r;
        sscanf_s(val.c_str(), "%f %f %f", &p, &y, &r);
        return y; // Return Yaw
    }
    return 0.0f;
}

