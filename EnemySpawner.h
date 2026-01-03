#pragma once
#include <btBulletDynamicsCommon.h>
#include "CFlyingEnemy.h" // Your existing enemy class

enum EFormationType 
{
    FORM_GRID,      // Standard block
    FORM_V_SHAPE,   // Flying V
    FORM_CIRCLE,    // Ring
    FORM_DUAL_LINE  // Two vertical lines
};

// Simple struct to define a specific wave
struct WaveDef 
{
    int enemyCount;
    EFormationType formation;
    EEnemyState movementStrategy; // Attack, ZigZag, etc. (From previous step)
    float difficultySpeed;
};

class EnemySpawner
{
private:
    btDiscreteDynamicsWorld* m_pWorld;
    CXMesh* m_pEnemyMesh; // The visual for the enemy
    CXMesh* m_pBulletMesh; // The visual for bullets

    // Lists
    std::vector<CFlyingEnemy*> m_enemies;
    //std::vector<CEnemyProjectile*> m_bullets; // Bullets live here now

    // Wave Logic
    int m_currentWaveIndex;
    float m_timeBetweenWaves;
    float m_waveTimer;
    bool m_isWaveActive;

    // The "Script" of levels
    std::vector<WaveDef> m_waves;

public:
    EnemySpawner(btDiscreteDynamicsWorld* world, CXMesh* enemyMesh, CXMesh* bulletMesh);
    ~EnemySpawner();

    void Update(float dt, D3DXVECTOR3 playerPos);
    void Render(IDirect3DDevice9* device);

    // The Core Algorithms
    void StartNextWave();
    std::vector<D3DXVECTOR3> CalculateFormation(EFormationType type, int count, D3DXVECTOR3 centerPos);

    // Getters for collision detection in main game
    std::vector<CFlyingEnemy*>& GetEnemies() { return m_enemies; }
    //std::vector<CEnemyProjectile*>& GetBullets() { return m_bullets; }
};