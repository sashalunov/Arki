#pragma once
#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "CArkiBlock.h"

// Static noise buffer
static float s_noiseBuffer[1024];
static bool s_noiseInitialized = false;

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
class CArkiCliff
{
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
public:
    CArkiCliff(btDiscreteDynamicsWorld* world, float startY, float endY, float xPos, bool facingRight)
    {
        InitNoise();

        m_pWorld = world;
        m_baseX = xPos;
        m_facingRight = facingRight;
        m_scrollAccumulator = 0.0f;
        m_uvScale = 0.5f; // Texture repeats every 2 units

        m_pBody = nullptr;
        m_pMeshShape = nullptr;
        m_pSharedMesh = nullptr;

        int segments = 40;
        float height = endY - startY;
        float segHeight = height / segments;

        // We use 8 Vertices per row to allow Hard Edges (Split Normals)
         // 0-1: Jagged Wall Face (Normal varies)
         // 2-3: Front Face (Normal Z)
         // 4-5: Outer Wall Face (Normal X)
         // 6-7: Back Face (Normal -Z)

        for (int i = 0; i <= segments; i++)
        {
            float y = startY + (i * segHeight);
            D3DCOLOR c = D3DCOLOR_XRGB(255, 255, 255);

            // Push 8 placeholders (Positions set in Animate)
            for (int k = 0; k < 8; k++)
                m_vertices.push_back({ 0, y, 0, 0,0,0, c, 0,0 });

            if (i > 0) {
                int cur = i * 8;
                int prev = (i - 1) * 8;

                // 1. Jagged Wall (Connects 0 & 1)
                AddQuad(prev + 0, prev + 1, cur + 1, cur + 0);

                // 2. Front Face (Connects 2 & 3)
                // Note: 2 is Inner (Jagged side), 3 is Outer
                AddQuad(prev + 3, prev + 2, cur + 2, cur + 3);

                // 3. Outer Face (Connects 4 & 5)
                AddQuad(prev + 4, prev + 5, cur + 5, cur + 4);

                // 4. Back Face (Connects 6 & 7)
                // AddQuad(prev+7, prev+6, cur+6, cur+7); // Optional
            }
        }

        Animate(0, 0);

        // Physics Setup (Same as before, Logic works fine with duplicate verts)
        btIndexedMesh meshPart;
        meshPart.m_numTriangles = m_indices.size() / 3;
        meshPart.m_triangleIndexBase = (const unsigned char*)m_indices.data();
        meshPart.m_triangleIndexStride = 3 * sizeof(short);
        meshPart.m_numVertices = m_vertices.size();
        meshPart.m_vertexBase = (const unsigned char*)m_vertices.data();
        meshPart.m_vertexStride = sizeof(CliffVertex);
        meshPart.m_indexType = PHY_SHORT;
        meshPart.m_vertexType = PHY_FLOAT;

        m_pSharedMesh = new btTriangleIndexVertexArray();
        m_pSharedMesh->addIndexedMesh(meshPart, PHY_SHORT);
        m_pMeshShape = new btBvhTriangleMeshShape(m_pSharedMesh, true, true);

        btTransform trans; trans.setIdentity();
        btDefaultMotionState* ms = new btDefaultMotionState(trans);
        btRigidBody::btRigidBodyConstructionInfo rb(0.0f, ms, m_pMeshShape, btVector3(0, 0, 0));
        m_pBody = new btRigidBody(rb);
        m_pBody->setFriction(0.0f);
        m_pBody->setRestitution(1.0f);
        m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_pBody->setActivationState(DISABLE_DEACTIVATION);
        m_pWorld->addRigidBody(m_pBody);
        PhysicsData* pData = new PhysicsData{ TYPE_WALL, nullptr };
        m_pBody->setUserPointer(pData);
    }

    ~CArkiCliff() {
        if (m_pBody) {
            m_pWorld->removeRigidBody(m_pBody);
            if (m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
            delete m_pBody->getMotionState();
            delete m_pBody;
        }
        delete m_pMeshShape;
        delete m_pSharedMesh;
    }

    void Animate(float dt, float speed)
    {
        m_scrollAccumulator -= speed * dt;

        int numRows = m_vertices.size() / 8;
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

    void Render(IDirect3DDevice9* device, IDirect3DTexture9* texture = nullptr)
    {
        if (texture) device->SetTexture(0, texture);
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
        D3DXMATRIX matIdentity;
        D3DXMatrixIdentity(&matIdentity);
        device->SetTransform(D3DTS_WORLD, &matIdentity);
        device->SetFVF(CliffVertex::FVF);


        device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
            0, m_vertices.size(), m_indices.size() / 3,
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

