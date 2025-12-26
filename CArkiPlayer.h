#pragma once
#include "CArkiBlock.h"
#include "CArkiBullet.h"

class CArkiPlayer
{
public:
    btRigidBody* m_pBody;
    float m_speed;
    btDiscreteDynamicsWorld* m_pDynamicsWorld; // Store reference to world to spawn bullets
    // Gun Management
    std::vector<CArkiBullet*> m_bullets;
    float m_shootCooldown;

    CArkiPlayer(btDiscreteDynamicsWorld* dynamicsWorld, D3DXVECTOR3 startPos)
    {
        m_pDynamicsWorld = dynamicsWorld;
        m_speed = 20.0f;
        m_shootCooldown = 0.0f;
        // Paddle Shape (e.g., 2.0 wide, 0.5 high, 0.5 deep)
        btCollisionShape* shape = new btBoxShape(btVector3(2.0f, 0.25f, 0.5f));

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));
        btDefaultMotionState* motionState = new btDefaultMotionState(transform);

        // Mass 0 because we don't want physics to move it; WE move it.
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape, btVector3(0, 0, 0));
        m_pBody = new btRigidBody(rbInfo);
        m_pBody->setRestitution(1.0f);
        m_pBody->setFriction(0.0f);
        m_pBody->setRollingFriction(0.0f);
        m_pBody->setDamping(0.0f, 0.0f);
        m_pBody->setLinearFactor(btVector3(1, 1, 0));

        // MARK AS KINEMATIC!
        m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_pBody->setActivationState(DISABLE_DEACTIVATION); // Always active

        dynamicsWorld->addRigidBody(m_pBody);

        PhysicsData* pData = new PhysicsData{ TYPE_PLAYER, this };
        m_pBody->setUserPointer(pData);
    }
    ~CArkiPlayer()
    {
        // Cleanup Bullets
        for (auto* b : m_bullets) {
            m_pDynamicsWorld->removeRigidBody(b->m_pBody);
            delete b;
        }
        m_bullets.clear();
        // ... existing cleanup ...
        if (m_pBody && m_pBody->getUserPointer())
        {
            delete (PhysicsData*)m_pBody->getUserPointer();
        }
    }

    void Shoot()
    {
        if (m_shootCooldown > 0) return;

        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        btVector3 spawnPos = trans.getOrigin();

        // Spawn slightly above the paddle so it doesn't get stuck inside it
        spawnPos.setY(spawnPos.getY() + 1.0f);

        CArkiBullet* newBullet = new CArkiBullet(m_pDynamicsWorld, spawnPos);
        m_bullets.push_back(newBullet);

        m_shootCooldown = 0.25f; // Delay between shots
    }

    void Update(float deltaTime, bool moveLeft, bool moveRight)
    {
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        btVector3 origin = trans.getOrigin();

        if (moveLeft)  origin -= btVector3(m_speed * deltaTime,0,0);
        if (moveRight) origin += btVector3(m_speed * deltaTime, 0, 0);

        // Apply boundaries if needed
        // if (origin.getX() < -10) origin.setX(-10);

        trans.setOrigin(origin);
        m_pBody->getMotionState()->setWorldTransform(trans);

        // 2. Handle Shooting
        if (m_shootCooldown > 0) m_shootCooldown -= deltaTime;
        //if (fireInput) Shoot();

       // 3. Update Bullet Timers (BUT DO NOT DELETE HERE)
        for (auto* b : m_bullets)
        {
            b->Update(deltaTime);

            // Boundary Check
            if (b->m_pBody->getWorldTransform().getOrigin().getY() > 50.0f) {
                b->m_markForDelete = true;
            }
        }
    }

    // Call this AFTER CheckCollisions()
    void CleanupBullets()
    {
        for (int i = 0; i < m_bullets.size(); i++)
        {
            CArkiBullet* b = m_bullets[i];
            if (b->m_markForDelete)
            {
                m_pDynamicsWorld->removeRigidBody(b->m_pBody);
                delete b;
                m_bullets.erase(m_bullets.begin() + i);
                i--;
            }
        }
    }

    void Render(IDirect3DDevice9* device)
    {
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);

        // ... Get Transform from Bullet and Draw Cube ...
        D3DXMATRIXA16 matWorld, matRot, matScale, matTrans;

        D3DXMatrixScaling(&matScale, 4, 0.5f, 1);
        D3DXMatrixTranslation(&matTrans, trans.getOrigin().getX(), trans.getOrigin().getY(), 0);
        matWorld = matScale * matTrans;
        device->SetTransform(D3DTS_WORLD, &matWorld);
        CArkiBlock::s_pSharedBoxMesh->DrawSubset(0);
        // Render Bullets
        for (auto* b : m_bullets)
        {
            b->Render(device);
        }
    }
};