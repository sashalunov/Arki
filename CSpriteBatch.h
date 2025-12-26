#pragma once
#include "stdafx.h"
#include <btBulletDynamicsCommon.h>

// Add this definition if btVector2 is not defined elsewhere
struct btVector2
{
    float m_x, m_y;
    btVector2() : m_x(0), m_y(0) {}
    btVector2(float x, float y) : m_x(x), m_y(y) {}
    float x() const { return m_x; }
    float y() const { return m_y; }
    void setX(float x) { m_x = x; }
    void setY(float y) { m_y = y; }
};
// Define how the sprite behaves in 3D space
enum ESpriteMode
{
    SPRITE_BILLBOARD,      // Fully rotates to face camera
    SPRITE_AXIS_Y,         // Rotates around Y axis only (e.g. Trees)
    SPRITE_FLAT_XZ,        // Lays flat on the ground (e.g. Shadow blobs)
    SPRITE_FIXED           // Uses pre-defined rotation (no auto-facing)
};

struct SpriteVertex
{
    float x, y, z;
    D3DCOLOR color;
    float u, v;

    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};

// Internal structure to store draw requests
struct SpriteInfo
{
    btVector3 position;
    btVector3 size;       // X=Width, Y=Height
    IDirect3DTexture9* texture;
    D3DCOLOR color;
    ESpriteMode mode;
    float rotation2D;     // Optional Z-roll (in radians)
    float distToCamera;   // For sorting

    // Helper for sorting back-to-front
    bool operator < (const SpriteInfo& other) const {
        return distToCamera > other.distToCamera;
    }
};

class CSpriteBatch
{
private:
    IDirect3DDevice9* m_device;
    IDirect3DVertexBuffer9* m_vb;

    // Batching Limits
    static const int MAX_BATCH_SIZE = 2048; // Sprites per draw call
    SpriteVertex m_cpuVertices[MAX_BATCH_SIZE * 6]; // 6 verts per sprite (2 tris)

    // Storage for the current frame
    std::vector<SpriteInfo> m_sprites;

    // Camera state cache
    btVector3 m_camPos;
    btVector3 m_camRight;
    btVector3 m_camUp;
    btVector3 m_camFwd;

public:
    CSpriteBatch();
    ~CSpriteBatch();

    // Initialize the dynamic buffer
    void Init(IDirect3DDevice9* device);
    void Invalidate(); // Call on LostDevice

    // --- Frame Workflow ---

    // 1. Begin the batch. Pass the camera to calculate billboarding vectors.
    void Begin(const D3DXMATRIX& viewMatrix);

    // 2. Add a sprite to the list
    void Draw(IDirect3DTexture9* tex, const btVector3& pos, const btVector2& size = btVector2(1, 1),
        D3DCOLOR color = 0xFFFFFFFF, ESpriteMode mode = SPRITE_BILLBOARD);

    // 3. Process, Sort, and Render all sprites
    void End();
    // Call this BEFORE device->Reset()
    void OnLostDevice()
    {
        SAFE_RELEASE(m_vb);
    }

    // Call this AFTER device->Reset()
    void OnResetDevice()
    {
        // Re-create the buffer exactly as we did in the constructor
        if (!m_vb)
        {
            m_device->CreateVertexBuffer(
                MAX_BATCH_SIZE * 6 * sizeof(SpriteVertex),
                D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                SpriteVertex::FVF,
                D3DPOOL_DEFAULT,
                &m_vb, NULL);
        }
    }

private:
    void Flush(); // Internal render execution

    D3DXVECTOR3 GetCameraPos(const D3DXMATRIX& matView)
    {
        D3DXMATRIX matInvView;
        D3DXVECTOR3 cameraPos;

        // The Inverse of the View Matrix is the "Camera's World Matrix"
        // The bottom row of a World Matrix is always the position.
        D3DXMatrixInverse(&matInvView, NULL, &matView);

        cameraPos.x = matInvView._41;
        cameraPos.y = matInvView._42;
        cameraPos.z = matInvView._43;

        return cameraPos;
    }

};

