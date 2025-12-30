#include "stdafx.h"
#include "CSkybox.h"


const D3DVERTEXELEMENT9 SKYBOXVERT::Decl[] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
VOID WINAPI ProceduralSpaceGen(D3DXVECTOR4* pOut, const D3DXVECTOR3* pTexCoord,const D3DXVECTOR3* pTexelSize, LPVOID pData)
{
    // 1. Normalize the direction vector
    D3DXVECTOR3 dir;
    D3DXVec3Normalize(&dir, pTexCoord);

    // --- NEBULA GENERATION ---
    // Generate a noise value
    float noise = NebulaNoise(dir);

    // Base Colors for space (Deep Black/Blue)
    D3DXVECTOR4 colorDeepSpace(0.05f, 0.05f, 0.1f, 1.0f);

    // Color for the Nebula (Purple/Pink)
    D3DXVECTOR4 colorNebula(0.4f, 0.1f, 0.6f, 1.0f);

    // Mix them based on noise
    // We pow() the noise to make the transitions sharper
    float blendFactor = pow(noise, 3.0f);
    D3DXVec4Lerp(pOut, &colorDeepSpace, &colorNebula, blendFactor);

    // --- STAR GENERATION ---
    // Stars are high-frequency noise. We multiply the direction by a huge number.
    // This creates a "static" pattern.
    float starVal = Hash3D(dir * 100.0f); // 100.0f controls star density (higher = smaller/more stars)

    // Threshold: Only draw a star if the random value is very high (> 0.98)
    if (starVal > 0.995f)
    {
        // Add brightness for the star
        float brightness = (starVal - 0.995f) * 200.0f; // Amplify remaining range
        *pOut += D3DXVECTOR4(brightness, brightness, brightness, 0.0f);
    }

    // Force Alpha to 1.0
    pOut->w = 1.0f;
}

//--------------------------------------------------------------------------------------
void CSkybox::DrawSkybox(IDirect3DDevice9* pd3dDevice, D3DXMATRIX matView, double deltaTime)
{
    // Accumulate rotation over time
    m_fCurrentRotationX += m_fRotationSpeed * (FLOAT)deltaTime;
    // Optional: Keep rotation within 0 to 2*PI to prevent large float values
  // This maintains precision for longer running applications.
    while (m_fCurrentRotationX > D3DX_PI * 2.0f) // 2*PI is a full circle
    {
        m_fCurrentRotationX -= D3DX_PI * 2.0f;
    }
    // 1. Remove translation from View Matrix
    // The translation in a standard DX9 view matrix is in the 4th row (_41, _42, _43)
   // matView._41 = 0.0f;
    //matView._42 = 0.0f;
   // matView._43 = 0.0f;

   // pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
    //pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // World matrix can just be Identity since we handled position in View
    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
	D3DXVECTOR3 camPos = GetCameraPos(matView);
    D3DXMatrixTranslation(&matWorld, camPos.x, camPos.y, camPos.z);
    // Create the rotation matrix around the Y-axis
    D3DXMATRIX matRotX;
    D3DXMatrixRotationX(&matRotX, m_fCurrentRotationX);
    // Combine rotation and translation for the world matrix
   // Order: Rotate the skybox, then translate it to appear centered on the camera.
    matWorld = matRotX * matWorld;
    pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // ... Rendering continues below ...
    pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    // 2. Set Render States
    pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);         // Skybox emits its own color
    pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);     // Don't write to depth buffer
    pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);//D3DTOP_SELECTARG1
    pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

    // Clamp textures to avoid visible seams at the edges of the cube
    pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // 3. Bind Texture and Stream
    pd3dDevice->SetTexture(0, g_pSkyboxTexture);
    pd3dDevice->SetStreamSource(0, g_pSkyboxVB, 0, sizeof(SKYBOXVERT));

    // 4. Set Vertex Declaration (crucial for your custom struct)
    IDirect3DVertexDeclaration9* pVertDecl = NULL;
    // (Ideally, create this pVertDecl once during Init, not every frame)
    pd3dDevice->CreateVertexDeclaration(SKYBOXVERT::Decl, &pVertDecl);
    pd3dDevice->SetVertexDeclaration(pVertDecl);
    //pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);

    // 5. Draw
    pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12); // 12 triangles (2 per face * 6 faces)

    // 6. Cleanup / Restore States
    pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    if (pVertDecl) pVertDecl->Release();
    // Restore World transform to identity
    pd3dDevice->SetTransform(D3DTS_WORLD, &D3DXMATRIXA16(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
}
