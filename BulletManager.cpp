#include "stdafx.h"
#include "BulletManager.h"

CBulletManager::CBulletManager(btDiscreteDynamicsWorld* world)
    : m_pWorld(world)
{
    // OPTIONAL: Pre-warm the pool
    // Create 1000 bullets at startup so we don't lag during gameplay
    for (int i = 0; i < 1000; i++) 
    {
        CEnemyBullet* b = new CEnemyBullet(world, D3DXVECTOR3(0, -1000, 0), D3DXVECTOR3(0, 0, 0), PROJ_LINEAR);
        b->m_markForDelete = true; // Start inactive
        // Remove from physics immediately so they don't collide off-screen
        m_pWorld->removeRigidBody(b->m_pBody);
        m_enemyPool.push_back(b);
    }
}
CBulletManager::~CBulletManager()
{
    // 1. Delete Active Bullets
    // These are currently in the physics world. Calling delete triggers 
    // ~CBullet(), which automatically removes them from m_pWorld.
    for (CBullet* b : m_activeBullets)
    {
        delete b;
    }
    m_activeBullets.clear();

    // 2. Delete Inactive Enemy Pool
    // These are sitting in memory waiting to be reused.
    for (CEnemyBullet* b : m_enemyPool)
    {
        delete b;
    }
    m_enemyPool.clear();

    // 3. Delete Inactive Player Pool
    //for (CPlayerBullet* b : m_playerPool)
    //{
    //    delete b;
    //}
    //m_playerPool.clear();
}

void CBulletManager::SpawnEnemyBullet(D3DXVECTOR3 start, D3DXVECTOR3* target, EProjectileType type)
{
    CEnemyBullet* bullet = nullptr;

    // 1. TRY TO RECYCLE
    if (!m_enemyPool.empty()) 
    {
        bullet = m_enemyPool.back();
        m_enemyPool.pop_back();

        // Reactivate Physics
        int myGroup = COL_BULLET;
        int myMask = COL_BLOCK | COL_WALL | COL_ENEMY | COL_PADDLE;

        m_pWorld->addRigidBody(bullet->m_pBody);

        // Reset State (CRITICAL in pooling)
        bullet->Reset(start, target, type); 
    }
    else 
    {
        // Pool empty? Create new (Slow path)
        bullet = new CEnemyBullet(m_pWorld, start, *target, type);
    }

    m_activeBullets.push_back(bullet);
}

void CBulletManager::Update(double dt)
{
    for (int i = 0; i < m_activeBullets.size(); i++)
    {
        CBullet* b = m_activeBullets[i];
        b->Update(dt);

        if (b->m_markForDelete)
        {
            // DO NOT DELETE. RECYCLE.
            // 1. Remove from Physics
            m_pWorld->removeRigidBody(b->m_pBody);
            // 2. Return to specific pool
            if (b->m_type == TYPE_ENEMY_BULLET) 
            {
                // Safe cast because we checked owner
                m_enemyPool.push_back(static_cast<CEnemyBullet*>(b));
            }
            else 
            {
                //m_playerPool.push_back(static_cast<CPlayerBullet*>(b));
            }

            // 3. Remove from Active List (Fast Swap-and-Pop)
            // Instead of erase() which shifts all elements (slow), 
            // we swap with the last element and pop. Order doesn't matter for bullets.
            m_activeBullets[i] = m_activeBullets.back();
            m_activeBullets.pop_back();

            i--; // Decrement i so we don't skip the one we just swapped in
        }
    }
}

void CBulletManager::Render(IDirect3DDevice9* device)
{
    for (int i = 0; i < m_activeBullets.size(); i++)
    {
        CBullet* b = m_activeBullets[i];
		b->Render(device);
    }
}

