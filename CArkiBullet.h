#pragma once
#include "stdafx.h"
#include "CArkiBlock.h" // Assumed to hold PhysicsData and shared mesh

class CArkiBullet
{
public:
    btRigidBody* m_pBody;
    bool m_markForDelete; // Flag to remove bullet if it hits something or goes off screen
    float m_lifeTime; // Lifetime in seconds

    CArkiBullet(btDiscreteDynamicsWorld* dynamicsWorld, btVector3 startPos)
    {
        m_markForDelete = false;
        m_lifeTime = 3.0f; // Bullet lives for 3 seconds

        // Bullet Shape: Small cube (0.2 units)
        btCollisionShape* shape = new btBoxShape(btVector3(0.2f, 0.2f, 0.2f));

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(startPos);
        btDefaultMotionState* motionState = new btDefaultMotionState(transform);

        // Dynamic body (Mass = 1.0f) so it moves physically
        btRigidBody::btRigidBodyConstructionInfo rbInfo(1.0f, motionState, shape, btVector3(0, 0, 0));
        m_pBody = new btRigidBody(rbInfo);

        // Prevent rotation
        m_pBody->setAngularFactor(btVector3(0, 0, 0));
        // Set Initial Velocity (Moving Upwards)
        m_pBody->setLinearVelocity(btVector3(0, 40.0f, 0));

        // CCD (Continuous Collision Detection) is important for fast moving objects
        // so they don't tunnel through blocks
        m_pBody->setCcdMotionThreshold(0.2f);
        m_pBody->setCcdSweptSphereRadius(0.1f);

        dynamicsWorld->addRigidBody(m_pBody);

        // Assume TYPE_BULLET is added to your enums
        PhysicsData* pData = new PhysicsData{ TYPE_BULLET, this };
        m_pBody->setUserPointer(pData);
    }

    ~CArkiBullet()
    {
        if (m_pBody)
        {
            if (m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
            delete m_pBody->getMotionState();
            delete m_pBody->getCollisionShape();
            delete m_pBody;
        }


    }
    void Update(float dt)
    {
        m_lifeTime -= dt;
        if (m_lifeTime <= 0)
        {
            m_markForDelete = true;
        }
    }
    void Render(IDirect3DDevice9* device)
    {
        if (m_markForDelete ) return;

        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);

        D3DXMATRIXA16 matWorld, matScale, matTrans;

        // Scale small for bullet
        D3DXMatrixScaling(&matScale, 0.4f, 0.4f, 0.4f);
        D3DXMatrixTranslation(&matTrans, trans.getOrigin().getX(), trans.getOrigin().getY(), 0);

        matWorld = matScale * matTrans;
        device->SetTransform(D3DTS_WORLD, &matWorld);

        // Reuse the block mesh
        CArkiBlock::s_pSharedBoxMesh->DrawSubset(0);
    }
};