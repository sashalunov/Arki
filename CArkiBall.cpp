#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "TextureTools.h"
#include "CArkiBall.h"

void CArkiBall::InitTrail(IDirect3DDevice9* device, const TCHAR* textureFilename)
{
    // 1. Load Texture
    //D3DXCreateTextureFromFile(device, textureFilename, &m_pTrailTexture);
    // Create a 256x256 texture fading from Red to Blue
    HRESULT hr = CreateHorizontalGradientTexture(
        device,            // Your initialized D3D9 Device
        256,                   // Width
        256,                   // Height
        D3DCOLOR_ARGB(155, 0, 100,250),// Start Color (Red)
        D3DCOLOR_ARGB(0, 0, 255,0),// End Color (Blue)
        &m_pTrailTexture          // Output pointer
    );

    // 2. Create Dynamic Vertex Buffer
    // Size: MaxSegments * 2 vertices (left side + right side)
    int numVerts = m_maxPathSegments * 2;

    // USAGE_DYNAMIC and POOL_DEFAULT are required for frequent CPU updates
    device->CreateVertexBuffer(
        numVerts * sizeof(TRAILVERTEX),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        TRAILVERTEX::FVF,
        D3DPOOL_DEFAULT, // Must be DEFAULT for dynamic usage
        &m_pTrailVB,
        NULL
    );
}

void CArkiBall::RenderTrail(IDirect3DDevice9* device,const D3DXVECTOR3& cameraPos)
{
    // Need at least 2 points to make a strip segment
    if (m_pathHistory.size() < 2 || !m_pTrailVB) return;

    int numPoints = m_pathHistory.size();
    int numVertices = numPoints * 2;

    // 1. Lock Dynamic Buffer (D3DLOCK_DISCARD is fast!)
    TRAILVERTEX* pVertices = NULL;
    m_pTrailVB->Lock(0, numVertices * sizeof(TRAILVERTEX), (void**)&pVertices, D3DLOCK_DISCARD);

    // Define "Up" for Arkanoid (Y-axis)
    //D3DXVECTOR3 upVec(0.0f, 1.0f, 0.0f);

    // 2. Generate Geometry Loop
    for (int i = 0; i < numPoints; ++i)
    {
        D3DXVECTOR3 currentCenter = m_pathHistory[i];
        D3DXVECTOR3 direction;

        // Calculate direction to the *next* point to find the sideways vector
        if (i < numPoints - 1) {
            // Direction towards tail
            direction = m_pathHistory[i + 1] - currentCenter;
        }
        else {
            // Last point reuses previous direction
            direction = currentCenter - m_pathHistory[i - 1];
        }
        D3DXVec3Normalize(&direction, &direction);

        // 2. NEW: Calculate Vector to Camera
        D3DXVECTOR3 toCamera = cameraPos - currentCenter;
        D3DXVec3Normalize(&toCamera, &toCamera);
        // 3. NEW: Calculate "Right" Vector
        // Cross Product of (Forward) and (ToCamera) gives a vector 
        // that is perpendicular to the view direction -> Facing the camera!
        D3DXVECTOR3 rightVec;
        D3DXVec3Cross(&rightVec, &direction, &toCamera);
        D3DXVec3Normalize(&rightVec, &rightVec);
        // 4. Calculate Edges (Same as before)
        D3DXVECTOR3 leftPos = currentCenter - (rightVec * m_ribbonWidth * 0.5f);
        D3DXVECTOR3 rightPos = currentCenter + (rightVec * m_ribbonWidth * 0.5f);
        // Calculate fading Alpha (0.0 at head, 1.0 at tail)
        // We invert it so the texture coordinate (U) controls the fade
        float t = (float)i / (float)(numPoints - 1);
        int alpha = (int)((1.0f - t) * 255.0f);
        D3DCOLOR col = D3DCOLOR_ARGB(alpha, 255, 255, 255);

        // Fill 2 vertices in the buffer
        // Even indices = Left side, Odd indices = Right side
        int vIndex = i * 2;

        // Left Vertex (V=0)
        pVertices[vIndex].position = leftPos;
        pVertices[vIndex].color = col;
        pVertices[vIndex].tex = D3DXVECTOR2(t, 0.0f);

        // Right Vertex (V=1)
        pVertices[vIndex + 1].position = rightPos;
        pVertices[vIndex + 1].color = col;
        pVertices[vIndex + 1].tex = D3DXVECTOR2(t, 1.0f);
    }

    m_pTrailVB->Unlock();

    // 3. Render States
    device->SetRenderState(D3DRS_LIGHTING, FALSE);

    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    // Important: Turn off culling so the ribbon is visible from both sides if it twists
    //device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    // Important: Don't write to Z-Buffer (transparency issue fix)
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    // Setup Texture mixing (Modulate texture color with vertex alpha)
    device->SetTexture(0, m_pTrailTexture);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha

    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    // Ensure 2D coordinates are used
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

    // World Matrix needs to be Identity because our vertices are already in World Space
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    device->SetTransform(D3DTS_WORLD, &matIdentity);

    // 4. Draw
    device->SetStreamSource(0, m_pTrailVB, 0, sizeof(TRAILVERTEX));
    device->SetFVF(TRAILVERTEX::FVF);
    // A triangle strip has (NumVertices - 2) triangles
    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, numVertices - 2);

    // 5. Restore States
   // device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

}

void CArkiBall::OnDeviceLost()
{
   // if(m_pTrailTexture)
   //     {
   //     m_pTrailTexture->Release();
   //     m_pTrailTexture = NULL;
	//}
    // Release the dynamic buffer because it is in D3DPOOL_DEFAULT
    if (m_pTrailVB)
    {
        m_pTrailVB->Release();
        m_pTrailVB = NULL;
    }
}

void CArkiBall::OnDeviceReset(IDirect3DDevice9* device)
{

   // if(m_pTrailTexture)
    //    {
        // Re-create the texture from file
   //     D3DXCreateTextureFromFile(device, TEXT(".\\ray_blue.png"), &m_pTrailTexture);
	//}
    if (m_pTrailVB) return; // Already exists?

    // Re-create the buffer with the exact same settings as InitTrail
    int numVerts = m_maxPathSegments * 2;

    device->CreateVertexBuffer(
        numVerts * sizeof(TRAILVERTEX),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        TRAILVERTEX::FVF,
        D3DPOOL_DEFAULT, // Still needs to be Default
        &m_pTrailVB,
        NULL
    );
}