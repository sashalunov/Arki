#pragma once

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class CXMesh
{
public:
	CXMesh(void);
	CXMesh(LPDIRECT3DDEVICE9 dev, TCHAR* pszFileName);
	~CXMesh(void);

	void Render(void);
	void Render(IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle);

	void Update(void);
	HRESULT Load(TCHAR *pszFileName);

	void SetPos(float x, float y, float z){m_vPos = D3DXVECTOR3(x, y, z);}
	void SetPos(const btVector3 &v){m_vPos = D3DXVECTOR3((FLOAT)v[0], (FLOAT)v[1], (FLOAT)v[2]);}
	void SetScale(float x, float y, float z){m_vScale = D3DXVECTOR3(x, y, z);}
	
	D3DXMATRIX GetWorld(){return m_matWorld;}
	void SaveAsOBJ(const char* filename);

private:
	LPDIRECT3DDEVICE9	m_pDev;
    LPD3DXMESH			m_pSysMemMesh;
    LPD3DXMESH			m_pLocalMesh;

	D3DXMATRIX		m_matWorld;
	D3DXMATRIX		m_matTranslation;
	D3DXMATRIX		m_matRotation;
	D3DXMATRIX		m_matScaling;

    TCHAR*		m_pszFileName;
    BOOL		m_bUseMaterials;
    DWORD		m_dwNumMaterials;

	D3DXVECTOR3		m_vPos;
	D3DXVECTOR3		m_vRot;
	D3DXVECTOR3		m_vScale;

	BOOL	m_bVisible;
	BOOL	m_bActive;

};

