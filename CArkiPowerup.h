#pragma once
#include "CArkiBlock.h" // For shared mesh or headers
#include "stdafx.h"

enum PowerupType {
    PU_HEALTH,
    PU_ARMOR,
    PU_GUN,
    PU_BALL,
    PU_MISSILE
    // Add more here...
};

class CArkiPowerup
{
public:
    btRigidBody* m_pBody;
    PowerupType m_type;
    bool m_markForDelete;

    CArkiPowerup(btDiscreteDynamicsWorld* dynamicsWorld, btVector3 spawnPos, PowerupType type)
    {
        m_type = type;
        m_markForDelete = false;

        // 1. Setup Physics (Small Box or Sphere)
        btCollisionShape* shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));

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

        // 3. Add to world
        dynamicsWorld->addRigidBody(m_pBody);

        // 4. Set User Pointer (So collision loop knows what this is)
        // Ensure you add TYPE_POWERUP to your ObjectType enum!
        PhysicsData* pData = new PhysicsData{ TYPE_POWERUP, this };
        m_pBody->setUserPointer(pData);
    }

    ~CArkiPowerup()
    {
        if (m_pBody) {
            if (m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
            delete m_pBody->getMotionState();
            delete m_pBody->getCollisionShape();
            delete m_pBody;
        }
    }

    void Render(IDirect3DDevice9* device)
    {
        // Simple Color Coding based on Type
        D3DMATERIAL9 mtrl;
        ZeroMemory(&mtrl, sizeof(mtrl));

        switch (m_type) {
        case PU_HEALTH:     mtrl.Diffuse = D3DXCOLOR(1, 0, 0, 1); break; // Red
        case PU_GUN:        mtrl.Diffuse = D3DXCOLOR(0, 0, 1, 1); break; // Blue
        case PU_BALL:       mtrl.Diffuse = D3DXCOLOR(0, 1, 0, 1); break; // Green
        default:            mtrl.Diffuse = D3DXCOLOR(1, 1, 0, 1); break; // Yellow
        }
        device->SetMaterial(&mtrl);

        // Transform
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);

        D3DXMATRIXA16 matWorld, matTrans;
        D3DXMatrixTranslation(&matTrans, trans.getOrigin().getX(), trans.getOrigin().getY(), 0);
        device->SetTransform(D3DTS_WORLD, &matTrans);

        // Draw (Using shared block mesh for now, or load a capsule mesh)
        if (CArkiBlock::s_pSharedBoxMesh) CArkiBlock::s_pSharedBoxMesh->DrawSubset(0);
    }
};