// --------------------------------------------------------------------------------
//	file: CArkiBlock.cpp
//---------------------------------------------------------------------------------
//  created: 03.01.2026 13:18 by JIyHb
// --------------------------------------------------------------------------------

#include "stdafx.h"
#include "CSpriteFont.h"
#include "CArkiBlock.h"

// IMPORTANT: Define the static variable in your .cpp file (outside any function)
ID3DXMesh* CArkiBlock::s_pSharedBoxMesh = NULL;

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CArkiBlock::CArkiBlock(btDiscreteDynamicsWorld* dynamicsWorld, D3DXVECTOR3 position, D3DXVECTOR3 halfSize, D3DCOLOR color, int score)
{
    m_halfSize = halfSize;
    m_pos = position;
    m_isDestroyed = false;
    m_color = color;
    // 1. Create Collision Shape (Box)
    btCollisionShape* shape = new btBoxShape(btVector3(halfSize.x, halfSize.y, halfSize.z));
    m_pendingDestruction = false;
    // 2. Create Motion State (Position)
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    // 3. Create Rigid Body (Mass = 0 for static objects)
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape, btVector3(0, 0, 0));
    m_pBody = new btRigidBody(rbInfo);
    m_pBody->setRestitution(1.0f);
    m_pBody->setFriction(0.0f);
    m_pBody->setRollingFriction(0.0f);
    m_pBody->setDamping(0.0f, 0.0f);
    //m_pBody->setLinearFactor(btVector3(1, 1, 0));
    //m_pBody->setCcdMotionThreshold(radius);
    //m_pBody->setCcdSweptSphereRadius(radius * 0.5f);
    //m_pBody->setGravity(btVector3(0, 0, 0));
    // User pointer is useful for collision callbacks later (to know WHICH block was hit)
    // Heap allocate this so it persists (remember to delete in destructor!)
    PhysicsData* pData = new PhysicsData{ TYPE_BLOCK, this };
    m_pBody->setUserPointer(pData);
    // Collides with Ball and Bullet
    // 4. Add to world
    dynamicsWorld->addRigidBody(m_pBody, COL_BLOCK, COL_BALL | COL_BULLET | COL_POWERUP);
    scoreValue = score;
}
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CArkiBlock::Render(IDirect3DDevice9* device, CSpriteFont* font, IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle)
{
    if (m_isDestroyed || !s_pSharedBoxMesh) return;
    // ... Your DirectX Drawing Code here (e.g., DrawBox(m_pos)) ...
    // --- A. Setup Matrices ---
    D3DXMATRIX matWorld, matScale, matTrans;
    // 1. Scale: The mesh is 1x1x1, so we multiply by (HalfSize * 2) to get full size
    D3DXMatrixScaling(&matScale, 1.0f, 1.0f, 1.0f);
    // 2. Translate: Move to position
    D3DXMatrixTranslation(&matTrans, m_pos.x, m_pos.y, m_pos.z);
    // 3. Combine: World = Scale * Translate
    matWorld = matScale * matTrans;
    device->SetTransform(D3DTS_WORLD, &matWorld);
    // 1. Set the Color Material
    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Diffuse = mtrl.Ambient = D3DXCOLOR(m_color); // Convert D3DCOLOR to D3DXCOLOR struct
    mtrl.Emissive = D3DXCOLOR(0.1f, 0.1f, 0.1f, 1.0f); // Slight glow
    mtrl.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.Power = 60.0f;
    device->SetTexture(0, NULL);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    // --- 3. ENABLE REFLECTION MAPPING ---
    if (pReflectionTexture)
    {
        // Bind the Skybox Texture
        device->SetTexture(0, pReflectionTexture);
        // MAGIC: Tell DX9 to automatically calculate the Reflection Vector based on Camera Angle
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
        // This rotates the 3D reflection vector around the Y axis
        D3DXMATRIX matTextureRot;
        D3DXMatrixRotationX(&matTextureRot, rotationAngle);
        // C. Apply the Matrix to Texture Stage 0
        device->SetTransform(D3DTS_TEXTURE0, &matTextureRot);
        // Tell DX9 this is a Cube Map (requires 3D coordinates)
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        // Optional: Mix the reflection with the material color (Modulate or Add)
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD); // Add makes it look shiny/glowing
        // device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
    }
    //device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    //device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    //device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha
    device->SetMaterial(&mtrl);
    s_pSharedBoxMesh->DrawSubset(0);
    // Render the Score on the face of the block
    // Offset Z slightly towards the camera (-0.1f) to prevent "Z-Fighting" (flickering)
    D3DXVECTOR3 textPos = m_pos;
    textPos.z += 0.5f; // Assuming camera is at -Z, move text closer to camera
    textPos.x -= 0.0f; // Center text roughly
    textPos.y += 0.3f; // Center text roughly
    // Scale: 0.05f reduces the huge font pixels to reasonable 3D units
    font->DrawString3D(textPos, 0.012f, std::to_wstring(scoreValue), D3DCOLOR_XRGB(255, 255, 0));
}
