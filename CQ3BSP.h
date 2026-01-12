#pragma once
#include "stdafx.h"
#include "CArkiBlock.h"
#include "Q3BSPStructures.h"



class CQ3BSP
{
private:
	BYTE* m_pBuffer;

	// Data stored in dynamic vectors
	std::vector<BSPEntity>		m_entities;
	std::vector<dshader_t>      m_shaders;
	std::vector<dmodel_t>       m_models;
	std::vector<dplane_t>       m_planes;
	std::vector<dleaf_t>        m_leafs;
	std::vector<dnode_t>        m_nodes;
	std::vector<dsurface_t>     m_surfaces;
	std::vector<drawVert_t>     m_vertices;
	std::vector<int>            m_indexes;
	std::vector<BYTE>           m_lightBytes;
	std::vector<BYTE>           m_lightGrid;

	// --- Generated Patch Data ---
	std::vector<Q3BSPVertex>      m_patchVertices; // Converted directly to GPU format
	std::vector<int>            m_patchIndices;
	std::vector<PatchRenderInfo> m_patchRenderInfos;

	// --- DirectX 9 Resources ---
	LPDIRECT3DDEVICE9           m_pDevice;
	// Group 1: Static World
	LPDIRECT3DVERTEXBUFFER9     m_pVB_World;
	LPDIRECT3DINDEXBUFFER9      m_pIB_World;

	// Group 2: Bezier Patches
	LPDIRECT3DVERTEXBUFFER9     m_pVB_Patch;
	LPDIRECT3DINDEXBUFFER9      m_pIB_Patch;

	std::vector<LPDIRECT3DTEXTURE9> m_pLightmaps;

	// Bullet Collision Objects
	btDynamicsWorld* m_pdworld;
	btTriangleMesh* m_pTriangleMesh;
	btBvhTriangleMeshShape* m_pCollisionShape;
	btCollisionObject* m_pLevelObject;

public:
	CQ3BSP();
	~CQ3BSP();
	void Clear();

	BOOL Load(btDynamicsWorld* dynamicsWorld, const std::wstring& filename);
	void Render();
	BOOL InitGraphics(LPDIRECT3DDEVICE9 pDevice);
	void CreateLightmaps();

	void OnLostDevice();
	void OnResetDevice();

	// Getters return pointers to vector data or size
	int GetNumVertices() const { return static_cast<int>(m_vertices.size()); }
	int GetNumIndexes() const { return static_cast<int>(m_indexes.size()); }
	int GetNumSurfaces() const { return static_cast<int>(m_surfaces.size()); }

	// Direct access to data for rendering
	const drawVert_t* GetVertices() const { return m_vertices.data(); }
	const int* GetIndexes() const { return m_indexes.data(); }
	const dsurface_t* GetSurfaces() const { return m_surfaces.data(); }
	const dshader_t* GetShaders() const { return m_shaders.data(); }
	const BYTE* GetLightMapBytes() const { return m_lightBytes.data(); }
	const std::vector<BSPEntity>& GetEntities() const { return m_entities; }

	const float SCALE_FACTOR = 0.03f;


protected:
	BOOL	CopyHeader(dheader_t* header);
	int		CopyLump(dheader_t header, int lump, void* dest, int size);
	int		Flength(FILE* f);
	BOOL	LoadEntities(FILE* f, const dheader_t& h);
	// Helper to load specific lumps
	template <typename T>
	BOOL	LoadLump(FILE* f, const dheader_t& h, int lumpIndex, std::vector<T>& destVector);
	// Call this after the BSP and Radiosity are baked
	void InitPhysics(btDynamicsWorld* dynamicsWorld);
	void CleanupPhysics();

};

