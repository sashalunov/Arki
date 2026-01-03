#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "CRigidBody.h"
#include "CFlyingEnemy.h"

// ========================================================
// ENEMY CONTEXT IMPLEMENTATION
// ========================================================
CFlyingEnemy::CFlyingEnemy(btDiscreteDynamicsWorld* world, D3DXVECTOR3 pos, CXMesh* mesh)//, std::vector<CEnemyProjectile*>& bullets)
{
	m_pWorld = world;
    m_pMesh = mesh;
    m_startPos = pos;
    m_currentPos = pos;
    m_isDead = false;

    // Physics Setup (Kinematic)
    btCollisionShape* shape = new btBoxShape(btVector3(1.0f, 0.5f, 0.5f));
    btTransform t; t.setIdentity(); t.setOrigin(btVector3(pos.x, pos.y, pos.z));

    btDefaultMotionState* ms = new btDefaultMotionState(t);
    m_pBody = new btRigidBody(0, ms, shape);
    m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_pBody->setActivationState(DISABLE_DEACTIVATION);
    m_pBody->setUserPointer(this);
    world->addRigidBody(m_pBody);

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
}

void CFlyingEnemy::Render(IDirect3DDevice9* dev)
{
    if (m_isDead) return;
    D3DXMATRIXA16 matWorld = D3DXMATRIXA16(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        m_currentPos.x, m_currentPos.y, m_currentPos.z, 1);

    dev->SetTransform(D3DTS_WORLD, &matWorld);
	CRigidBody::s_pRigidBodySphereMesh->DrawSubset(0);
    //m_pMesh->SetPosition(m_currentPos);
    //m_pMesh->Render();
}

void CFlyingEnemy::OnHit()
{
    m_isDead = true;
    SetPosition(0, -999); // Hide logic
}