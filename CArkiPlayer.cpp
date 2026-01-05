#include "stdafx.h"
#include <btBulletDynamicsCommon.h>

#include "CArkiPlayer.h"


CArkiPlayer::CArkiPlayer(btDiscreteDynamicsWorld* dynamicsWorld, IDirect3DDevice9* device, D3DXVECTOR3 startPos)
{
    m_pDevice = device;
    m_pDynamicsWorld = dynamicsWorld;
    m_speed = 20.0f;
    m_shootCooldown = 0.0f;

    m_maxHealth = 100.0f;
    m_health = m_maxHealth;
    m_isDead = false;
    m_invulnerabilityTimer = 0.0f;
    m_color = D3DXCOLOR(1, 1, 1, 1); // White by default

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

    dynamicsWorld->addRigidBody(m_pBody, COL_PADDLE, COL_BALL | COL_POWERUP);

    PhysicsData* pData = new PhysicsData{ TYPE_PLAYER, this };
    m_pBody->setUserPointer(pData);

    AddBall(); // Start with one ball
}

void CArkiPlayer::TakeDamage(float damage)
{
    // 1. Check Invulnerability and Death state
    if (m_isDead || m_invulnerabilityTimer > 0.0f) return;

    m_health -= damage;

    // 2. Visual Feedback (Flash Red)
    m_color = D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f);
    m_invulnerabilityTimer = 2.0f; // 2 seconds of safety

    // 3. Play Sound (Assuming you have a function like this based on your headers)
    // PlayAudioFromMemory(GenerateExplosionSound()); 

    // 4. Handle Death
    if (m_health <= 0)
    {
        m_health = 0;
        m_isDead = true;

        // Optional: Hide the paddle or disable physics
        // m_pBody->setActivationState(DISABLE_SIMULATION);
        // Or trigger a "Game Over" event here
    }
}

void CArkiPlayer::Heal(float amount)
{
    if (m_isDead) return; // Can't heal if dead
    m_health += amount;
    if (m_health > m_maxHealth)
        m_health = m_maxHealth;
    // Optional: Play healing sound or visual effect
    // PlayAudioFromMemory(GenerateHealingSound());
}