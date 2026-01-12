#include "CQ3BSP.h"
#include "D3DRender.h"
#include "BezierTessellator.h"
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
CQ3BSP::CQ3BSP(void)
{
	m_pBuffer = NULL;
	m_pDevice = NULL;
	m_pVB_World = NULL;
	m_pIB_World = NULL;
	m_pVB_Patch = NULL;
	m_pIB_Patch = NULL;
	m_pdworld = NULL;
	m_pCollisionShape = NULL;
	m_pTriangleMesh = NULL;
	m_pLevelObject = NULL;
	Clear();
	OnLostDevice();
}

CQ3BSP::~CQ3BSP(void)
{
	CleanupPhysics();

	SAFE_DELETE(m_pBuffer);
	Clear();
	OnLostDevice();
}

void CQ3BSP::Clear()
{
	m_entities.clear();
	m_shaders.clear();
	m_surfaces.clear();
	m_vertices.clear();
	m_indexes.clear();
	m_models.clear();
	m_leafs.clear();
	m_nodes.clear();
	m_planes.clear();
	m_lightBytes.clear();
	m_patchVertices.clear();
	m_patchIndices.clear();
	m_patchRenderInfos.clear();
}

// ----------------------------------------------------------------------------
// LOAD ENTITIES (Parses the ASCII buffer)
// ----------------------------------------------------------------------------
BOOL CQ3BSP::LoadEntities(FILE* f, const dheader_t& h)
{
	m_entities.clear();

	int fileLen = h.lumps[LUMP_ENTITIES].filelen;
	int fileOfs = h.lumps[LUMP_ENTITIES].fileofs;

	if (fileLen == 0) return true; // No entities is technically valid

	// 1. Read the raw string data
	std::vector<char> buffer(fileLen + 1); // +1 for null terminator
	if (fseek(f, fileOfs, SEEK_SET) != 0) return FALSE;
	if (fread(buffer.data(), 1, fileLen, f) != (size_t)fileLen) return FALSE;
	buffer[fileLen] = '\0'; // Ensure null termination

	// 2. Parse the buffer
	const char* pData = buffer.data();
	BSPEntity currentEntity;
	BOOL parsingEntity = FALSE;

	while (*pData)
	{
		// Skip whitespace
		if (*pData <= ' ') {
			pData++;
			continue;
		}

		// Start of Entity
		if (*pData == '{')
		{
			currentEntity.properties.clear();
			parsingEntity = true;
			pData++;
			continue;
		}

		// End of Entity
		if (*pData == '}')
		{
			if (parsingEntity) {
				m_entities.push_back(currentEntity);
				parsingEntity = false;
			}
			pData++;
			continue;
		}

		// Key-Value Pair (Both must be quoted)
		if (*pData == '\"')
		{
			std::string key, value;

			// Parse Key
			pData++; // Skip opening quote
			while (*pData && *pData != '\"') {
				key += *pData++;
			}
			if (*pData == '\"') pData++; // Skip closing quote

			// Skip whitespace between key and value
			while (*pData && *pData <= ' ') pData++;

			// Parse Value
			if (*pData == '\"')
			{
				pData++; // Skip opening quote
				while (*pData && *pData != '\"') {
					value += *pData++;
				}
				if (*pData == '\"') pData++; // Skip closing quote
			}

			// Store
			if (parsingEntity && !key.empty()) {
				currentEntity.properties[key] = value;
			}
			continue;
		}

		// Skip unknown characters
		pData++;
	}

	return TRUE;
}

// Template to safely load a lump into a vector
template <typename T>
BOOL CQ3BSP::LoadLump(FILE* f, const dheader_t& h, int lumpIndex, std::vector<T>& destVector)
{
	int fileLen = h.lumps[lumpIndex].filelen;
	int fileOfs = h.lumps[lumpIndex].fileofs;

	if (fileLen % sizeof(T) != 0) return false; // Corrupt data size

	int numElements = fileLen / sizeof(T);
	destVector.resize(numElements);

	if (numElements > 0)
	{
		fseek(f, fileOfs, SEEK_SET);
		size_t read = fread(destVector.data(), sizeof(T), numElements, f);

		if (read != numElements) 
			return false; // Read error
	}
	return TRUE;
}
//==============================================================================
//==============================================================================
int CQ3BSP::Flength(FILE* f)
{
	int		pos;
	int		end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}
BOOL CQ3BSP::CopyHeader(dheader_t* header)
{
	memcpy(header, m_pBuffer, sizeof(dheader_t));

	if (header->ident != BSP_IDENT)
		return FALSE;

	if (header->version != BSP_VERSION)
		return FALSE;

	return TRUE;
}
int CQ3BSP::CopyLump(dheader_t header, int lump, void* dest, int size)
{
	int		filelen;
	int		fileofs;

	filelen = header.lumps[lump].filelen;
	fileofs = header.lumps[lump].fileofs;

	if (filelen % size)
		return -1;

	memcpy(dest, m_pBuffer + fileofs, filelen);

	return filelen / size;
}
// ----------------------------------------------------------------------------
BOOL CQ3BSP::Load(btDynamicsWorld* dynamicsWorld, const std::wstring & filename)
{
	Clear();
	OnLostDevice();

	FILE* file = _wfopen(filename.c_str(), L"rb");
	if (!file) return false;

	dheader_t header;
	if (fread(&header, sizeof(dheader_t), 1, file) != 1)
	{
		fclose(file);
		return false;
	}

	if (header.ident != BSP_IDENT || header.version != BSP_VERSION)
	{
		fclose(file);
		return false;
	}

	// Load lumps directly from file into vectors
	// No massive buffer allocation, no hardcoded limits
	BOOL success = true;
	success &= LoadEntities(file, header);
	success &= LoadLump(file, header, LUMP_SHADERS, m_shaders);
	success &= LoadLump(file, header, LUMP_MODELS, m_models);
	success &= LoadLump(file, header, LUMP_PLANES, m_planes);
	success &= LoadLump(file, header, LUMP_LEAFS, m_leafs);
	success &= LoadLump(file, header, LUMP_NODES, m_nodes);
	success &= LoadLump(file, header, LUMP_SURFACES, m_surfaces);
	success &= LoadLump(file, header, LUMP_DRAWVERTS, m_vertices);
	success &= LoadLump(file, header, LUMP_DRAWINDEXES, m_indexes);

	BezierTessellator tessellator;
	int tessLevel = 8; // 10 is decent quality. 
// We can't easily insert into vectors while iterating indices, 
// so we append to end and fix offsets.
	for (int i = 0; i < (int)m_surfaces.size(); ++i)
	{
		const dsurface_t& surf = m_surfaces[i];

		if (surf.surfaceType == MST_PATCH)
		{
			// Gather controls
			std::vector<drawVert_t> controls(surf.numVerts);
			for (int k = 0; k < surf.numVerts; k++) 
			{
				controls[k] = m_vertices[surf.firstVert + k];
			}

			// Generate
			tessellator.TessellatePatch(controls.data(), surf.patchWidth, surf.patchHeight, tessLevel);

			// Record Render Info
			PatchRenderInfo info;
			info.originalSurfaceIndex = i;
			info.startIndex = (int)m_patchIndices.size(); // Offset in NEW Index Buffer
			info.minVertIndex = (int)m_patchVertices.size(); // Offset in NEW Vertex Buffer
			info.numVerts = (int)tessellator.m_outVerts.size();
			info.primitiveCount = (int)tessellator.m_outIndices.size() / 3;

			// Store Data
			// Mapped 1:1, indices are 0-based relative to this patch's vertices
			// We need to offset indices by the current size of m_patchVertices
			int vertOffset = (int)m_patchVertices.size();

			m_patchVertices.insert(m_patchVertices.end(), tessellator.m_outVerts.begin(), tessellator.m_outVerts.end());

			for (int idx : tessellator.m_outIndices) 
			{
				m_patchIndices.push_back(idx + vertOffset);
			}

			m_patchRenderInfos.push_back(info);
		}
	}

	// Lightmaps are just bytes, treat slightly differently or specialize template
	// Note: Lightmaps are usually 128x128x3 bytes per block
	int lmLen = header.lumps[LUMP_LIGHTMAPS].filelen;
	if (lmLen > 0) {
		m_lightBytes.resize(lmLen);
		fseek(file, header.lumps[LUMP_LIGHTMAPS].fileofs, SEEK_SET);
		fread(m_lightBytes.data(), 1, lmLen, file);
	}

	fclose(file);

	success &= InitGraphics(d3d9->GetDevice());

	InitPhysics(dynamicsWorld);

	return success;
}

BOOL CQ3BSP::InitGraphics(LPDIRECT3DDEVICE9 pDevice)
{
	// Quake maps are huge. 0.03f is a common scale for D3D games.
	m_pDevice = pDevice;
	if (!pDevice || m_vertices.empty())
		return false;
	// --- 1. Create WORLD Buffers (Static BSP geometry) ---
	if (!m_vertices.empty())
	{
		m_pDevice->CreateVertexBuffer((UINT)m_vertices.size() * sizeof(Q3BSPVertex),
			D3DUSAGE_WRITEONLY, D3DFVF_Q3BSPVERTEX, D3DPOOL_MANAGED, &m_pVB_World, NULL);

		Q3BSPVertex* pVerts;
		m_pVB_World->Lock(0, 0, (void**)&pVerts, 0);
		for (size_t i = 0; i < m_vertices.size(); i++) {
			// Convert drawVert_t to BSPVertex here (swizzle coords)
			//pVerts[i].pos = m_vertices[i].xyz;
			pVerts[i].pos.x = m_vertices[i].xyz.x * -SCALE_FACTOR;;
			pVerts[i].pos.y = m_vertices[i].xyz.z * SCALE_FACTOR;;
			pVerts[i].pos.z = m_vertices[i].xyz.y * SCALE_FACTOR;;

			pVerts[i].normal = m_vertices[i].normal;
			pVerts[i].color = D3DCOLOR_RGBA(m_vertices[i].color[0], m_vertices[i].color[1], m_vertices[i].color[2], m_vertices[i].color[3]);
			pVerts[i].uv0[0] = m_vertices[i].st[0];
			pVerts[i].uv0[1] = m_vertices[i].st[1];
			pVerts[i].uv1[0] = m_vertices[i].lightmap[0];
			pVerts[i].uv1[1] = m_vertices[i].lightmap[1];
		}
		m_pVB_World->Unlock();

		m_pDevice->CreateIndexBuffer(m_indexes.size() * sizeof(int),
			D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_pIB_World, NULL);

		void* pInds;
		m_pIB_World->Lock(0, 0, &pInds, 0);
		memcpy(pInds, m_indexes.data(), m_indexes.size() * sizeof(int));
		m_pIB_World->Unlock();
	}

	// --- 2. Create PATCH Buffers (Generated geometry) ---
	if (!m_patchVertices.empty())
	{
		m_pDevice->CreateVertexBuffer((UINT)m_patchVertices.size() * sizeof(Q3BSPVertex),
			D3DUSAGE_WRITEONLY, D3DFVF_Q3BSPVERTEX, D3DPOOL_MANAGED, &m_pVB_Patch, NULL);

		void* pVerts;
		Q3BSPVertex* pPatchVerts;
		if (SUCCEEDED(m_pVB_Patch->Lock(0, 0, (void**)&pPatchVerts, 0)))
		{
			for (size_t i = 0; i < m_patchVertices.size(); i++) {
				// The patch vertices are already BSPVertex, but they are in Q3 space
				// because the Tessellator calculated them from raw Q3 data.
				Q3BSPVertex src = m_patchVertices[i]; // Copy original

				pPatchVerts[i] = src; // Init
				// Apply Scale & Swizzle
				pPatchVerts[i].pos.x = src.pos.x * -SCALE_FACTOR;
				pPatchVerts[i].pos.y = src.pos.z * SCALE_FACTOR; // Swap Y/Z
				pPatchVerts[i].pos.z = src.pos.y * SCALE_FACTOR;

				pPatchVerts[i].normal.x = src.normal.x;
				pPatchVerts[i].normal.y = src.normal.z; // Swap Y/Z
				pPatchVerts[i].normal.z = src.normal.y;
			}
			m_pVB_Patch->Unlock();
		}
		//memcpy(pVerts, m_patchVertices.data(), m_patchVertices.size() * sizeof(Q3BSPVertex));
		//m_pVB_Patch->Unlock();

		m_pDevice->CreateIndexBuffer(m_patchIndices.size() * sizeof(int),
			D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_pIB_Patch, NULL);

		void* pInds;
		m_pIB_Patch->Lock(0, 0, &pInds, 0);
		memcpy(pInds, m_patchIndices.data(), m_patchIndices.size() * sizeof(int));
		m_pIB_Patch->Unlock();
	}

	return true;
}

void CQ3BSP::OnLostDevice()
{
	if (m_pVB_World) { m_pVB_World->Release(); m_pVB_World = NULL; }
	if (m_pIB_World) { m_pIB_World->Release(); m_pIB_World = NULL; }
	if (m_pVB_Patch) { m_pVB_Patch->Release(); m_pVB_Patch = NULL; }
	if (m_pIB_Patch) { m_pIB_Patch->Release(); m_pIB_Patch = NULL; }
}

void CQ3BSP::OnResetDevice()
{
	if (!m_pDevice)
		return;
	InitGraphics(m_pDevice);
}

// ----------------------------------------------------------------------------
// RENDER
// ----------------------------------------------------------------------------
void CQ3BSP::Render()
{
	if (!m_pDevice || !m_pVB_World || !m_pVB_Patch) return;

	m_pDevice->SetFVF(D3DFVF_Q3BSPVERTEX);
	// Render States (Basic)
	m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE); // Q3 maps use lightmaps
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); // Check winding order
	m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);



	// --------------------------------------------------------
	// PASS 1: Static World (Walls, Floors)
	// --------------------------------------------------------
	if (m_pVB_World)
	{
		m_pDevice->SetStreamSource(0, m_pVB_World, 0, sizeof(Q3BSPVertex));
		m_pDevice->SetIndices(m_pIB_World);

		for (const auto& surf : m_surfaces)
		{
			if (surf.surfaceType == MST_PLANAR || surf.surfaceType == MST_TRIANGLE_SOUP)
			{
				// Apply Textures/Shaders based on surf.shaderNum here...

				m_pDevice->DrawIndexedPrimitive(
					D3DPT_TRIANGLELIST,
					surf.firstVert,                  
					0,     
					(UINT)m_vertices.size(),
					surf.firstIndex,    
					surf.numIndexes / 3 
				);
			}
		}
	}

	// --------------------------------------------------------
	// PASS 2: Bezier Patches
	// --------------------------------------------------------
	if (m_pVB_Patch && !m_patchRenderInfos.empty())
	{
		m_pDevice->SetStreamSource(0, m_pVB_Patch, 0, sizeof(Q3BSPVertex));
		m_pDevice->SetIndices(m_pIB_Patch);

		for (const auto& info : m_patchRenderInfos)
		{
			// Get original surface to bind correct texture
			// const dsurface_t& originalSurf = m_surfaces[info.originalSurfaceIndex];
			// BindTexture(originalSurf.shaderNum);

			m_pDevice->DrawIndexedPrimitive(
				D3DPT_TRIANGLELIST,
				0,                  // BaseVertexIndex (Indices are already offset in our list)
				0,  // MinVertexIndex
				(UINT)m_patchVertices.size(),       // NumVertices
				info.startIndex,    // StartIndex in IB
				info.primitiveCount // PrimitiveCount
			);
		}
	}

	//m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); // Check winding order

}
void CQ3BSP::InitPhysics(btDynamicsWorld* dynamicsWorld)
{
	CleanupPhysics();

	m_pdworld = dynamicsWorld;
	// 1. Create the Triangle Mesh interface
	m_pTriangleMesh = new btTriangleMesh(true, false);
	// -----------------------------------------------------------------------
	// PASS 1: Static World Geometry (Walls/Floors)
	// -----------------------------------------------------------------------
	for (const auto& surf : m_surfaces)
	{
		// Type 1 (Planar) and 3 (Mesh) are solid. 
		// Skip patches (2) here, we handle them in Pass 2.
		if (surf.surfaceType == 1 || surf.surfaceType == 3)
		{
			int numTriangles = surf.numIndexes / 3;

			for (int i = 0; i < numTriangles; i++)
			{
				// Get indices from the Index Buffer
				int idx0 = m_indexes[surf.firstIndex + i * 3 + 0] + surf.firstVert;
				int idx1 = m_indexes[surf.firstIndex + i * 3 + 1] + surf.firstVert;
				int idx2 = m_indexes[surf.firstIndex + i * 3 + 2] + surf.firstVert;

				// Get Vertices & Apply Swizzle/Scale
				// Q3 Z-up -> Bullet Y-up (x, z, y)
				btVector3 v0(
					m_vertices[idx0].xyz.x * -SCALE_FACTOR,
					m_vertices[idx0].xyz.z * SCALE_FACTOR,
					m_vertices[idx0].xyz.y * SCALE_FACTOR
				);
				btVector3 v1(
					m_vertices[idx1].xyz.x * -SCALE_FACTOR,
					m_vertices[idx1].xyz.z * SCALE_FACTOR,
					m_vertices[idx1].xyz.y * SCALE_FACTOR
				);
				btVector3 v2(
					m_vertices[idx2].xyz.x * -SCALE_FACTOR,
					m_vertices[idx2].xyz.z * SCALE_FACTOR,
					m_vertices[idx2].xyz.y * SCALE_FACTOR
				);

				m_pTriangleMesh->addTriangle(v0, v1, v2);
			}
		}
	}
	// -----------------------------------------------------------------------
	// PASS 2: Tessellated Patches (Curves)
	// -----------------------------------------------------------------------
	// Since m_patchVertices is a flat list of triangles, we can iterate simply.
	int numPatchTriangles = (int)m_patchIndices.size() / 3;

	for (int i = 0; i < numPatchTriangles; i++)
	{
		int idx0 = m_patchIndices[i * 3 + 0];
		int idx1 = m_patchIndices[i * 3 + 1];
		int idx2 = m_patchIndices[i * 3 + 2];

		// Access the generated patch vertices
		// NOTE: m_patchVertices contains raw Q3 data (before D3D swizzle), 
		// so we must swizzle/scale them here too.
		const Q3BSPVertex& p0 = m_patchVertices[idx0];
		const Q3BSPVertex& p1 = m_patchVertices[idx1];
		const Q3BSPVertex& p2 = m_patchVertices[idx2];

		btVector3 v0(p0.pos.x * -SCALE_FACTOR, p0.pos.z * SCALE_FACTOR, p0.pos.y * SCALE_FACTOR);
		btVector3 v1(p1.pos.x * -SCALE_FACTOR, p1.pos.z * SCALE_FACTOR, p1.pos.y * SCALE_FACTOR);
		btVector3 v2(p2.pos.x * -SCALE_FACTOR, p2.pos.z * SCALE_FACTOR, p2.pos.y * SCALE_FACTOR);

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

void CQ3BSP::CleanupPhysics()
{
	if (m_pdworld && m_pLevelObject) {
		m_pdworld->removeCollisionObject(m_pLevelObject);
		delete m_pLevelObject;
	}
	SAFE_DELETE(m_pCollisionShape);
	SAFE_DELETE(m_pTriangleMesh);
}

void CQ3BSP::CreateLightmaps()
{
	if (!m_pDevice || m_lightBytes.empty()) return;

	// 1. Calculate how many lightmaps exist
	// Each lightmap is 128x128 pixels * 3 bytes (RGB)
	const int LIGHTMAP_SIZE = 128 * 128 * 3;
	int numLightmaps = m_lightBytes.size() / LIGHTMAP_SIZE;

	// 2. Load each lightmap
	for (int i = 0; i < numLightmaps; i++)
	{
		LPDIRECT3DTEXTURE9 pTex = NULL;

		// Create 128x128 Texture, 32-bit Color (X8R8G8B8)
		if (FAILED(m_pDevice->CreateTexture(128, 128, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pTex, NULL)))
			continue;

		// Lock and Fill
		D3DLOCKED_RECT rect;
		if (SUCCEEDED(pTex->LockRect(0, &rect, NULL, 0)))
		{
			BYTE* pDst = (BYTE*)rect.pBits;
			const BYTE* pSrc = &m_lightBytes[i * LIGHTMAP_SIZE];

			for (int y = 0; y < 128; y++)
			{
				DWORD* row = (DWORD*)(pDst + y * rect.Pitch);
				for (int x = 0; x < 128; x++)
				{
					// Get Raw RGB
					int r = pSrc[0];
					int g = pSrc[1];
					int b = pSrc[2];

					// OPTIONAL: Gamma Correction / Brightness Boost
					// Q3 maps can look dark without this.
					// Simple 2x brighten (clamped to 255)
					/*
					r = min(255, r * 2);
					g = min(255, g * 2);
					b = min(255, b * 2);
					*/

					// Write XRGB (Little Endian: B G R X)
					row[x] = 0xFF000000 | (r << 16) | (g << 8) | b;

					pSrc += 3; // Move to next RGB triplet
				}
			}
			pTex->UnlockRect(0);
		}

		m_pLightmaps.push_back(pTex);
	}
}
