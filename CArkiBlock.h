// --------------------------------------------------------------------------------
//	file: CArkiBlock.h
//---------------------------------------------------------------------------------
//  created: 22.12.2025 8:39 by JIyHb
// --------------------------------------------------------------------------------
#pragma once
#include "CSpriteFont.h"
#include <btBulletDynamicsCommon.h>

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
    TYPE_POWERUP
};
struct PhysicsData
{
    EntityType type;
    void* pObject; // Pointer to the actual class (Ball*, Block*, etc.)
};

enum BlockType {
    BLOCK_STONE,
    BLOCK_METAL,
	BLOCK_POWERUP
};

// The Brick for my arkanoid-style game
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

    // --- SHARED RESOURCES (STATIC) ---
    // All blocks share this one mesh to save memory
    static ID3DXMesh* s_pSharedBoxMesh;
public:
    // 1. Static Setup (Call this ONCE in Game::Init)
    static HRESULT InitSharedMesh(IDirect3DDevice9* device)
    {
        if (s_pSharedBoxMesh) return S_OK; // Already created

        // Create a 1x1x1 Unit Cube
        // We will scale this up to the correct size in Render()
        return D3DXCreateBox(device, 1.0f, 1.0f, 1.0f, &s_pSharedBoxMesh, NULL);
    }

    // 2. Static Cleanup (Call this ONCE in Game::Shutdown)
    static void CleanupSharedMesh()
    {
        if (s_pSharedBoxMesh) {
            s_pSharedBoxMesh->Release();
            s_pSharedBoxMesh = NULL;
        }
    }

    CArkiBlock(btDiscreteDynamicsWorld* dynamicsWorld, D3DXVECTOR3 position, D3DXVECTOR3 halfSize, D3DCOLOR color, int score)
    {
        m_halfSize = halfSize;
        m_pos = position;
        m_isDestroyed = false;
        m_color = color;
        // 1. Create Collision Shape (Box)
        btCollisionShape* shape = new btBoxShape(btVector3(halfSize.x, halfSize.y, halfSize.z));
        m_pendingDestruction = false;
        // 2. Create Motion State (Position)
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(position.x, position.y, position.z));
        btDefaultMotionState* motionState = new btDefaultMotionState(transform);

        // 3. Create Rigid Body (Mass = 0 for static objects)
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape, btVector3(0, 0, 0));
        m_pBody = new btRigidBody(rbInfo);
        m_pBody->setRestitution(1.0f);
        m_pBody->setFriction(0.0f);
        m_pBody->setRollingFriction(0.0f);
        m_pBody->setDamping(0.0f, 0.0f);
        //m_pBody->setLinearFactor(btVector3(1, 1, 0));
        //m_pBody->setCcdMotionThreshold(radius);
       // m_pBody->setCcdSweptSphereRadius(radius * 0.5f);
        //m_pBody->setGravity(btVector3(0, 0, 0));

        // User pointer is useful for collision callbacks later (to know WHICH block was hit)
        // Heap allocate this so it persists (remember to delete in destructor!)
        PhysicsData* pData = new PhysicsData{ TYPE_BLOCK, this };
        m_pBody->setUserPointer(pData);

        // Collides with Ball and Bullet
        // 4. Add to world
        dynamicsWorld->addRigidBody(m_pBody, COL_BLOCK, COL_BALL | COL_BULLET | COL_POWERUP);
        scoreValue = score;
    }

    ~CArkiBlock()
    {
        // Cleanup physics memory in destructor
        if (m_pBody && m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
        if (m_pBody && m_pBody->getMotionState()) delete m_pBody->getMotionState();
        if (m_pBody && m_pBody->getCollisionShape()) delete m_pBody->getCollisionShape();
        if (m_pBody) delete m_pBody;
    }

    void Render(IDirect3DDevice9* device, CSpriteFont* font)
    {
        if (m_isDestroyed || !s_pSharedBoxMesh) return;
        // ... Your DirectX Drawing Code here (e.g., DrawBox(m_pos)) ...
        // --- A. Setup Matrices ---
        D3DXMATRIX matWorld, matScale, matTrans;

        // 1. Scale: The mesh is 1x1x1, so we multiply by (HalfSize * 2) to get full size
        D3DXMatrixScaling(&matScale, m_halfSize.x * 2.0f, m_halfSize.y * 2.0f, m_halfSize.z * 2.0f);

        // 2. Translate: Move to position
        D3DXMatrixTranslation(&matTrans, m_pos.x, m_pos.y, m_pos.z);

        // 3. Combine: World = Scale * Translate
        matWorld = matScale * matTrans;
        device->SetTransform(D3DTS_WORLD, &matWorld);

        // 1. Set the Color Material
        D3DMATERIAL9 mtrl;
        ZeroMemory(&mtrl, sizeof(mtrl));
        mtrl.Diffuse = mtrl.Ambient = D3DXCOLOR(m_color); // Convert D3DCOLOR to D3DXCOLOR struct
        mtrl.Emissive = D3DXCOLOR(0.1f, 0.1f, 0.1f, 1.0f); // Slight glow

        device->SetTexture(0, NULL);
        device->SetRenderState(D3DRS_LIGHTING, TRUE);

        //device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        //device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
       // device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha

        device->SetMaterial(&mtrl);
        s_pSharedBoxMesh->DrawSubset(0);

        // Render the Score on the face of the block
        // Offset Z slightly towards the camera (-0.1f) to prevent "Z-Fighting" (flickering)
        D3DXVECTOR3 textPos = m_pos;
        textPos.z += 0.5f; // Assuming camera is at -Z, move text closer to camera
        textPos.x -= 0.0f; // Center text roughly
        textPos.y += 0.3f; // Center text roughly

        // Scale: 0.05f reduces the huge font pixels to reasonable 3D units
        font->DrawString3D(textPos, 0.012f, std::to_wstring(scoreValue), D3DCOLOR_XRGB(255, 255, 0));


    }
};
