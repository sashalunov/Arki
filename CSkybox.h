#pragma once

//--------------------------------------------------------------------------------------
// This is the vertex format used for the skybox.
//--------------------------------------------------------------------------------------
struct SKYBOXVERT
{
    D3DXVECTOR3 pos;    // Position
    D3DXVECTOR3 tex;    // Texcoord

    const static D3DVERTEXELEMENT9 Decl[3];
};


VOID WINAPI ProceduralSpaceGen(D3DXVECTOR4* pOut, const D3DXVECTOR3* pTexCoord, const D3DXVECTOR3* pTexelSize, LPVOID pData);

//--------------------------------------------------------------------------------------
class CSkybox
{
    IDirect3DCubeTexture9* g_pSkyboxTexture = NULL;
    float m_fCurrentRotationY;     // Accumulated Y rotation angle
    const float m_fRotationSpeed;  // Speed of rotation in radians/second

    HRESULT GenerateSkyboxTexture(IDirect3DDevice9* pd3dDevice, UINT size = 512)
    {
        // 1. Create an empty Cube Texture
        // Size 512x512 is usually enough for generated noise
        HRESULT hr = D3DXCreateCubeTexture(pd3dDevice, size, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_pSkyboxTexture);

        if (FAILED(hr)) return hr;

        // 2. Fill it using our procedural function
        // This automates the loop over all 6 faces and X,Y pixels
        return D3DXFillCubeTexture(g_pSkyboxTexture, ProceduralSpaceGen, NULL);
        //return S_OK;
    }
    HRESULT InitSkyboxTexture(IDirect3DDevice9* pd3dDevice, TCHAR* filepath)
    {
        // D3DX function to load a cube map from file (Unicode version)
        return D3DXCreateCubeTextureFromFile(pd3dDevice, filepath, &g_pSkyboxTexture);
    }

    IDirect3DVertexBuffer9* g_pSkyboxVB = NULL;
public:
    HRESULT InitSkyboxGeometry(IDirect3DDevice9* pd3dDevice)
    {
        HRESULT hr = pd3dDevice->CreateVertexBuffer(36 * sizeof(SKYBOXVERT),
            0, 0, D3DPOOL_MANAGED, &g_pSkyboxVB, NULL);
        if (FAILED(hr)) return hr;

        SKYBOXVERT* pVerts;
        g_pSkyboxVB->Lock(0, 0, (void**)&pVerts, 0);

        // Define the unit cube distance
        float dist = 100.0f;

        // Helper to set a vertex quickly
        // Note: For a skybox, texture coord (UVW) is usually the same as Position (XYZ)
        auto SetVert = [&](int idx, float x, float y, float z) {
            pVerts[idx].pos = D3DXVECTOR3(x, y, z) * dist;
            pVerts[idx].tex = D3DXVECTOR3(x, y, z); // Normalized direction
            };

        // FRONT Face
        SetVert(0, -1, -1, 1); SetVert(1, -1, 1, 1); SetVert(2, 1, -1, 1);
        SetVert(3, 1, -1, 1); SetVert(4, -1, 1, 1); SetVert(5, 1, 1, 1);

        // BACK Face
        SetVert(6, 1, -1, -1); SetVert(7, 1, 1, -1); SetVert(8, -1, -1, -1);
        SetVert(9, -1, -1, -1); SetVert(10, 1, 1, -1); SetVert(11, -1, 1, -1);

        // LEFT Face
        SetVert(12, -1, -1, -1); SetVert(13, -1, 1, -1); SetVert(14, -1, -1, 1);
        SetVert(15, -1, -1, 1); SetVert(16, -1, 1, -1); SetVert(17, -1, 1, 1);

        // RIGHT Face
        SetVert(18, 1, -1, 1); SetVert(19, 1, 1, 1); SetVert(20, 1, -1, -1);
        SetVert(21, 1, -1, -1); SetVert(22, 1, 1, 1); SetVert(23, 1, 1, -1);

        // TOP Face
        SetVert(24, -1, 1, 1); SetVert(25, -1, 1, -1); SetVert(26, 1, 1, 1);
        SetVert(27, 1, 1, 1); SetVert(28, -1, 1, -1); SetVert(29, 1, 1, -1);

        // BOTTOM Face
        SetVert(30, -1, -1, -1); SetVert(31, -1, -1, 1); SetVert(32, 1, -1, -1);
        SetVert(33, 1, -1, -1); SetVert(34, -1, -1, 1); SetVert(35, 1, -1, 1);

       return g_pSkyboxVB->Unlock();
    }

	CSkybox::CSkybox()
        : m_fCurrentRotationY(0.0f)
        , m_fRotationSpeed(0.01f) // Example: 0.01 radians per second (approx 0.57 degrees/sec)
    {}
    CSkybox::CSkybox(IDirect3DDevice9* pd3dDevice, TCHAR* skyboxTexturePath)
        : m_fCurrentRotationY(0.0f)
        , m_fRotationSpeed(0.01f) // Example: 0.01 radians per second (approx 0.57 degrees/sec)
    {
        // Initialize Skybox Texture from file
        HRESULT hr = InitSkyboxTexture(pd3dDevice, skyboxTexturePath);
        if (FAILED(hr))
        {
            // If loading failed, generate a procedural texture instead
            hr = GenerateSkyboxTexture(pd3dDevice);
        }
        // Initialize Skybox Geometry
        hr = InitSkyboxGeometry(pd3dDevice);
	}   

    CSkybox::~CSkybox() {
        if (g_pSkyboxTexture) g_pSkyboxTexture->Release(); // DX9 Cleanup
        if (g_pSkyboxVB) g_pSkyboxVB->Release();
    }
    void DrawSkybox(IDirect3DDevice9* pd3dDevice, D3DXMATRIX matView, double deltaTime);

    HRESULT SaveSkyboxToDDS(IDirect3DCubeTexture9* pSkyTexture, TCHAR* filename)
    {
        if (!pSkyTexture) return E_FAIL;

        // D3DXIFF_DDS is the file format (DirectDraw Surface)
        // The function accepts IDirect3DBaseTexture9*, which CubeTexture inherits from.
        HRESULT hr = D3DXSaveTextureToFile(
            filename,               // File path (e.g., "skybox_generated.dds")
            D3DXIFF_DDS,            // Format
            pSkyTexture,            // The texture pointer
            NULL                    // Palette (NULL for modern colors)
        );

        return hr;
    }

    D3DXVECTOR3 GetCameraPos(const D3DXMATRIX& matView)
    {
        D3DXMATRIX matInvView;
        D3DXVECTOR3 cameraPos;

        // The Inverse of the View Matrix is the "Camera's World Matrix"
        // The bottom row of a World Matrix is always the position.
        D3DXMatrixInverse(&matInvView, NULL, &matView);

        cameraPos.x = matInvView._41;
        cameraPos.y = matInvView._42;
        cameraPos.z = matInvView._43;

        return cameraPos;
    }

	IDirect3DCubeTexture9* GetTexture() { return g_pSkyboxTexture; }

	float GetRotationY() const { return m_fCurrentRotationY; }

};

