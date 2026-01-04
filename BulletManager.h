#pragma once
#include "CBullet.h" 
//#include "CPlayerBullet.h"
#include "CEnemyBullet.h"

class CBulletManager
{
private:
    // THE POOLS (Inactive bullets waiting to be used)
    std::vector<CEnemyBullet*> m_enemyPool;
    //std::vector<CPlayerBullet*> m_playerPool;

    // THE ACTIVE LIST (Bullets currently flying)
    // We use a separate list for updating so we don't iterate over dead ones.
    std::vector<CBullet*> m_activeBullets;

    // References
    btDiscreteDynamicsWorld* m_pWorld;
    //CXMesh* m_pMeshEnemyBullet;
    //CXMesh* m_pMeshPlayerBullet;

public:
    CBulletManager(btDiscreteDynamicsWorld* world);
    ~CBulletManager();

    // The Main Loop Functions
    void Update(double deltaTime);
    void Render(IDirect3DDevice9* device);

    // Spawning Functions (Requested by Entities)
    void SpawnEnemyBullet(D3DXVECTOR3 start, D3DXVECTOR3* target, EProjectileType type);
    void SpawnPlayerBullet(D3DXVECTOR3 start);

    // Helper to get active list for collision detection
    const std::vector<CBullet*>& GetActiveBullets() { return m_activeBullets; }
};