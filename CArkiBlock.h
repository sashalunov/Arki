// --------------------------------------------------------------------------------
//	file: CArkiBlock.h
//---------------------------------------------------------------------------------
//  created: 22.12.2025 8:39 by JIyHb
// --------------------------------------------------------------------------------
#pragma once
#include <btBulletDynamicsCommon.h>

class CSpriteFont;

// Collision Categories (Bitmasks)
enum CollisionGroup {
    COL_NOTHING = 0,
    COL_WALL = 1 << 0, // 1
    COL_BALL = 1 << 1, // 2
    COL_PADDLE = 1 << 2, // 4
    COL_BLOCK = 1 << 3, // 8
    COL_POWERUP = 1 << 4, // 16
    COL_BULLET = 1 << 5  // 32
};

enum EntityType { 
    TYPE_BALL, 
    TYPE_BLOCK, 
    TYPE_PLAYER, 
    TYPE_WALL, 
    TYPE_BULLET,
    TYPE_POWERUP,
    TYPE_BOMB,
	TYPE_UNKNOWN
};
struct PhysicsData
{
    EntityType type;
    void* pObject; // Pointer to the actual class (Ball*, Block*, etc.)
};

enum BlockType {
    BLOCK_STONE,
    BLOCK_METAL,
	BLOCK_UNDESTRUCTIBLE
};

// --------------------------------------------------------------------------------
// The Brick for my arkanoid-style game
// --------------------------------------------------------------------------------
class CArkiBlock
{
public:
    btRigidBody* m_pBody;
    D3DXVECTOR3 m_pos;
    D3DXVECTOR3 m_halfSize;
    bool m_isDestroyed;
    D3DCOLOR m_color;
    bool m_pendingDestruction;
    int scoreValue;
    // All blocks share this one mesh to save memory
    static ID3DXMesh* s_pSharedBoxMesh;
public:
    // 1. Static Setup (Call this ONCE in Game::Init)
    static HRESULT InitSharedMesh(IDirect3DDevice9* device)
    {
        if (s_pSharedBoxMesh) return S_OK; // Already created
        // Create a 1x1x1 Unit Cube
        // We will scale this up to the correct size in Render()
        //D3DXCreateBox(device, 1.0f, 1.0f, 1.0f, &s_pSharedBoxMesh, NULL);
        s_pSharedBoxMesh = CreateRoundedBox(device, 2.0f, 1.0f, 1.0f, 0.15f, 9);
        return S_OK;
    }

    // 2. Static Cleanup (Call this ONCE in Game::Shutdown)
    static void CleanupSharedMesh()
    {
        if (s_pSharedBoxMesh) {
            s_pSharedBoxMesh->Release();
            s_pSharedBoxMesh = NULL;
        }
    }

    CArkiBlock(btDiscreteDynamicsWorld* dynamicsWorld, D3DXVECTOR3 position, D3DXVECTOR3 halfSize, D3DCOLOR color, int score);
   
    ~CArkiBlock()
    {
        // Cleanup physics memory in destructor
        if (m_pBody && m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
        if (m_pBody && m_pBody->getMotionState()) delete m_pBody->getMotionState();
        if (m_pBody && m_pBody->getCollisionShape()) delete m_pBody->getCollisionShape();
        if (m_pBody) delete m_pBody;
    }

    void Render(IDirect3DDevice9* device, CSpriteFont* font, IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle);
    void Update(double deltaTime) {}
   
};

