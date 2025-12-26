#pragma once

struct RibbonVertex {
    float x, y, z;
    D3DCOLOR color;
    float u, v; // Texture Coordinates

    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};

class СRibbonRenderer
{
private:
    LPDIRECT3DDEVICE9 m_device;
    LPDIRECT3DVERTEXBUFFER9 m_vb;
    LPDIRECT3DTEXTURE9 m_texture;
    int m_bufferSize;
    int m_offset;

public:
    СRibbonRenderer(LPDIRECT3DDEVICE9 device, int maxVerts)
        : m_device(device), m_bufferSize(maxVerts), m_offset(0)
    {
        device->CreateVertexBuffer(
            maxVerts * sizeof(RibbonVertex),
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
            RibbonVertex::FVF,
            D3DPOOL_DEFAULT,
            &m_vb, NULL
        );
        D3DXCreateTextureFromFile(device, L"trail.png", &m_texture);
    }

    void DrawTrails(const std::vector<Boid>& boids, const D3DXVECTOR3& camPos)
    {
        std::vector<RibbonVertex> verts;
        float ribbonWidth = 0.5f;

        // --- GEOMETRY GENERATION (CPU) ---
        for (const auto& boid : boids)
        {
            if (boid.trail.size() < 2) continue;

            for (size_t i = 0; i < boid.trail.size() - 1; i++)
            {
                D3DXVECTOR3 p1 = boid.trail[i];
                D3DXVECTOR3 p2 = boid.trail[i + 1];

                // 1. Calculate Direction of this segment
                D3DXVECTOR3 dir = p2 - p1;

                // 2. Calculate Vector to Camera
                D3DXVECTOR3 toCam = camPos - p1;

                // 3. Calculate "Right" Vector (Cross Product)
                D3DXVECTOR3 right;
                D3DXVec3Cross(&right, &toCam, &dir);
                D3DXVec3Normalize(&right, &right);

                // Expand width
                right *= ribbonWidth;

                // 4. Calculate UVs (0.0 at head, 1.0 at tail)
                float u1 = (float)i / (float)boid.trail.size();
                float u2 = (float)(i + 1) / (float)boid.trail.size();

                // 5. Fade Alpha based on trail position (Head = Opaque, Tail = Transparent)
                int alpha1 = (int)((1.0f - u1) * 255);
                int alpha2 = (int)((1.0f - u2) * 255);
                D3DCOLOR c1 = D3DCOLOR_ARGB(alpha1, 255, 100, 50);
                D3DCOLOR c2 = D3DCOLOR_ARGB(alpha2, 255, 100, 50);

                // 6. Build 2 Triangles (6 Verts) for this segment
                // Quad: TL, TR, BL, BR
                D3DXVECTOR3 TL = p1 + right;
                D3DXVECTOR3 TR = p1 - right;
                D3DXVECTOR3 BL = p2 + right;
                D3DXVECTOR3 BR = p2 - right;

                // Triangle 1
                verts.push_back({ TL.x, TL.y, TL.z, c1, 0.0f, u1 });
                verts.push_back({ TR.x, TR.y, TR.z, c1, 1.0f, u1 });
                verts.push_back({ BL.x, BL.y, BL.z, c2, 0.0f, u2 });

                // Triangle 2
                verts.push_back({ TR.x, TR.y, TR.z, c1, 1.0f, u1 });
                verts.push_back({ BR.x, BR.y, BR.z, c2, 1.0f, u2 });
                verts.push_back({ BL.x, BL.y, BL.z, c2, 0.0f, u2 });
            }
        }

        if (verts.empty()) return;

        // --- RENDERING (GPU) ---
        // Setup States
        m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE); // Additive looks best for trails
        m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); // Important! Ribbons have no "back"

        m_device->SetTexture(0, m_texture);
        m_device->SetFVF(RibbonVertex::FVF);
        m_device->SetStreamSource(0, m_vb, 0, sizeof(RibbonVertex));

        // Ring Buffer Lock/Copy (Standard logic)
        void* pData;
        int count = (int)verts.size();
        DWORD flags = D3DLOCK_NOOVERWRITE;
        if (m_offset + count >= m_bufferSize) {
            m_offset = 0; flags = D3DLOCK_DISCARD;
        }

        m_vb->Lock(m_offset * sizeof(RibbonVertex), count * sizeof(RibbonVertex), &pData, flags);
        memcpy(pData, verts.data(), count * sizeof(RibbonVertex));
        m_vb->Unlock();

        // Draw
        m_device->DrawPrimitive(D3DPT_TRIANGLELIST, m_offset, count / 3);

        m_offset += count;

        // Restore Culling
        m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
        m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    }
};