#pragma once
#include "CArkiBlock.h"
#include <btBulletDynamicsCommon.h>
#include "stdafx.h"

class CBullet
{
public:
    // Shared Data
    btRigidBody* m_pBody;
    btDiscreteDynamicsWorld* m_pWorld;

    bool m_markForDelete;
    float m_lifeTime;
    float m_speed;
    float m_damage;
    EEntityType m_type; // Who shot this?

    // 1. Base Constructor: Sets up Physics
    CBullet(btDiscreteDynamicsWorld* world, D3DXVECTOR3 startPos, EEntityType type)
        : m_pWorld(world), 
        m_type(type)
    {
        m_markForDelete = false;
        m_lifeTime = 5.0f;
        m_damage = 1.0f;
		m_speed = 10.0f;
        // 1. Convert D3D Vectors to Bullet Vectors
        btVector3 start(startPos.x, startPos.y, startPos.z);

        // 4. Setup Physics Body (Kinematic Sphere)
        btCollisionShape* shape = new btSphereShape(0.2f); // Tiny hitbox
        btTransform t;
        t.setIdentity();
        t.setOrigin(start);

        btDefaultMotionState* ms = new btDefaultMotionState(t);
        //m_pBody = new btRigidBody(0, ms, shape);
        // Dynamic body (Mass = 1.0f) so it moves physically
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0, ms, shape);
        m_pBody = new btRigidBody(rbInfo);
        // Prevent rotation
        //m_pBody->setAngularFactor(btVector3(0, 0, 0));
        // Set Initial Velocity (Moving Upwards)
        //m_pBody->setLinearVelocity(btVector3(0, 40.0f, 0));
        // CCD (Continuous Collision Detection) is important for fast moving objects
        // so they don't tunnel through blocks
        m_pBody->setCcdMotionThreshold(0.2f);
        m_pBody->setCcdSweptSphereRadius(0.2f);

        int myGroup = COL_BULLET;
        int myMask = COL_BLOCK | COL_WALL | COL_ENEMY | COL_PADDLE;
        // Kinematic Object (Code moves it, not gravity)
        m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_pBody->setActivationState(DISABLE_DEACTIVATION);
        m_pWorld->addRigidBody(m_pBody, myGroup, myMask);

        PhysicsData* pData = new PhysicsData{ type, this };
        m_pBody->setUserPointer(pData);
    }

    // 2. Virtual Destructor (Critical for inheritance!)
    virtual ~CBullet()
    {
        if (m_pBody) {
            if (m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
            m_pWorld->removeRigidBody(m_pBody);
            delete m_pBody->getMotionState();
            delete m_pBody->getCollisionShape();
            delete m_pBody;
        }
    }

    // 3. PURE VIRTUAL Update: Children MUST implement their own movement and render
    virtual void Update(double deltaTime) = 0;
    virtual void Render(IDirect3DDevice9* device) = 0;
    //{
       // btTransform trans = m_pBody->getWorldTransform();
        //btVector3 pos = trans.getOrigin();

        //m_pMesh->SetPosition(D3DXVECTOR3(pos.x(), pos.y(), pos.z()));
        // Optional: Auto-rotate to face velocity direction could go here
        //m_pMesh->Render();
    //}

    // 5. Shared Hit Logic
    virtual void OnHit() 
    {
        m_markForDelete = true; // Simple destroy
        // You could add particle spawning here later
    }
};