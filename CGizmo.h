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
    btVector3 m_startDragPos; // NEW: Stores object position at start of drag

public:
    CGizmo(IDirect3DDevice9* device) : m_pTarget(NULL), m_activeAxis(AXIS_NONE), m_hoverAxis(AXIS_NONE)
    {
        m_snapEnabled = false;
		m_snapSize = 1.0f;
        m_gizmoSize = 3.0f;
        // Create a cylinder to represent arrows
        D3DXCreateCylinder(device, 0.25f, 0.05f, m_gizmoSize, 8, 1, &m_pArrowMesh, NULL);
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
        D3DXVECTOR3 center((FLOAT)pos.x(), (FLOAT)pos.y(), (FLOAT)pos.z());

        // 1. Calculate Distances
        // Pass normalized axis vectors!
        float distX = RayAxisDist(rayOrigin, rayDir, center, D3DXVECTOR3(1, 0, 0));
        float distY = RayAxisDist(rayOrigin, rayDir, center, D3DXVECTOR3(0, 1, 0));
        float distZ = RayAxisDist(rayOrigin, rayDir, center, D3DXVECTOR3(0, 0, 1));

        // 2. Adjust Pick Radius
        // Use a value closer to the visual thickness of the arrow (plus some margin)
        // Arrow radius is 0.05f. Let's give it 0.2f margin.
        float pickRadius = 0.25f;

        // 3. Find Best Candidate
        float closestDist = pickRadius;
        EGizmoAxis bestAxis = AXIS_NONE;

        if (distX < closestDist) { closestDist = distX; bestAxis = AXIS_X; }
        if (distY < closestDist) { closestDist = distY; bestAxis = AXIS_Y; }
        if (distZ < closestDist) { closestDist = distZ; bestAxis = AXIS_Z; }

        m_hoverAxis = bestAxis;
    }
    // 2. Begin Dragging
    void OnMouseDown(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayDir)
    {
        m_activeAxis = m_hoverAxis;

        if (m_pTarget && m_activeAxis != AXIS_NONE)
        {
            // 1. Capture the Starting Position (Static Reference)
            m_startDragPos = m_pTarget->GetPosition();

            // Calculate initial offset so object doesn't jump
            //btVector3 objPos = m_pTarget->GetPosition();
            D3DXVECTOR3 axisDir = GetAxisVector(m_activeAxis);


            // Use startDragPos as the origin for the axis line
            D3DXVECTOR3 startOrigin((FLOAT)m_startDragPos.x(), (FLOAT)m_startDragPos.y(), (FLOAT)m_startDragPos.z());


            D3DXVECTOR3 hitPoint = GetPointOnAxis(rayOrigin, rayDir, startOrigin, axisDir);
            // Calculate offset: Center - HitPoint
            // This ensures if you click the tip of the arrow, the object doesn't snap to center.
            m_dragOffset = m_startDragPos - btVector3(hitPoint.x, hitPoint.y, hitPoint.z);
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

        // Use the STATIC start position, not the moving current position
        D3DXVECTOR3 origin((FLOAT)m_startDragPos.x(), (FLOAT)m_startDragPos.y(), (FLOAT)m_startDragPos.z());

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

        // 3. Constrain to Axis using STARTING position
      // This prevents the object from drifting on other axes
        if (m_activeAxis == AXIS_X) {
            newPos.setY(m_startDragPos.y());
            newPos.setZ(m_startDragPos.z());
        }
        else if (m_activeAxis == AXIS_Y) {
            newPos.setX(m_startDragPos.x());
            newPos.setZ(m_startDragPos.z());
        }
        else if (m_activeAxis == AXIS_Z) {
            newPos.setX(m_startDragPos.x());
            newPos.setY(m_startDragPos.y());
        }

        // 4. APPLY SNAPPING
        if (m_snapEnabled)
        {
            // Helper Lambda for rounding
            auto SnapVal = [&](float val) -> float {
                return floor(val / m_snapSize + 0.5f) * m_snapSize;
                };

            if (m_activeAxis == AXIS_X) newPos.setX(SnapVal((FLOAT)newPos.x()));
            if (m_activeAxis == AXIS_Y) newPos.setY(SnapVal((FLOAT)newPos.y()));
            if (m_activeAxis == AXIS_Z) newPos.setZ(SnapVal((FLOAT)newPos.z()));
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
        D3DXMatrixRotationY(&r, D3DX_PI / 2.0f); // Rotate cylinder to point X
        D3DXMatrixTranslation(&t, (FLOAT)pos.x() + m_gizmoSize*0.5f, (FLOAT)pos.y(), (FLOAT)pos.z()); // Offset so it starts at center
        world = r * t;
        device->SetTransform(D3DTS_WORLD, &world);
        // Highlight if selected
        D3DCOLOR color = (m_hoverAxis == AXIS_X || m_activeAxis == AXIS_X) ? 0xFFFFFF00 : 0xFFFF0000;
        DrawGizmoArrow(device, color);

        // --- Draw Y Axis (Green) ---
            // Cylinder is Z-aligned. Rotate -90 deg around X to point it along Y.
        D3DXMatrixRotationX(&r, -D3DX_PI / 2.0f);
        D3DXMatrixTranslation(&t, (FLOAT)pos.x(), (FLOAT)pos.y() + m_gizmoSize * 0.5f, (FLOAT)pos.z());
        world = r * t;
        device->SetTransform(D3DTS_WORLD, &world);
        D3DCOLOR yColor = (m_hoverAxis == AXIS_Y || m_activeAxis == AXIS_Y) ? 0xFFFFFF00 : 0xFF00FF00;
        DrawGizmoArrow(device, yColor);

        // --- Draw Z Axis (Blue) ---
            // Cylinder is ALREADY Z-aligned. No rotation needed, just translation.
        D3DXMatrixIdentity(&r); // No rotation needed
        D3DXMatrixTranslation(&t, (FLOAT)pos.x(), (FLOAT)pos.y(), (FLOAT)pos.z() + m_gizmoSize * 0.5f);
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
    float RayAxisDist(D3DXVECTOR3 rO, D3DXVECTOR3 rD, D3DXVECTOR3 axisO, D3DXVECTOR3 axisD)
    {
        D3DXVECTOR3 w0 = rO - axisO;
        float a = D3DXVec3Dot(&rD, &rD);       // always 1.0 if normalized
        float b = D3DXVec3Dot(&rD, &axisD);
        float c = D3DXVec3Dot(&axisD, &axisD); // always 1.0 if normalized
        float d = D3DXVec3Dot(&rD, &w0);
        float e = D3DXVec3Dot(&axisD, &w0);

        float det = a * c - b * b;
        float sc, tc;

        // Parallel check
        if (det < 0.000001f)
        {
            sc = 0.0f;
            tc = (b > c ? d / b : e / c); // fallback
        }
        else
        {
            sc = (b * e - c * d) / det;
            tc = (a * e - b * d) / det;
        }

        // 'tc' is the distance along the Axis.
        // We MUST clamp 'tc' to the physical length of the arrow (0 to gizmoSize)
        // BEFORE calculating the distance.
        float tClamped = tc;
        if (tClamped < 0.0f) tClamped = 0.0f;
        if (tClamped > m_gizmoSize) tClamped = m_gizmoSize;

        // Calculate points
        D3DXVECTOR3 P_ray = rO + rD * sc;          // Point on Ray
        D3DXVECTOR3 P_axis = axisO + axisD * tClamped; // Point on Axis Segment

        // Return distance between the Ray point and the *Clamped* Axis point
        return D3DXVec3Length(&D3DXVECTOR3(P_ray - P_axis));
    }
    D3DXVECTOR3 GetPointOnAxis(D3DXVECTOR3 rO, D3DXVECTOR3 rD, D3DXVECTOR3 axisO, D3DXVECTOR3 axisD)
    {
        // 1. Compute a Plane Normal that contains the Axis and faces the Ray
        // A plane containing the Axis has a normal perpendicular to the Axis.
        // To make it face the camera, we cross the Axis with the result of (Ray x Axis).

        D3DXVECTOR3 planeTangent;
        D3DXVec3Cross(&planeTangent, &rD, &axisD);

        // If Ray and Axis are parallel, this length is 0. Handle fallback.
        if (D3DXVec3LengthSq(&planeTangent) < 0.001f) {
            return axisO;
        }

        D3DXVECTOR3 planeNormal;
        D3DXVec3Cross(&planeNormal, &axisD, &planeTangent);
        D3DXVec3Normalize(&planeNormal, &planeNormal);

        // 2. Intersect Ray with Plane
        // Ray: P = rO + t * rD
        // Plane: (P - axisO) . planeNormal = 0
        // t = ((axisO - rO) . planeNormal) / (rD . planeNormal)

        float denom = D3DXVec3Dot(&rD, &planeNormal);

        // Avoid division by zero (grazing angle)
        if (fabs(denom) < 0.0001f) return axisO;

        D3DXVECTOR3 diff = axisO - rO;
        float t = D3DXVec3Dot(&diff, &planeNormal) / denom;

        // 3. Calculate Hit Point
        D3DXVECTOR3 hitPoint = rO + rD * t;

        // 4. Project Hit Point onto Axis (Clean up floating point noise)
        // We strictly want a point ON the axis line.
        D3DXVECTOR3 hitVector = hitPoint - axisO;
        float scalarOnAxis = D3DXVec3Dot(&hitVector, &axisD);

        return axisO + axisD * scalarOnAxis;
    }

};

