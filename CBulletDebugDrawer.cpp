#include "stdafx.h"
#include "CBulletDebugDrawer.h"



CBulletDebugDrawer::CBulletDebugDrawer(LPDIRECT3DDEVICE9 device)
    : m_pDevice(device), m_debugMode(0)
{
}

void CBulletDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    // Convert Bullet Color (0.0 - 1.0) to DirectX Color (0 - 255)
    D3DCOLOR d3dColor = D3DCOLOR_XRGB(
        (int)(color.x() * 255.0f),
        (int)(color.y() * 255.0f),
        (int)(color.z() * 255.0f)
    );

    // Create Start Vertex
    DebugVertex v1;
    v1.x = (FLOAT)from.x(); v1.y = (FLOAT)from.y(); v1.z = (FLOAT)from.z();
    v1.color = d3dColor;

    // Create End Vertex
    DebugVertex v2;
    v2.x = (FLOAT)to.x(); v2.y = (FLOAT)to.y(); v2.z = (FLOAT)to.z();
    v2.color = d3dColor;

    // Add to buffer
     m_lineBuffer.push_back(v1);
    m_lineBuffer.push_back(v2);
}

void CBulletDebugDrawer::Draw()
{
    if (m_lineBuffer.empty()) return;

    // 1. Disable Lighting (we want pure colored lines)
    m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    // --- DISABLE Z-BUFFER ---
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    // --- FIX START: UNBIND TEXTURES ---
    // Unbind any texture left over from the Particle System
    m_pDevice->SetTexture(0, NULL);

    // Optional: Ensure the GPU uses the Vertex Color (DIFFUSE) and ignores texture stages
    // This effectively says: "Output = VertexColor"
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    // --- FIX END ---
    // 2. Set Vertex Format
    m_pDevice->SetFVF(DebugVertex::FVF);
    // 3. Draw all lines in one go
    // D3DPT_LINELIST means every 2 vertices make 1 line
    // Primitive Count = Total Vertices / 2
    m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, m_lineBuffer.size() / 2, &m_lineBuffer[0], sizeof(DebugVertex));
    m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    // 4. Restore Lighting (optional, good practice)
    m_pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    // Restore default texture blending (Modulate) so other objects render correctly
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);


    // 5. Clear buffer for the next frame
    m_lineBuffer.clear();
}

// --- Required Stubs ---

void CBulletDebugDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
    // Optional: Draw a small line representing the contact normal
    btVector3 to = PointOnB + normalOnB * 1.0f; // 1 meter long normal
    drawLine(PointOnB, to, color);
}

void CBulletDebugDrawer::reportErrorWarning(const char* warningString)
{
    OutputDebugStringA(warningString); // Print to Visual Studio Output
}

void CBulletDebugDrawer::draw3dText(const btVector3& location, const char* textString)
{
    // Leaving empty. Requires ID3DXFont to implement properly.
}

void CBulletDebugDrawer::setDebugMode(int debugMode)
{
    m_debugMode = debugMode;
}

int CBulletDebugDrawer::getDebugMode() const
{
    return m_debugMode;
}

