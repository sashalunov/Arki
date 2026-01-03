#pragma once
#include "MovementStrategy.h"
#include "XMesh.h"
// --------------------------------------------------------
// THE CONTEXT CLASS (The Enemy)
// --------------------------------------------------------
enum EEnemyState {
    STATE_ATTACK,
    STATE_RETREAT
};

class CFlyingEnemy
{
public:
    // Physics & Visuals
    btRigidBody* m_pBody;
    CXMesh* m_pMesh;
    btDiscreteDynamicsWorld* m_pWorld;

    // Game Logic
    bool m_isDead;
    D3DXVECTOR3 m_startPos;     // Home position
    D3DXVECTOR3 m_currentPos;   // Current position

    // Combat
    float m_shootTimer;
    //std::vector<CEnemyProjectile*>& m_bulletList;

    // --- STRATEGY & STATE MANAGEMENT ---
    EEnemyState m_currentState;
    IMovementStrategy* m_pActiveStrategy;

    // Pre-allocated strategies to avoid 'new' every switch
    CSineMove2 m_strategyAttack;
    CReturnMove2 m_strategyRetreat;

public:
    CFlyingEnemy(btDiscreteDynamicsWorld* world, D3DXVECTOR3 pos, CXMesh* mesh);// , std::vector<CEnemyProjectile*>& bullets);
    ~CFlyingEnemy();

    void Update(double dt);
    void Render(IDirect3DDevice9* dev);

    // Helper to switch states
    void ChangeState(EEnemyState newState);

    // Helpers for Strategies to control the enemy
    void SetPosition(float x, float y);
    void Shoot();
    void OnHit();
};
