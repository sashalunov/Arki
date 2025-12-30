#pragma once
#include "CRigidBody.h"
#include "stdafx.h"

enum PowerupType {
    PU_HEALTH,
    PU_FLOOR,
    PU_GUN,
    PU_SPEED,
    PU_BALL,
    PU_MISSILE,
    PU_LASER,
    PU_BOMB,
    PU_SENTRY,
    PU_WIDE_PADDLE
    // Add more here...
};

class CArkiPowerup
{
public:
    btRigidBody* m_pBody;
    PowerupType m_type;
    bool m_markForDelete;
    bool m_collected;
private:
    btDiscreteDynamicsWorld* m_pWorld;

public:
    CArkiPowerup(btDiscreteDynamicsWorld* dynamicsWorld, btVector3 spawnPos, PowerupType type)
    {
        m_type = type;
        m_markForDelete = false;
        m_collected = false;
		m_pWorld = dynamicsWorld;
        // 1. Setup Physics (Small Box or Sphere)
        btCollisionShape* shape = new btSphereShape(0.5f);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(spawnPos);

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);

        // Mass = 1.0f (So gravity pulls it down)
        btRigidBody::btRigidBodyConstructionInfo rbInfo(1.0f, motionState, shape, btVector3(0, 0, 0));
        m_pBody = new btRigidBody(rbInfo);

        // 2. Lock Constraints (Important for 2D feel)
        // Prevent moving in Z (depth) and prevent ALL rotation
        m_pBody->setLinearFactor(btVector3(1, 1, 0));
        m_pBody->setAngularFactor(btVector3(0, 0, 0));
        m_pBody->setActivationState(DISABLE_DEACTIVATION);

        int myGroup = COL_POWERUP;
        int myMask = COL_PADDLE | COL_WALL | COL_BLOCK | COL_POWERUP |  COL_BALL;
        // 3. Add to world
        dynamicsWorld->addRigidBody(m_pBody, myGroup, myMask);

        // 4. Set User Pointer (So collision loop knows what this is)
        // Ensure you add TYPE_POWERUP to your ObjectType enum!
        PhysicsData* pData = new PhysicsData{ TYPE_POWERUP, this };
        m_pBody->setUserPointer(pData);
    }

    ~CArkiPowerup()
    {
        if (m_pBody) {
            m_pWorld->removeRigidBody(m_pBody);
            if (m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
            delete m_pBody->getMotionState();
            delete m_pBody->getCollisionShape();
            delete m_pBody;
        }
    }

    void Render(IDirect3DDevice9* device, CSpriteFont* font, IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle)
    {
        // Transform
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        D3DXMATRIXA16 matWorld, matTrans;

        // Simple Color Coding based on Type
        D3DMATERIAL9 mtrl;
        ZeroMemory(&mtrl, sizeof(mtrl));
        InitMaterialS(mtrl);
        // Offset Z slightly towards the camera (-0.1f) to prevent "Z-Fighting" (flickering)
        D3DXVECTOR3 textPos = D3DXVECTOR3((float)trans.getOrigin().getX(), (float)trans.getOrigin().getY(), (float)trans.getOrigin().getZ());
        textPos.z += 0.01f; // Assuming camera is at -Z, move text closer to camera
        textPos.x -= 0.0f; // Center text roughly
        textPos.y += 0.3f; // Center text roughly

        switch (m_type) {
        case PU_HEALTH:     
            mtrl.Diffuse = mtrl.Emissive = D3DXCOLOR(0, 0.7f, 0.3f, 1.0f);
            font->DrawString3D(textPos, 0.016f, L"H", D3DCOLOR_XRGB(255, 255, 0));
            break; // Green
		case PU_BOMB:      mtrl.Diffuse =  D3DXCOLOR(0.83f, 0.03f, 0.0f, 1);
            font->DrawString3D(textPos, 0.016f, L"B", D3DCOLOR_XRGB(255, 255, 0));
            break; // Blue
        case PU_GUN:        mtrl.Diffuse = mtrl.Emissive = D3DXCOLOR(0.25f, 0.15f, 0, 1);
            font->DrawString3D(textPos, 0.016f, L"G", D3DCOLOR_XRGB(255, 255, 0));
            break; // RED
        case PU_BALL:       mtrl.Diffuse = mtrl.Emissive = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1);
            font->DrawString3D(textPos, 0.016f, L"B", D3DCOLOR_XRGB(255, 255, 0));
            break; // Yellow
		case PU_MISSILE:    mtrl.Diffuse = mtrl.Emissive = D3DXCOLOR(1, 0.5f, 0, 1);
            font->DrawString3D(textPos, 0.016f, L"M", D3DCOLOR_XRGB(255, 255, 0));
            break; // Orange 
        default:            mtrl.Diffuse = D3DXCOLOR(1, 1, 1, 1); break; 
        }
        device->SetMaterial(&mtrl);
		device->SetTexture(0, NULL); // No texture
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE); // Use vertex alpha
		device->SetRenderState(D3DRS_LIGHTING, TRUE);


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
            device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT); // Add makes it look shiny/glowing
            device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE); // Add makes it look shiny/glowing

            // device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
        }

        D3DXMatrixTranslation(&matTrans, (float)trans.getOrigin().getX(), (float)trans.getOrigin().getY(), 0);
        device->SetTransform(D3DTS_WORLD, &matTrans);

        // Draw (Using shared block mesh for now, or load a capsule mesh)
        if (CRigidBody::s_pRigidBodySphereMesh) CRigidBody::s_pRigidBodySphereMesh->DrawSubset(0);
		D3DXMatrixIdentity(&matWorld);
		device->SetTransform(D3DTS_WORLD, &matWorld);

        // --- 5. CLEANUP (CRITICAL!) ---
        D3DXMATRIX matIdentity;
        D3DXMatrixIdentity(&matIdentity);
        device->SetTransform(D3DTS_TEXTURE0, &matIdentity); // Reset Matrix
        // If you don't turn this off, your Blocks and Text will look crazy/broken
        device->SetTexture(0, NULL);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0); // Reset to standard UVs
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE); // Reset to standard mixing


    }
};