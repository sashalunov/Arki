#pragma once
#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "CArkiBlock.h"
#include "TextureTools.h"

// Static noise buffer
static float s_noiseBuffer[1024];
static bool s_noiseInitialized = false;


// Helper: Convert Bullet Physics Transform to DirectX 9 Matrix
inline D3DXMATRIXA16 ConvertBulletTransform(const btTransform& trans)
{
    D3DXMATRIXA16 matWorld;
    D3DXMatrixIdentity(&matWorld);

    // 1. Extract Rotation Basis and Origin (Position)
    btMatrix3x3 rot = trans.getBasis();
    btVector3 pos = trans.getOrigin();

    // 2. Copy Rotation (Row-Major mapping)
    matWorld._11 = (float)rot[0][0]; matWorld._12 = (float)rot[0][1]; matWorld._13 = (float)rot[0][2];
    matWorld._21 = (float)rot[1][0]; matWorld._22 = (float)rot[1][1]; matWorld._23 = (float)rot[1][2];
    matWorld._31 = (float)rot[2][0]; matWorld._32 = (float)rot[2][1]; matWorld._33 = (float)rot[2][2];

    // 3. Copy Translation (Bottom row in DX9)
    matWorld._41 = (float)pos.getX();
    matWorld._42 = (float)pos.getY();
    matWorld._43 = (float)pos.getZ();

    return matWorld;
}

void InitNoise();

// Get jagged offset for a specific vertical position
float GetNoiseAt(float y);

// Custom Vertex Format for DX9
struct CliffVertex {
    float x, y, z;
    float nx, ny, nz; // NORMAL
    D3DCOLOR color;
    float u, v;
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
class CArkiCliffTreadmill
{
public:
    // Store exact position state
    float m_currentY;      // Current Y position of the segment start
    float m_height;        // Height of this segment
    float m_topJaggedX;    // The X offset of the top-most vertex (for stitching)
    float m_topVirtualY;
private:
    btRigidBody* m_pBody;
    btBvhTriangleMeshShape* m_pMeshShape;
    //btTriangleMesh* m_pTriMesh; // The Bullet Physics Mesh Data
    btTriangleIndexVertexArray* m_pSharedMesh;

    std::vector<CliffVertex> m_vertices;
    std::vector<short> m_indices;

    float m_scrollAccumulator; // Tracks "Virtual" distance scrolled
    float m_startY, m_endY;
    float m_baseX;
    bool m_facingRight;
    btDiscreteDynamicsWorld* m_pWorld;
    float m_uvScale;

    IDirect3DTexture9* m_cliffTexture;
public:
    CArkiCliffTreadmill(btDiscreteDynamicsWorld* world, IDirect3DDevice9* device, float height, float startY, float xPos, bool facingRight, float startVirtualY)
    {
        InitNoise();
        // Generate a 256x256 texture procedurally
        m_cliffTexture = CTextureGenerator::CreateRockTexture(device, 512, 512);
		//SaveTexture(m_cliffTexture, L"cliff_texture.png");

        m_pWorld = world;
        m_baseX = xPos;
        m_facingRight = facingRight;
        m_height = height;
        m_uvScale = 0.2f;
        m_currentY = startY;

        m_pBody = nullptr;
        m_pMeshShape = nullptr;
        m_pSharedMesh = nullptr;

        // 1. Setup Topology (One-time setup)
        int segments = 64;
        float segHeight = m_height / segments;

        for (int i = 0; i <= segments; i++)
        {
            // Local Y (0 to Height)
            float localY = i * segHeight;
            D3DCOLOR c = D3DCOLOR_XRGB(255, 255, 255);

            // Push 8 placeholders per row
            for (int k = 0; k < 8; k++)
                m_vertices.push_back({ 0, localY, 0, 0,0,0, c, 0,0 });

            if (i > 0) {
                int cur = i * 8;
                int prev = (i - 1) * 8;
                AddQuad(prev + 0, prev + 1, cur + 1, cur + 0); // Jagged
                AddQuad(prev + 3, prev + 2, cur + 2, cur + 3); // Front
                AddQuad(prev + 4, prev + 5, cur + 5, cur + 4); // Outer
            }
        }

        // 2. Setup Physics Memory Bridge
        btIndexedMesh meshPart;
        meshPart.m_numTriangles = (int)m_indices.size() / 3;
        meshPart.m_triangleIndexBase = (const unsigned char*)m_indices.data();
        meshPart.m_triangleIndexStride = 3 * sizeof(short);
        meshPart.m_numVertices = (int)m_vertices.size();
        meshPart.m_vertexBase = (const unsigned char*)m_vertices.data();
        meshPart.m_vertexStride = sizeof(CliffVertex);
        meshPart.m_indexType = PHY_SHORT;
        meshPart.m_vertexType = PHY_FLOAT;

        m_pSharedMesh = new btTriangleIndexVertexArray();
        m_pSharedMesh->addIndexedMesh(meshPart, PHY_SHORT);

        // 3. Create Physics Object
        m_pMeshShape = new btBvhTriangleMeshShape(m_pSharedMesh, true, true); // No compression

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(0, startY, 0)); // Start Position

        btDefaultMotionState* ms = new btDefaultMotionState(trans);
        btRigidBody::btRigidBodyConstructionInfo rb(0.0f, ms, m_pMeshShape, btVector3(0, 0, 0));
        m_pBody = new btRigidBody(rb);

        m_pBody->setFriction(0.0f);
        m_pBody->setRestitution(1.0f);
        m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_pBody->setActivationState(DISABLE_DEACTIVATION);

        m_pWorld->addRigidBody(m_pBody, COL_WALL, COL_BALL|COL_BULLET|COL_POWERUP|COL_PADDLE|COL_BLOCK);
        PhysicsData* pData = new PhysicsData{ TYPE_WALL, nullptr };
        m_pBody->setUserPointer(pData);

        // 4. Generate Initial Shape (Start with flat 0 connection)
        Regenerate(startY, startVirtualY);
    }

    ~CArkiCliffTreadmill()
    {
        if (m_pBody) {
            m_pWorld->removeRigidBody(m_pBody);
            if (m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
            delete m_pBody->getMotionState();



            // if (m_pBody && m_pBody->getCollisionShape()) delete m_pBody->getCollisionShape();
            if (m_pBody) delete m_pBody;
        }
        delete m_pMeshShape;
        delete m_pSharedMesh;
		SAFE_RELEASE(m_cliffTexture);
    }

    // 1. Move the wall down by 'speed'
    void Scroll(float dt, float speed)
    {
        m_currentY -= speed * dt;

        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        trans.setOrigin(btVector3(0, m_currentY, 0));
        m_pBody->getMotionState()->setWorldTransform(trans);
    }

    void Animate(float dt, float speed)
    {
        m_scrollAccumulator -= speed * dt;

        int numRows = (int)m_vertices.size() / 8;
        float width = 3.0f;
        float zFront = 2.0f;
        float zBack = -2.0f;

        for (int i = 0; i < numRows; i++)
        {
            float physicalY = m_vertices[i * 8].y;
            float virtualY = physicalY + m_scrollAccumulator;

            float jagged = GetNoiseAt(virtualY);
            float xJagged = m_facingRight ? (m_baseX - jagged) : (m_baseX + jagged);
            float xOuter = m_facingRight ? (m_baseX + width) : (m_baseX - width);

            // --- NORMALS CALCULATION ---

            // 1. Jagged Wall Normal (Dynamic, based on noise slope)
            float nAbove = GetNoiseAt(virtualY + 0.1f);
            float nBelow = GetNoiseAt(virtualY - 0.1f);
            float slope = (nAbove - nBelow);
            float jnx = m_facingRight ? -1.0f : 1.0f;
            float jny = slope * -2.0f;
            float len = sqrt(jnx * jnx + jny * jny);
            jnx /= len; jny /= len;

            // 2. Front Normal (Static Z)
            float fnx = 0, fny = 0, fnz = 1.0f;

            // 3. Outer Normal (Static X)
            float onx = m_facingRight ? 1.0f : -1.0f;

            // --- UPDATE VERTICES (SPLIT LOGIC) ---

            // Shared coordinates for UVs
            float v = virtualY * m_uvScale;

            // -- SET 1: JAGGED WALL (Indices 0, 1) --
            // Uses Jagged Normal
            SetVert(i * 8 + 0, xJagged, physicalY, zFront, jnx, jny, 0, 0, v); // Front-Inner Corner
            SetVert(i * 8 + 1, xJagged, physicalY, zBack, jnx, jny, 0, 1, v); // Back-Inner Corner

            // -- SET 2: FRONT FACE (Indices 2, 3) --
            // Uses Front Normal (Hard Edge!)
            // Index 2 is at xJagged (Same pos as 0, but different normal)
            SetVert(i * 8 + 2, xJagged, physicalY, zFront, fnx, fny, fnz, 0, v);
            // Index 3 is at xOuter
            SetVert(i * 8 + 3, xOuter, physicalY, zFront, fnx, fny, fnz, 1, v);

            // -- SET 3: OUTER FACE (Indices 4, 5) --
            SetVert(i * 8 + 4, xOuter, physicalY, zFront, onx, 0, 0, 0, v);
            SetVert(i * 8 + 5, xOuter, physicalY, zBack, onx, 0, 0, 1, v);

            // (Back face 6,7 ignored for brevity, similar to Front)
        }
        // 3. Update Physics AABB
        if (m_pMeshShape) {
            btVector3 aabbMin(-100, -100, -100);
            btVector3 aabbMax(100, 100, 100);
            m_pMeshShape->refitTree(aabbMin, aabbMax);
            m_pBody->activate(true);
        }
    }

    // NEW REGENERATE FUNCTION
    // We replace 'connectionX' with 'startVirtualY'
    void Regenerate(float newY, float startVirtualY)
    {
        m_currentY = newY;

        // Move Physics immediately
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        trans.setOrigin(btVector3(0, m_currentY, 0));
        m_pBody->getMotionState()->setWorldTransform(trans);

        int numRows = (int)m_vertices.size() / 8;
        float width = 16.0f;
        float zFront = 2.0f;
        float zBack = -8.0f;

        for (int i = 0; i < numRows; i++)
        {
            float localY = m_vertices[i * 8].y; // 0 to Height

            // 1. CALCULATE CONTINUOUS VIRTUAL Y
            // We just add local height to the start. 
            // Since startVirtualY comes from the previous wall's end, it's seamless.
            float myVirtualY = startVirtualY + localY;

            // 2. SAMPLE NOISE
            float jagged = GetNoiseAt(myVirtualY);

            // 3. STORE END POINT
            // If this is the last row, save this Y so the next wall knows where to start
            if (i == numRows - 1) {
                m_topVirtualY = myVirtualY;
            }

            float xJagged = m_facingRight ? (m_baseX - jagged) : (m_baseX + jagged);
            float xOuter = m_facingRight ? (m_baseX + width) : (m_baseX - width);

            float uOuter = width / 4.0f;

            // 4. CALCULATE NORMAL (Using VirtualY for consistency)
            float nNext = GetNoiseAt(myVirtualY + 0.1f);
            float nPrev = GetNoiseAt(myVirtualY - 0.1f);
            float slope = nNext - nPrev;
            float jnx = m_facingRight ? -1.0f : 1.0f;
            float jny = slope * -2.0f;
            float len = sqrt(jnx * jnx + jny * jny);
            jnx /= len; jny /= len;

            // 5. CALCULATE UVs
            // Using myVirtualY ensures texture pattern doesn't reset/seam either!
            float v = myVirtualY * m_uvScale;

            // Update Vertices (Same as before)
            SetVert(i * 8 + 0, xJagged, localY, zFront, jnx, jny, 0, 0, v);
            SetVert(i * 8 + 1, xJagged, localY, zBack, jnx, jny, 0, 1, v);
            SetVert(i * 8 + 2, xJagged, localY, zFront, 0, 0, 1, 0, v);
            SetVert(i * 8 + 3, xOuter, localY, zFront, 0, 0, 1, uOuter, v);
            SetVert(i * 8 + 4, xOuter, localY, zFront, (m_facingRight ? 1 : -1), 0, 0, 0, v);
            SetVert(i * 8 + 5, xOuter, localY, zBack, (m_facingRight ? 1 : -1), 0, 0, 1, v);
        }

        // Refit Physics
        btVector3 aabbMin(-100, -100, -100);
        btVector3 aabbMax(100, 100, 100);
        m_pMeshShape->refitTree(aabbMin, aabbMax);
    }

    void Render(IDirect3DDevice9* device)
    {
        if (m_cliffTexture) device->SetTexture(0, m_cliffTexture);
        // 1. WRAP: When UV is > 1.0, go back to 0.0 (Infinite Tiling)
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        // IMPORTANT: We must apply the body's transform to the graphics!
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        D3DXMATRIXA16 matWorld = ConvertBulletTransform(trans);
        // 3. Apply to Device
        device->SetTransform(D3DTS_WORLD, &matWorld);
        
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha
        device->SetRenderState(D3DRS_LIGHTING, TRUE);



		D3DMATERIAL9 mat;
		InitMaterial(mat, 1.0f, 0.3f,0.6f,1.0f);
		device->SetMaterial(&mat);

        device->SetFVF(CliffVertex::FVF);
        device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
            0, (UINT)m_vertices.size(), (UINT)m_indices.size() / 3,
            m_indices.data(), D3DFMT_INDEX16,
            m_vertices.data(), sizeof(CliffVertex));


    }

private:
    void AddQuad(int tl, int tr, int br, int bl) {
        m_indices.push_back((short)tl); m_indices.push_back((short)tr); m_indices.push_back((short)br);
        m_indices.push_back((short)tl); m_indices.push_back((short)br); m_indices.push_back((short)bl);
    }

    void SetVert(int idx, float x, float y, float z, float nx, float ny, float nz, float u, float v) {
        m_vertices[idx].x = x; m_vertices[idx].y = y; m_vertices[idx].z = z;
        m_vertices[idx].nx = nx; m_vertices[idx].ny = ny; m_vertices[idx].nz = nz;
        m_vertices[idx].u = u; m_vertices[idx].v = v;
    }
};

