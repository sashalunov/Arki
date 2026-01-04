#pragma once
#include <btBulletDynamicsCommon.h>
#include "CBullet.h"
#include "CRigidBody.h" 

// Define behaviors for bullets
enum EProjectileType 
{
    PROJ_LINEAR,        // Standard straight shot
    PROJ_ACCELERATING,  // Starts slow, gets very fast (Sniper feel)
    PROJ_HOMING,        // Steers towards the player (Missile)
    PROJ_WOBBLE,         // Moves in a Sine wave (Hard to dodge)
    PROJ_WOBBLE_HOMING
};

class CEnemyBullet : public CBullet
{
private:
    EProjectileType m_projtype;

    // Movement State
    btVector3 m_velocity;
    btVector3 m_targetPos;  // Last known player position (for homing)
    btVector3 m_lastOffset;

    float m_stateTimer;     // General timer for math (sine waves)

public:
    CEnemyBullet(btDiscreteDynamicsWorld* world,D3DXVECTOR3 startPos, D3DXVECTOR3 targetPos, EProjectileType type);

    // The logic loop
    void Update(double deltaTime) override;
    // Sync physics to graphics
    void Render(IDirect3DDevice9* device) override;

    void Reset(D3DXVECTOR3 start, D3DXVECTOR3* target, EProjectileType type)
    {
        m_markForDelete = false;
        m_lifeTime = 5.0f;
        m_projtype = type;
        //m_pPlayerTarget = target;

        // Teleport Physics Body
        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(start.x, start.y, start.z));

        m_pBody->setWorldTransform(t);
        m_pBody->getMotionState()->setWorldTransform(t);

        // Recalculate Velocity
        // ... (Copy logic from Constructor) ...
        m_pBody->setLinearVelocity(btVector3(0, 0, 0)); // Clear old momentum
    }
};