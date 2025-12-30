#pragma once
#include "btBulletCollisionCommon.h"
#include <btBulletDynamicsCommon.h>
#include "D3DRender.h"
#include "tweeny.h"
using tweeny::easing;


// Bullet uses a Right-Handed coordinate system (Z comes toward you).
// default DirectX 9 is Left-Handed (Z+ goes into the screen).

class CQuatCamera
{
private:
    btVector3 m_position;
    btQuaternion m_rotation; // The master rotation state
    float m_accumulatedPitch;

    // --- SHAKE VARIABLES ---
    float     m_shakeDuration;     // Current time left to shake
    float     m_shakeStrength;     // Intensity of the shake
    float     m_startShakeDuration;// To calculate fade-out ratio
    btVector3 m_shakeOffset;       // The temporary offset applied this frame
    tweeny::tween<float> m_shakeTween;
public:
    float fovY;
    float aspectRatio;
    float zNear;
    float zFar ;

public:
    CQuatCamera();

    // Trigger the shake
    void Shake(float strength, float duration);
    // --- Movement & Rotation ---
    // Move relative to the camera's current view (Forward/Back/Strafing)
    void MoveLocal(float distForward, float distRight, float distUp);
    void MoveLocal(btVector3 dist);

    // Rotate around the camera's LOCAL axes (Pitch/Yaw/Roll)
    // Use this for airplanes or space flight.
    void RotateLocal(float pitch, float yaw, float roll);

    // Rotate around the WORLD Y-Axis (and local X). 
    // Use this for FPS style (keeping the horizon level).
    void RotateFPS(float pitch, float globalYaw);

    // --- Utilities ---
    void LookAt(const btVector3& target);
    void SlerpTo(const btQuaternion& targetRot, float t); // Smoothly interpolate to new rotation
    void SetAspectRatio(float width, float height);

    // --- Output for DirectX 9 ---
    D3DXMATRIXA16 GetViewMatrix() const;

    // --- Getters ---
    btVector3 GetPosition() const { return m_position; }
    btQuaternion GetRotation() const { return m_rotation; }

    void Render(IDirect3DDevice9* device);
    void Update(double deltaTime);

    // Get the vector pointing strictly to the "Right" of the camera (Local +X)
    btVector3 CQuatCamera::GetRightVector() const
    {
        // Method 1: Matrix Column Extraction (Cleanest for all axes)
        btMatrix3x3 rotMat(m_rotation);
        return rotMat.getColumn(0);
    }

    // Get the vector pointing strictly "Up" relative to the camera (Local +Y)
    // Note: In 6DOF mode, this might not point to World Up (0,1,0).
    btVector3 CQuatCamera::GetUpVector() const
    {
        btMatrix3x3 rotMat(m_rotation);
        return rotMat.getColumn(1);
    }

    // Get the vector the camera is looking at (Local -Z)
    btVector3 CQuatCamera::GetForwardVector() const
    {
        btMatrix3x3 rotMat(m_rotation);
        // In Right-Handed coordinates, the 'Z' column (column 2) points BACKWARDS.
        // We must negate it to get the "Look" direction.
        return rotMat.getColumn(2) * -1.0f;
    }
};

