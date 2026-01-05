#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "CRigidBody.h"
#include "CFlyingEnemy.h"

// ========================================================
// ENEMY CONTEXT IMPLEMENTATION
// ========================================================
CFlyingEnemy::CFlyingEnemy(btDiscreteDynamicsWorld* world, CBulletManager* manager, D3DXVECTOR3 pos, CXMesh* mesh)
{
	m_pWorld = world;
    m_pMesh = mesh;
    m_startPos = pos;
    m_currentPos = pos;
	m_targetPos = D3DXVECTOR3(0, 0, 0); // Default target
    m_isDead = false;
    m_pBulletMan = manager;

    m_shootTimer = 2.0f + (rand() % 100) / 100.0f;
	m_burstCounter = 0;
	m_burstTimer = 5.0f;
	m_shootPattern = SHOOT_SINGLE;

    // Physics Setup (Kinematic)
    //btCollisionShape* shape = new btBoxShape(btVector3(1.0f, 0.5f, 0.5f));
	btCollisionShape* shape = new btSphereShape(0.56f);
    btTransform t; t.setIdentity(); t.setOrigin(btVector3(pos.x, pos.y, pos.z));

    btDefaultMotionState* ms = new btDefaultMotionState(t);
    m_pBody = new btRigidBody(0, ms, shape);
    m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_pBody->setActivationState(DISABLE_DEACTIVATION);
    m_pBody->setUserPointer(this);
    world->addRigidBody(m_pBody, COL_ENEMY, COL_BALL | COL_POWERUP);

    // Initial State
    ChangeState(STATE_ATTACK);
}

CFlyingEnemy::~CFlyingEnemy() {
    if (m_pBody) {
        m_pWorld->removeRigidBody(m_pBody);
        delete m_pBody->getMotionState();
        delete m_pBody;
    }
}

void CFlyingEnemy::ChangeState(EEnemyState newState)
{
    m_currentState = newState;

    // Strategy Swap
    if (newState == STATE_ATTACK) {
        m_pActiveStrategy = &m_strategyAttack;
    }
    else if (newState == STATE_RETREAT) {
        m_pActiveStrategy = &m_strategyRetreat;
    }

    // Notify new strategy
    m_pActiveStrategy->OnEnter(this);
}

void CFlyingEnemy::Update(double dt)
{
    if (m_isDead) return;
    // Delegate movement to the active strategy
    // If Update returns true, the strategy is finished.
    if (m_pActiveStrategy->Update(this, dt))
    {
        // State Machine Logic checks transitions
        if (m_currentState == STATE_ATTACK) {
            ChangeState(STATE_RETREAT);
        }
        else if (m_currentState == STATE_RETREAT) {
            ChangeState(STATE_ATTACK);
        }
    }

    switch (m_shootPattern) 
    {
    case SHOOT_SINGLE: 
        UpdateShooting_Simple(dt, m_targetPos);
       break;
    case SHOOT_BURST:  
        //UpdateShooting_Burst(dt, playerPos); 
       break;
        // Spread is usually triggered by a timer in Simple, just calling a different Shoot function
    }
}
// In CFlyingEnemy::Update(float dt, D3DXVECTOR3 playerPos)
void CFlyingEnemy::UpdateShooting_Simple(double dt, D3DXVECTOR3 playerPos)
{
    m_shootTimer -= (FLOAT)dt;
    D3DXVECTOR3 spawnPos = m_currentPos;
    spawnPos.y -= 1.5f;

    if (m_shootTimer <= 0.0f)
    {
        // 1. Fire!
        // We use the Manager we created earlier
        m_pBulletMan->SpawnEnemyBullet(spawnPos, &playerPos, PROJ_LINEAR);

        // 2. Reset Timer with Randomness
        // Base time 2.0s + Random 0.0 to 1.0s
        m_shootTimer = 2.0f + (rand() % 100) / 100.0f;
    }
}

void CFlyingEnemy::SetPosition(float x, float y)
{
    m_currentPos.x = x;
    m_currentPos.y = y;

    // Update Physics
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(btVector3(x, y, m_startPos.z));
    if (m_pBody && m_pBody->getMotionState()) {
        m_pBody->getMotionState()->setWorldTransform(trans);
        m_pBody->setWorldTransform(trans);
    }
}

void CFlyingEnemy::Shoot()
{
    D3DXVECTOR3 spawnPos = m_currentPos;
    spawnPos.y -= 1.5f;
    // Create bullet (assuming CEnemyProjectile constructor exists)
    //m_bulletList.push_back(new CEnemyProjectile(m_pWorld, spawnPos, m_pMesh));
	if (m_pBulletMan)m_pBulletMan->SpawnEnemyBullet(spawnPos, nullptr, PROJ_LINEAR);
}

void CFlyingEnemy::Render(IDirect3DDevice9* dev)
{
    if (m_isDead) return;
    // --- NEAT FEATURE: AUTO-ROTATION ---
    // Rotate the mesh to face the direction it is traveling.
    // Calculate angle from Velocity using atan2
    // Note: atan2(y, x) gives angle in radians.
    D3DXVECTOR3 v = m_strategyAttack.m_velocity;
    float angleZ = (FLOAT)atan2(v.y, v.x);
    // Depending on your mesh, you might need to add/subtract 90 degrees (1.57 rads)
    // assuming your mesh points "Up" by default.
    angleZ -= 1.57f;

    m_pMesh->SetPos(m_currentPos);
    // Apply rotation (Roll/Pitch/Yaw)
    m_pMesh->SetRotation(D3DXVECTOR3(0, 0, angleZ));
    m_pMesh->Update();

    m_pMesh->Render();
}

void CFlyingEnemy::OnHit()
{
    m_isDead = true;
    SetPosition(0, -999); // Hide logic
}