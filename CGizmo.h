#pragma once
#include <btBulletDynamicsCommon.h>
#include "CRigidBody.h"
#include "stdafx.h"

enum EGizmoAxis { AXIS_NONE, AXIS_X, AXIS_Y, AXIS_Z };

class CGizmo
{
private:
    CRigidBody* m_pTarget;
    EGizmoAxis   m_activeAxis;
    EGizmoAxis   m_hoverAxis;

    // Visuals
    ID3DXMesh* m_pArrowMesh; // Simple cylinder/cone mesh

    // --- NEW: Snapping State ---
    bool  m_snapEnabled;
    float m_snapSize;       // e.g. 1.0 units
    float m_gizmoSize;
    // Helper to store the offset between object center and mouse click
    // This prevents the object's center from "popping" to the mouse cursor instantly.
    btVector3 m_dragOffset;

public:
    CGizmo(IDirect3DDevice9* device) : m_pTarget(NULL), m_activeAxis(AXIS_NONE), m_hoverAxis(AXIS_NONE)
    {
        m_gizmoSize = 3.0f;
        // Create a cylinder to represent arrows
        D3DXCreateCylinder(device, 0.05f, 0.05f, m_gizmoSize, 8, 1, &m_pArrowMesh, NULL);
    }

    void SetTarget(CRigidBody* obj) { m_pTarget = obj; }

    // Toggle Snapping (Call this when user holds CTRL)
    void SetSnapping(bool enabled, float size = 1.0f) {
        m_snapEnabled = enabled;
        m_snapSize = size;
    }

    bool IsActive() const { return m_activeAxis != AXIS_NONE; }
    // --------------------------------------------------------
    // MOUSE INTERACTION
    // --------------------------------------------------------
    
    // 1. Check if mouse is hovering over an axis
    void UpdateHover(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDir)
    {
        if (!m_pTarget) return;
        m_hoverAxis = AXIS_NONE;

        btVector3 pos = m_pTarget->GetPosition();
        D3DXVECTOR3 center(pos.x(), pos.y(), pos.z());

        // Check each axis (Simplified cylinder/line distance check)
        // Note: In production, check X, Y, Z and pick the CLOSEST one.
        float distX = RayAxisDist(rayOrigin, rayDir, center, D3DXVECTOR3(1, 0, 0));
        float distY = RayAxisDist(rayOrigin, rayDir, center, D3DXVECTOR3(0, 1, 0));
        float distZ = RayAxisDist(rayOrigin, rayDir, center, D3DXVECTOR3(0, 0, 1));

        float limit = 0.2f; // Selection radius
        if (distX < limit) m_hoverAxis = AXIS_X;
        else if (distY < limit) m_hoverAxis = AXIS_Y;
        else if (distZ < limit) m_hoverAxis = AXIS_Z;
    }

    // 2. Begin Dragging
    void OnMouseDown(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDir)
    {
        m_activeAxis = m_hoverAxis;

        if (m_pTarget && m_activeAxis != AXIS_NONE)
        {
            // Calculate initial offset so object doesn't jump
            btVector3 objPos = m_pTarget->GetPosition();
            D3DXVECTOR3 axisDir = GetAxisVector(m_activeAxis);
            D3DXVECTOR3 hitPoint = GetPointOnAxis(rayOrigin, rayDir, D3DXVECTOR3(objPos.x(), objPos.y(), objPos.z()), axisDir);

            // Store offset: ObjectPosition - HitPoint
            m_dragOffset = objPos - btVector3(hitPoint.x, hitPoint.y, hitPoint.z);
        }
    }

    void OnMouseUp() { m_activeAxis = AXIS_NONE; }

    // --------------------------------------------------------
    // MOVEMENT LOGIC (With Snapping)
    // --------------------------------------------------------
    void UpdateDrag(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDir)
    {
        if (!m_pTarget || m_activeAxis == AXIS_NONE) return;

        btVector3 currentPos = m_pTarget->GetPosition();
        D3DXVECTOR3 axisDir = GetAxisVector(m_activeAxis);
        D3DXVECTOR3 origin(currentPos.x(), currentPos.y(), currentPos.z());

        // 1. Find where mouse is on the axis
        // We project the mouse ray onto the infinite line defined by the axis
        // But we must ignore the current object position for the projection origin 
        // to handle "absolute" movement correctly.

        // Better Math: Project onto line passing through the ORIGINAL click point? 
        // Simplest: Project onto line passing through current object center.
        D3DXVECTOR3 hitPoint = GetPointOnAxis(rayOrigin, rayDir, origin, axisDir);

        // 2. Add the offset we calculated on MouseDown
        // This makes movement relative to where you grabbed the arrow
        btVector3 newPos(hitPoint.x, hitPoint.y, hitPoint.z);
        newPos += m_dragOffset;

        // 3. Constrain to Axis (Only update the active component)
        // The math above might return a point slightly off-axis due to precision.
        // We rigidly force the other axes to stay the same.
        if (m_activeAxis == AXIS_X) {
            newPos.setY(currentPos.y());
            newPos.setZ(currentPos.z());
        }
        else if (m_activeAxis == AXIS_Y) {
            newPos.setX(currentPos.x());
            newPos.setZ(currentPos.z());
        }
        else if (m_activeAxis == AXIS_Z) {
            newPos.setX(currentPos.x());
            newPos.setY(currentPos.y());
        }

        // 4. APPLY SNAPPING
        if (m_snapEnabled)
        {
            // Helper Lambda for rounding
            auto SnapVal = [&](float val) -> float {
                return floor(val / m_snapSize + 0.5f) * m_snapSize;
                };

            if (m_activeAxis == AXIS_X) newPos.setX(SnapVal(newPos.x()));
            if (m_activeAxis == AXIS_Y) newPos.setY(SnapVal(newPos.y()));
            if (m_activeAxis == AXIS_Z) newPos.setZ(SnapVal(newPos.z()));
        }

        m_pTarget->SetPosition(newPos);
    }

    void Render(IDirect3DDevice9* device)
    {
        if (!m_pTarget) return;

        btVector3 pos = m_pTarget->GetPosition();
        D3DXMATRIX t, r, world;
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

        device->SetRenderState(D3DRS_ZENABLE, FALSE); // Always draw on top
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
        //device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);//D3DTOP_SELECTARG1
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // Use vertex alpha
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

        device->SetTexture(0, NULL);
        D3DXMatrixIdentity(&r);
        device->SetTransform(D3DTS_TEXTURE0, &r);
        // Tell DX9 this is a Cube Map (requires 3D coordinates)
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);


        // Draw X Axis (Red)
        D3DXMatrixRotationY(&r, -D3DX_PI / 2.0f); // Rotate cylinder to point X
        D3DXMatrixTranslation(&t, pos.x() + m_gizmoSize*0.5f, pos.y(), pos.z()); // Offset so it starts at center
        world = r * t;
        device->SetTransform(D3DTS_WORLD, &world);
        // Highlight if selected
        D3DCOLOR color = (m_hoverAxis == AXIS_X || m_activeAxis == AXIS_X) ? 0xFFFFFF00 : 0xFFFF0000;
        DrawGizmoArrow(device, color);

        // --- Draw Y Axis (Green) ---
            // Cylinder is Z-aligned. Rotate -90 deg around X to point it along Y.
        D3DXMatrixRotationX(&r, -D3DX_PI / 2.0f);
        D3DXMatrixTranslation(&t, pos.x(), pos.y() + m_gizmoSize * 0.5f, pos.z());
        world = r * t;
        device->SetTransform(D3DTS_WORLD, &world);
        D3DCOLOR yColor = (m_hoverAxis == AXIS_Y || m_activeAxis == AXIS_Y) ? 0xFFFFFF00 : 0xFF00FF00;
        DrawGizmoArrow(device, yColor);

        // --- Draw Z Axis (Blue) ---
            // Cylinder is ALREADY Z-aligned. No rotation needed, just translation.
        D3DXMatrixIdentity(&r); // No rotation needed
        D3DXMatrixTranslation(&t, pos.x(), pos.y(), pos.z() + m_gizmoSize * 0.5f);
        world = t;
        device->SetTransform(D3DTS_WORLD, &world);

        D3DCOLOR zColor = (m_hoverAxis == AXIS_Z || m_activeAxis == AXIS_Z) ? 0xFFFFFF00 : 0xFF0000FF;
        DrawGizmoArrow(device, zColor);
        // ---------------------------------------------------------
        // CLEANUP: Restore states so we don't break the rest of the engine
        // ---------------------------------------------------------
        device->SetRenderState(D3DRS_ZENABLE, TRUE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

        // CRITICAL FIX: Reset World Matrix to Identity
        D3DXMatrixIdentity(&world);
        device->SetTransform(D3DTS_WORLD, &world);

    }

private:
    void DrawGizmoArrow(IDirect3DDevice9* dev, D3DCOLOR color) {
        D3DMATERIAL9 mtrl;
        ZeroMemory(&mtrl, sizeof(mtrl));

        // Extract ARGB
        float a = ((color >> 24) & 0xff) / 255.0f;
        float r = ((color >> 16) & 0xff) / 255.0f;
        float g = ((color >> 8) & 0xff) / 255.0f;
        float b = (color & 0xff) / 255.0f;

        // Set Diffuse and Ambient to the color
        mtrl.Diffuse.r = mtrl.Ambient.r = r;
        mtrl.Diffuse.g = mtrl.Ambient.g = g;
        mtrl.Diffuse.b = mtrl.Ambient.b = b;
        mtrl.Diffuse.a = mtrl.Ambient.a = a;

        // CRITICAL: Set Emissive. This makes it look bright/flat even without scene lights.
        mtrl.Emissive = mtrl.Diffuse;

        dev->SetMaterial(&mtrl);
        m_pArrowMesh->DrawSubset(0);
    }
    // Helper to return (1,0,0), (0,1,0) etc.
    D3DXVECTOR3 GetAxisVector(EGizmoAxis axis) {
        if (axis == AXIS_X) return D3DXVECTOR3(1, 0, 0);
        if (axis == AXIS_Y) return D3DXVECTOR3(0, 1, 0);
        if (axis == AXIS_Z) return D3DXVECTOR3(0, 0, 1);
        return D3DXVECTOR3(0, 0, 0);
    }
    // Helper: Distance between two lines (Ray and Axis)
    float RayAxisDist(D3DXVECTOR3 rO, D3DXVECTOR3 rD, D3DXVECTOR3 axisO, D3DXVECTOR3 axisD) {
        // Cross product logic to find distance between skew lines
        D3DXVECTOR3 w0 = rO - axisO;
        float a = D3DXVec3Dot(&rD, &rD);
        float b = D3DXVec3Dot(&rD, &axisD);
        float c = D3DXVec3Dot(&axisD, &axisD);
        float d = D3DXVec3Dot(&rD, &w0);
        float e = D3DXVec3Dot(&axisD, &w0);
        float sc = (b * e - c * d) / (a * c - b * b);
        D3DXVECTOR3 Pt = rO + rD * sc; // Closest point on Ray
        // Now project Pt onto axis line to check if it's within the arrow length (0 to 1.0)
        D3DXVECTOR3 v = Pt - axisO;
        float t = D3DXVec3Dot(&v, &axisD);
        if (t < 0.0f || t > 1.0f) return 10000.0f; // Too far along axis

        D3DXVECTOR3 Paxis = axisO + axisD * t;
        return D3DXVec3Length(&(Pt - Paxis));
    }

    D3DXVECTOR3 GetPointOnAxis(D3DXVECTOR3 rO, D3DXVECTOR3 rD, D3DXVECTOR3 axisO, D3DXVECTOR3 axisD) {
        // Similar math to above, finding the point on the axis closest to the mouse ray
        D3DXVECTOR3 w0 = rO - axisO;
        float b = D3DXVec3Dot(&rD, &axisD);
        float c = D3DXVec3Dot(&axisD, &axisD);
        float d = D3DXVec3Dot(&rD, &w0);
        float e = D3DXVec3Dot(&axisD, &w0);
        // Simple projection if we assume camera ray and axis aren't parallel
        // Solve for parameter on axis
        float t = (b * d - D3DXVec3Dot(&rD, &rD) * e) / (D3DXVec3Dot(&rD, &rD) * c - b * b); // Approx
        // Actually, for robust movement, we usually intersect Ray with a Plane defined by the Axis and the Camera Right vector.
        return axisO + axisD * t; // (Simplified)
    }
};