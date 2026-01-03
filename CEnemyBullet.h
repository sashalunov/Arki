#pragma once
#include <btBulletDynamicsCommon.h>
#include "XMesh.h" 

// Define behaviors for bullets
enum EProjectileType 
{
    PROJ_LINEAR,        // Standard straight shot
    PROJ_ACCELERATING,  // Starts slow, gets very fast (Sniper feel)
    PROJ_HOMING,        // Steers towards the player (Missile)
    PROJ_WOBBLE,         // Moves in a Sine wave (Hard to dodge)
    PROJ_WOBBLE_HOMING
};

class CEnemyBullet
{
private:
    // Physics & Visuals
    btRigidBody* m_pBody;
    CXMesh* m_pMesh;
    btDiscreteDynamicsWorld* m_pWorld;

    // Logic
    bool m_toBeDeleted;     // Manager checks this to clean up
    EProjectileType m_type;

    // Movement State
    btVector3 m_velocity;
    btVector3 m_targetPos;  // Last known player position (for homing)
    float m_speed;
    float m_lifeTime;       // How long until it vanishes
    float m_stateTimer;     // General timer for math (sine waves)

    // NEW: We need to remember how much we offset the position last frame
    // so we can calculate the true "forward" line next frame.
    btVector3 m_lastOffset;
public:

    // Constructor
    // startPos: Where bullet spawns
    // targetPos: Where aiming at (Player Position)
    // type: The behavior
    CEnemyBullet(btDiscreteDynamicsWorld* world, CXMesh* mesh,
        D3DXVECTOR3 startPos, D3DXVECTOR3 targetPos, EProjectileType type);

    ~CEnemyBullet();

    // The logic loop
    void Update(float dt, D3DXVECTOR3 currentPlayerPos);

    // Sync physics to graphics
    void Render(IDirect3DDevice9* device);
};