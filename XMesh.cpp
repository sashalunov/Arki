
#include "StdAfx.h"
#include "Logger.h"
#include "D3DRender.h"

#include "btBulletDynamicsCommon.h"

#include "XMesh.h"


CXMesh::CXMesh()
{
	m_bVisible = 0;
	m_bActive = 0;

	m_pSysMemMesh = NULL;
	m_pLocalMesh = NULL;

	m_pDev			 = d3d9->GetDevice();
	m_pszFileName    = NULL;

	m_vPos	 = D3DXVECTOR3(0, 0, 0);
	m_vRot	 = D3DXVECTOR3(0, 0, 0);
	m_vScale = D3DXVECTOR3(1, 1, 1);

	D3DXMatrixIdentity(&m_matWorld);
	D3DXMatrixIdentity(&m_matTranslation);	
	D3DXMatrixIdentity(&m_matRotation);
	D3DXMatrixIdentity(&m_matScaling);

}

// ----------------------------------------------------------------------------
CXMesh::CXMesh(LPDIRECT3DDEVICE9 dev, TCHAR* pszFileName) 
{
	m_bVisible = 0;
	m_bActive = 0;

	m_pSysMemMesh = NULL;
	m_pLocalMesh = NULL;

	m_pDev = dev;
	m_pszFileName = pszFileName;

	m_vPos			= D3DXVECTOR3(0, 0, 0);
	m_vRot			= D3DXVECTOR3(0, 0, 0);
	m_vScale		= D3DXVECTOR3(1, 1, 1);

	Load(pszFileName);

	D3DXMatrixIdentity(&m_matWorld);
	D3DXMatrixIdentity(&m_matTranslation);	
	D3DXMatrixIdentity(&m_matRotation);
	D3DXMatrixIdentity(&m_matScaling);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
CXMesh::~CXMesh(void)
{
    //SAFE_DELETE_ARRAY(m_pTextures);
    //SAFE_DELETE_ARRAY(m_pMaterials);
	SAFE_RELEASE(m_pSysMemMesh);
	SAFE_RELEASE(m_pLocalMesh);

}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
HRESULT CXMesh::Load(TCHAR *pszFileName)
{
	// FIX: Ensure we don't leak an existing mesh if Load is called twice
	SAFE_RELEASE(m_pSysMemMesh);
	SAFE_RELEASE(m_pLocalMesh); // If you use this later

    HRESULT hr;
    LPD3DXBUFFER pAdjacencyBuffer	= NULL;
    LPD3DXBUFFER pMtrlBuffer		= NULL;
	m_pszFileName = pszFileName;

	hr = D3DXLoadMeshFromX(m_pszFileName, D3DXMESH_MANAGED , m_pDev, 
		&pAdjacencyBuffer, &pMtrlBuffer, NULL, &m_dwNumMaterials, &m_pSysMemMesh);
	if(FAILED(hr))
	{
		Log->Log(L"Failed to load %s\n", m_pszFileName);
		return hr;
	}

	hr = m_pSysMemMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE,
		(DWORD*)pAdjacencyBuffer->GetBufferPointer(), NULL, NULL, NULL);

	SAFE_RELEASE(pAdjacencyBuffer);
	SAFE_RELEASE(pMtrlBuffer);

	if(FAILED(hr))
	{
       // SAFE_RELEASE(pAdjacencyBuffer);
        //SAFE_RELEASE(pMtrlBuffer);
		return hr;
	}
/*	if(pMtrlBuffer && m_dwNumMaterials > 0)
	{
        D3DXMATERIAL* d3dxMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
        m_pMaterials = new D3DMATERIAL9[m_dwNumMaterials];
        m_pTextures  = new LPDIRECT3DTEXTURE9[m_dwNumMaterials];

        for(DWORD i=0; i<m_dwNumMaterials; i++)
		{
            m_pMaterials[i]	= d3dxMtrls[i].MatD3D;
            m_pTextures[i]	= NULL;

            if( d3dxMtrls[i].pTextureFilename )
            {
				m_pMaterials[i].Diffuse.a = 1;
				m_pMaterials[i].Diffuse.r = 1;
				m_pMaterials[i].Diffuse.g = 1;
				m_pMaterials[i].Diffuse.b = 1;

				if(FAILED(D3DXCreateTextureFromFileEx(m_pDev, d3dxMtrls[i].pTextureFilename, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
					0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_BOX, 0, NULL, NULL, &m_pTextures[i])))
					m_pTextures[i] = NULL;
			}
		}
	}*/

	m_bVisible = TRUE;
	m_bActive = TRUE;

	return S_OK;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CXMesh::Render(void)
{
	if(!m_bVisible)
		return;
	
	if(!m_pSysMemMesh)
		return;

	//m_pDev->SetTransform( D3DTS_WORLD, &m_matWorld );

	/*if(!bUseMaterials)
	{
		m_pDev->SetMaterial(&m_pMat);
		m_pDev->SetTexture(0, NULL);
	}*/

	for(DWORD i=0; i<m_dwNumMaterials; i++ )
	{
		m_pSysMemMesh->DrawSubset(i);
	}
}
// ----------------------------------------------------------------------------
void CXMesh::Render(IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle)
{
	if (!m_bVisible)
		return;

	if (!m_pSysMemMesh)
		return;

	m_pDev->SetTexture(0, pReflectionTexture);

	// MAGIC: Tell DX9 to automatically calculate the Reflection Vector based on Camera Angle
	m_pDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	// This rotates the 3D reflection vector around the Y axis
	D3DXMATRIX matTextureRot;
	D3DXMatrixRotationY(&matTextureRot, rotationAngle);
	// C. Apply the Matrix to Texture Stage 0
	m_pDev->SetTransform(D3DTS_TEXTURE0, &matTextureRot);
	// Tell DX9 this is a Cube Map (requires 3D coordinates)
	m_pDev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);

	// Optional: Mix the reflection with the material color (Modulate or Add)
	m_pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD); // Add makes it look shiny/glowing

	for (DWORD i = 0; i < m_dwNumMaterials; i++)
	{
		m_pSysMemMesh->DrawSubset(i);
	}


}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CXMesh::Update(void)
{
	if(!m_bActive)return;

	D3DXMatrixIdentity(&m_matWorld);
	D3DXMatrixIdentity(&m_matTranslation);
	D3DXMatrixIdentity(&m_matRotation);
	D3DXMatrixIdentity(&m_matScaling);

	D3DXMatrixTranslation(&m_matTranslation, m_vPos.x, m_vPos.y, m_vPos.z);
	D3DXMatrixRotationYawPitchRoll(&m_matRotation, m_vRot.y, m_vRot.x, m_vRot.z);
	D3DXMatrixScaling(&m_matScaling, m_vScale.x, m_vScale.y, m_vScale.z);

	D3DXMatrixMultiply(&m_matWorld, &m_matWorld, &m_matTranslation);
	D3DXMatrixMultiply(&m_matWorld, &m_matWorld, &m_matRotation);
	D3DXMatrixMultiply(&m_matWorld, &m_matWorld, &m_matScaling);

}

// ----------------------------------------------------------------------------
// Save the current system memory mesh to a Wavefront OBJ file
// ----------------------------------------------------------------------------
void CXMesh::SaveAsOBJ(const char* filename)
{
	if (!m_pSysMemMesh)
		return;

	std::ofstream outFile(filename);
	if (!outFile.is_open())
	{
		Log->Log(L"Failed to open output file for OBJ export.\n");
		return;
	}

	outFile << "# Exported from CXMesh" << std::endl;

	// 1. Clone the mesh to a standard FVF (Position | Normal | TexCoord1)
	//    This ensures we can safely read the data struct without knowing the original FVF.
	LPD3DXMESH pTempMesh = NULL;
	DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

	// Clone into System Memory so we can lock it efficiently
	if (FAILED(m_pSysMemMesh->CloneMeshFVF(D3DXMESH_SYSTEMMEM, fvf, m_pDev, &pTempMesh)))
	{
		Log->Log(L"Failed to clone mesh for export.\n");
		return;
	}

	// 2. Define a struct that matches the FVF we requested above
	struct ExportVertex {
		D3DXVECTOR3 p;
		D3DXVECTOR3 n;
		float u, v;
	};

	ExportVertex* pVerts = NULL;

	// 3. Write Vertex Data
	if (SUCCEEDED(pTempMesh->LockVertexBuffer(0, (void**)&pVerts)))
	{
		DWORD numVerts = pTempMesh->GetNumVertices();

		// Write Positions (v x y z)
		for (DWORD i = 0; i < numVerts; i++)
		{
			// Note: D3D is Left-Handed, OBJ is Right-Handed. 
			// For a raw dump we keep it as-is, but you might want to negate Z (p.z * -1) 
			// if importing into Blender/Max/Maya.
			outFile << "v " << pVerts[i].p.x << " " << pVerts[i].p.y << " " << pVerts[i].p.z << std::endl;
		}

		// Write Texture Coordinates (vt u v)
		for (DWORD i = 0; i < numVerts; i++)
		{
			// OBJ V-coordinate is typically 0 at bottom, DirectX is 0 at top.
			// We invert V to match standard OBJ viewers.
			outFile << "vt " << pVerts[i].u << " " << (1.0f - pVerts[i].v) << std::endl;
		}

		// Write Normals (vn x y z)
		for (DWORD i = 0; i < numVerts; i++)
		{
			outFile << "vn " << pVerts[i].n.x << " " << pVerts[i].n.y << " " << pVerts[i].n.z << std::endl;
		}

		pTempMesh->UnlockVertexBuffer();
	}

	// 4. Write Face Data (Indices)
	void* pIndices = NULL;
	if (SUCCEEDED(pTempMesh->LockIndexBuffer(0, &pIndices)))
	{
		DWORD numFaces = pTempMesh->GetNumFaces();
		bool b32Bit = (pTempMesh->GetOptions() & D3DXMESH_32BIT) != 0;

		for (DWORD i = 0; i < numFaces; i++)
		{
			DWORD idx[3];

			// Read indices based on 16-bit or 32-bit buffer size
			if (b32Bit) {
				DWORD* ptr = (DWORD*)pIndices;
				idx[0] = ptr[i * 3 + 0];
				idx[1] = ptr[i * 3 + 1];
				idx[2] = ptr[i * 3 + 2];
			}
			else {
				WORD* ptr = (WORD*)pIndices;
				idx[0] = ptr[i * 3 + 0];
				idx[1] = ptr[i * 3 + 1];
				idx[2] = ptr[i * 3 + 2];
			}

			// OBJ indices are 1-based, D3D is 0-based.
			// Format is f v/vt/vn. Since we dumped v, vt, vn in the same order,
			// the indices for all three attributes are identical.
			outFile << "f "
				<< (idx[0] + 1) << "/" << (idx[0] + 1) << "/" << (idx[0] + 1) << " "
				<< (idx[1] + 1) << "/" << (idx[1] + 1) << "/" << (idx[1] + 1) << " "
				<< (idx[2] + 1) << "/" << (idx[2] + 1) << "/" << (idx[2] + 1) << std::endl;
		}
		pTempMesh->UnlockIndexBuffer();
	}

	// Cleanup
	pTempMesh->Release();
	outFile.close();
}
