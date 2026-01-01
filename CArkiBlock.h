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
	BLOCK_POWERUP
};



// Creates a 3D Rounded Box Mesh
// width, height, depth: Total dimensions of the box
// radius: Radius of the rounded corners
// slices: Smoothness of the curves (higher = smoother, e.g., 16 or 24)
inline ID3DXMesh* CreateRoundedBox(IDirect3DDevice9* device, float width, float height, float depth, float radius, int slices)
{
    // 1. Create a high-res Sphere to start. 
    // We use a sphere because it already has the correct topology (smooth corners).
    ID3DXMesh* pSphere = NULL;
    D3DXCreateSphere(device, 1.0f, slices, slices, &pSphere, NULL);

    // 2. Clone the mesh to get a vertex buffer we can modify (SYSTEMMEM allows locking)
    ID3DXMesh* pRoundedBox = NULL;
    pSphere->CloneMeshFVF(D3DXMESH_SYSTEMMEM, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, device, &pRoundedBox);
    pSphere->Release(); // Done with the template sphere

    // 3. Lock the vertex buffer
    struct Vertex { float x, y, z; float nx, ny, nz; float u, v; };
    Vertex* v = NULL;
    pRoundedBox->LockVertexBuffer(0, (void**)&v);

    int numVerts = pRoundedBox->GetNumVertices();

    // 4. Calculate the "Inner Box" limits.
    // The rounded box is essentially a smaller flat box + a radius extension.
    float innerW = (width * 0.5f) - radius;
    float innerH = (height * 0.5f) - radius;
    float innerD = (depth * 0.5f) - radius;

    // Safety check: if radius is too big, clamp it
    if (innerW < 0) innerW = 0;
    if (innerH < 0) innerH = 0;
    if (innerD < 0) innerD = 0;

    for (int i = 0; i < numVerts; i++)
    {
        // Get the unit vector from the center (which is just the sphere vertex pos)
        D3DXVECTOR3 normal(v[i].x, v[i].y, v[i].z);
        D3DXVec3Normalize(&normal, &normal);

        // --- The Morphing Logic ---
        // We push the vertex to the "corner" of the inner flat box...
        float sx = (normal.x > 0) ? innerW : -innerW;
        float sy = (normal.y > 0) ? innerH : -innerH;
        float sz = (normal.z > 0) ? innerD : -innerD;

        // ...and then add the radius in the direction of the normal.
        // This preserves the spherical curve at the corners but flattens the faces.
        v[i].x = sx + (normal.x * radius);
        v[i].y = sy + (normal.y * radius);
        v[i].z = sz + (normal.z * radius);

        // Update the normal (it remains the same as the sphere normal!)
        v[i].nx = normal.x;
        v[i].ny = normal.y;
        v[i].nz = normal.z;
    }

    pRoundedBox->UnlockVertexBuffer();

    // 5. Recalculate Bounding Box (Optional, D3DX updates this automatically usually)
    // D3DXComputeBoundingBox(...) 

    return pRoundedBox;
}

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

    void Render(IDirect3DDevice9* device, CSpriteFont* font, IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle)
    {
        if (m_isDestroyed || !s_pSharedBoxMesh) return;
        // ... Your DirectX Drawing Code here (e.g., DrawBox(m_pos)) ...
        // --- A. Setup Matrices ---
        D3DXMATRIX matWorld, matScale, matTrans;

        // 1. Scale: The mesh is 1x1x1, so we multiply by (HalfSize * 2) to get full size
        D3DXMatrixScaling(&matScale, 1.0f, 1.0f, 1.0f);

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
		mtrl.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
		mtrl.Power = 60.0f;

        device->SetTexture(0, NULL);
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
        device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
        // --- 3. ENABLE REFLECTION MAPPING ---
        if (pReflectionTexture)
        {
            // Bind the Skybox Texture
            device->SetTexture(0, pReflectionTexture);

            // MAGIC: Tell DX9 to automatically calculate the Reflection Vector based on Camera Angle
            device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
            // This rotates the 3D reflection vector around the Y axis
            D3DXMATRIX matTextureRot;
            D3DXMatrixRotationX(&matTextureRot, rotationAngle);
            // C. Apply the Matrix to Texture Stage 0
            device->SetTransform(D3DTS_TEXTURE0, &matTextureRot);
            // Tell DX9 this is a Cube Map (requires 3D coordinates)
            device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);

            // Optional: Mix the reflection with the material color (Modulate or Add)
            device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD); // Add makes it look shiny/glowing
            // device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
        }

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

