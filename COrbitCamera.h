#pragma once
#include <btBulletDynamicsCommon.h>
#include "D3DRender.h"


// COrbitCamera: A distinct class for Editor/Model-Viewer navigation.
// Behavior mimics Blender: 
// - Always looks at a specific 'Target' point.
// - 'Orbit' rotates the camera AROUND that target.
// - 'Pan' moves the target (and camera) purely in the view plane.

class COrbitCamera
{
private:
    // Core State
    btVector3 m_target;       // The pivot point we are looking at
    float m_distance;         // Distance from Target to Camera

    // Orientation State
    // We store Euler angles explicitly for the Orbit camera to strictly 
    // clamp pitch (prevent flipping upside down) which is standard for editors.
    float m_yaw;              // Rotation around World Y (in Radians)
    float m_pitch;            // Rotation up/down (in Radians)

    // Projection State
    float m_fovY;
    float m_aspectRatio;
    float m_zNear;
    float m_zFar;

public:
    COrbitCamera();

    // --- Blender-Style Controls ---

    // [MMB] Rotate around the target
    // yawDelta: Mouse X movement
    // pitchDelta: Mouse Y movement
    void Orbit(float yawDelta, float pitchDelta);

    // [Shift + MMB] Move the camera AND the target together
    void Pan(float dx, float dy);

    // [Mouse Wheel] Move closer/further
    void Zoom(float amount);

    // Focus controls (e.g., Double click on object)
    void SetTarget(const btVector3& target);
    void SetDistance(float dist);

    // --- Output & Rendering ---

    // Reconstructs position based on Target + Distance + Rotation
    btVector3 GetPosition() const;

    // Calculates the View Matrix for D3D9
    D3DXMATRIXA16 GetViewMatrix() const;
	D3DXMATRIXA16 GetProjectionMatrix() const;

    // Helper to apply transforms to device
    void Render(IDirect3DDevice9* device, double deltaTime);

    // --- Setters ---
    void SetProjection(float fov, float zNear, float zFar) {
        m_fovY = fov; m_zNear = zNear; m_zFar = zFar;
    }

    void SetAspectRatio(float width, float height)
    {
        if (height > 0.0f) // Prevent divide by zero
            m_aspectRatio = width / height;
    }
};