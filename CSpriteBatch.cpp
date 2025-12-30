#include "StdAfx.h"
#include "CSpriteBatch.h"

CSpriteBatch::CSpriteBatch() : m_device(NULL), m_vb(NULL) {
	ZeroMemory(&m_cpuVertices, sizeof(m_cpuVertices));
}

CSpriteBatch::~CSpriteBatch() { Invalidate(); }

void CSpriteBatch::Init(IDirect3DDevice9* device)
{
    m_device = device;
    // Create a DYNAMIC vertex buffer. 
    // D3DUSAGE_DYNAMIC + D3DUSAGE_WRITEONLY + D3DPOOL_DEFAULT is standard for batching.
    if (FAILED(m_device->CreateVertexBuffer(
        MAX_BATCH_SIZE * 6 * sizeof(SpriteVertex),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        SpriteVertex::FVF,
        D3DPOOL_DEFAULT,
        &m_vb, NULL)))
    {
        // Handle error
    }
}

void CSpriteBatch::Invalidate()
{
	SAFE_RELEASE(m_vb);
}

void CSpriteBatch::Begin(const D3DXMATRIX& viewMatrix)
{
    // Clear previous frame list
    m_sprites.clear();

	D3DXVECTOR3 cp = GetCameraPos(viewMatrix);
	m_camPos = btVector3(cp.x, cp.y, cp.z);
    //m_camPos = cameraPos;
    // Extract Camera Basis Vectors from View Matrix
    // The View Matrix is orthogonal. Row 1 is Right, Row 2 is Up, Row 3 is Forward.
    // (Assuming D3DX Matrix layout)
    m_camRight = btVector3(viewMatrix._11, viewMatrix._21, viewMatrix._31);
    m_camUp = btVector3(viewMatrix._12, viewMatrix._22, viewMatrix._32);
    m_camFwd = btVector3(viewMatrix._13, viewMatrix._23, viewMatrix._33);
}

void CSpriteBatch::Draw(IDirect3DTexture9* tex, const btVector3& pos, const btVector2& size,
    D3DCOLOR color, ESpriteMode mode)
{
    SpriteInfo info;
    info.texture = tex;
    info.position = pos;
    info.size = btVector3(size.x(), size.y(), 0);
    info.color = color;
    info.mode = mode;
    info.rotation2D = 0.0f;

    // Calculate distance for sorting later
    info.distToCamera = (FLOAT)m_camPos.distance2(pos);

    m_sprites.push_back(info);
}

void CSpriteBatch::End()
{
    if (m_sprites.empty()) return;

    // 1. Sort Sprites Back-to-Front
    // This is crucial for Alpha Blending to look correct.
    std::sort(m_sprites.begin(), m_sprites.end());

    // 2. Setup Device State
    m_device->SetFVF(SpriteVertex::FVF);
    m_device->SetStreamSource(0, m_vb, 0, sizeof(SpriteVertex));

    // Enable Alpha Blending
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // Disable Z-Write for transparent sprites (usually preferred)
    // You keep Z-Test ON, but turn Z-Write OFF.
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    // multiply texture alpha by vertex diffuse alpha
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha

    // 3. Batching Loop
    int currentBatchCount = 0;
    IDirect3DTexture9* currentTex = NULL;

    // We map the buffer with 'NOOVERWRITE' initially to append
    // If we fill it, we use 'DISCARD' to reset.
    SpriteVertex* pVerts = m_cpuVertices;

    for (size_t i = 0; i < m_sprites.size(); ++i)
    {
        SpriteInfo& spr = m_sprites[i];

        // If texture changes or buffer is full, flush!
        if (currentTex != spr.texture || currentBatchCount >= MAX_BATCH_SIZE)
        {
            if (currentBatchCount > 0)
            {
                // Write to GPU and Draw
                void* pGPUData;
                m_vb->Lock(0, currentBatchCount * 6 * sizeof(SpriteVertex), &pGPUData, D3DLOCK_DISCARD);
                memcpy(pGPUData, m_cpuVertices, currentBatchCount * 6 * sizeof(SpriteVertex));
                m_vb->Unlock();

                m_device->SetTexture(0, currentTex);
                m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, currentBatchCount * 2);
            }

            // Reset batch
            currentBatchCount = 0;
            currentTex = spr.texture;
            pVerts = m_cpuVertices; // Reset pointer
        }

        // --- Calculate Corners based on Mode ---
        btVector3 right, up;

        if (spr.mode == SPRITE_BILLBOARD)
        {
            right = m_camRight;
            up = m_camUp;
        }
        else if (spr.mode == SPRITE_AXIS_Y)
        {
            // Lock Up to World Y
            up = btVector3(0, 1, 0);
            // Right is Perpendicular to LookDir and Up
            btVector3 look = spr.position - m_camPos;
            look.setY(0); // Ignore height diff
            look.normalize();
            right = up.cross(look); // In Bullet/OpenGL RH, Y x Z = X
        }
        else if (spr.mode == SPRITE_FLAT_XZ)
        {
            right = btVector3(1, 0, 0);
            up = btVector3(0, 0, 1); // "Up" in texture space maps to Z in world
        }

        // Scale vectors by half-size
        btVector3 w = right * (spr.size.x() * 0.5f);
        btVector3 h = up * (spr.size.y() * 0.5f);

        // Calculate 4 corners
        btVector3 bl = spr.position - w - h; // Bottom Left
        btVector3 tl = spr.position - w + h; // Top Left
        btVector3 tr = spr.position + w + h; // Top Right
        btVector3 br = spr.position + w - h; // Bottom Right

        // Fill Vertex Buffer (2 Triangles = 6 Verts)
        // Triangle 1
        pVerts->x = (FLOAT)bl.x(); pVerts->y = (FLOAT)bl.y(); pVerts->z = (FLOAT)bl.z();
        pVerts->color = spr.color; pVerts->u = 0.0f; pVerts->v = 1.0f; pVerts++;

        pVerts->x = (FLOAT)tl.x(); pVerts->y = (FLOAT)tl.y(); pVerts->z = (FLOAT)tl.z();
        pVerts->color = spr.color; pVerts->u = 0.0f; pVerts->v = 0.0f; pVerts++;

        pVerts->x = (FLOAT)tr.x(); pVerts->y = (FLOAT)tr.y(); pVerts->z = (FLOAT)tr.z();
        pVerts->color = spr.color; pVerts->u = 1.0f; pVerts->v = 0.0f; pVerts++;

        // Triangle 2
        pVerts->x = (FLOAT)bl.x(); pVerts->y = (FLOAT)bl.y(); pVerts->z = (FLOAT)bl.z();
        pVerts->color = spr.color; pVerts->u = 0.0f; pVerts->v = 1.0f; pVerts++;

        pVerts->x = (FLOAT)tr.x(); pVerts->y = (FLOAT)tr.y(); pVerts->z = (FLOAT)tr.z();
        pVerts->color = spr.color; pVerts->u = 1.0f; pVerts->v = 0.0f; pVerts++;

        pVerts->x = (FLOAT)br.x(); pVerts->y = (FLOAT)br.y(); pVerts->z = (FLOAT)br.z();
        pVerts->color = spr.color; pVerts->u = 1.0f; pVerts->v = 1.0f; pVerts++;

        currentBatchCount++;
    }

    // Flush remaining
    if (currentBatchCount > 0)
    {
        void* pGPUData;
        m_vb->Lock(0, currentBatchCount * 6 * sizeof(SpriteVertex), &pGPUData, D3DLOCK_DISCARD);
        memcpy(pGPUData, m_cpuVertices, currentBatchCount * 6 * sizeof(SpriteVertex));
        m_vb->Unlock();

        m_device->SetTexture(0, currentTex);
        m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, currentBatchCount * 2);
    }

    // Restore State
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}