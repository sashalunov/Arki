
#include "StdAfx.h"
#include "COrbitCamera.h"

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
COrbitCamera::COrbitCamera()
{
    m_target = btVector3(0, 0, 0);
    m_distance = 10.0f;
    m_yaw = 0.0f;
    m_pitch = 0.5f; // Start looking slightly down at the object

    m_fovY = D3DX_PI / 4.0f;
    m_aspectRatio = 1.0f;
    m_zNear = 0.1f;
    m_zFar = 1000.0f;
}

// ----------------------------------------------------------------------
// Controls
// ----------------------------------------------------------------------

void COrbitCamera::Orbit(float yawDelta, float pitchDelta)
{
    // 1. Update Angles
    m_yaw += yawDelta;
    m_pitch += pitchDelta;

    // 2. Clamp Pitch to prevent "flipping" over the top (Gimbal Lock protection)
    // 89 degrees is approx 1.55 radians. 
    // Blender "Turntable" style usually stops you from going completely upside down.
    const float PITCH_LIMIT = 1.55f;

    if (m_pitch > PITCH_LIMIT) m_pitch = PITCH_LIMIT;
    if (m_pitch < -PITCH_LIMIT) m_pitch = -PITCH_LIMIT;

    // Yaw can wrap around freely (0 to 360)
}

void COrbitCamera::Zoom(float amount)
{
    m_distance -= amount;

    // Prevent zooming through the target or becoming inverted
    if (m_distance < 0.1f) m_distance = 0.1f;
}

void COrbitCamera::Pan(float dx, float dy)
{
    // Logic: We need to move the Target (and implicitly the camera) 
    // along the camera's local "Right" and "Up" vectors.

    // 1. Calculate the rotation matrix to get Right/Up vectors
    btQuaternion rotation;
    // Note: Bullet Euler is usually (Yaw, Pitch, Roll). Order matters.
    // For an orbit cam, we usually rotate Yaw around Y, then Pitch around local X.
    btQuaternion qYaw(btVector3(0, 1, 0), m_yaw);
    btQuaternion qPitch(btVector3(1, 0, 0), m_pitch);
    rotation = qYaw * qPitch;

    btMatrix3x3 rotMat(rotation);
    btVector3 right = rotMat.getColumn(0);
    btVector3 up = rotMat.getColumn(1); // Camera's Local Up

    // 2. Apply Pan to Target
    // Note: 'dx' usually maps to Right, 'dy' maps to Up.
    // Speed factor depends on distance (panning is faster when zoomed out)
    float panSpeed = m_distance * 0.001f;

    // We subtract because dragging mouse left usually pulls the world right
    m_target -= (right * dx * panSpeed);
    m_target += (up * dy * panSpeed);
}

void COrbitCamera::SetTarget(const btVector3& target)
{
    m_target = target;
}

void COrbitCamera::SetDistance(float dist)
{
    if (dist > 0.1f) m_distance = dist;
}

// ----------------------------------------------------------------------
// Physics / Math Calculations
// ----------------------------------------------------------------------

btVector3 COrbitCamera::GetPosition() const
{
    // Convert spherical coordinates (Yaw/Pitch/Dist) to Cartesian
    // But since we have Quaternions available, it's easier to rotate a vector.

    // 1. Create Rotation Quaternion
    btQuaternion qYaw(btVector3(0, 1, 0), m_yaw);
    btQuaternion qPitch(btVector3(1, 0, 0), m_pitch);
    btQuaternion rotation = qYaw * qPitch;

    // 2. Calculate Offset
    // In our view space (RHS), the camera is located +Distance along the +Z axis 
    // relative to the target (because it looks down -Z).
    btVector3 offset(0, 0, m_distance);

    // 3. Rotate the offset into World Space
    btVector3 worldOffset = quatRotate(rotation, offset);

    // 4. Final Position
    return m_target + worldOffset;
}

D3DXMATRIXA16 COrbitCamera::GetViewMatrix() const
{
    // 1. Calculate Rotation
    btQuaternion qYaw(btVector3(0, 1, 0), m_yaw);
    btQuaternion qPitch(btVector3(1, 0, 0), m_pitch);
    btQuaternion rot = qYaw * qPitch;

    // 2. Get Camera Position
    btVector3 pos = GetPosition();

    // 3. Build View Matrix manually
    // View = Inverse(World)
    // Rotation part is Transpose of rotMatrix
    // Translation part is -dot(axis, pos)

    btMatrix3x3 rotMat(rot);
    btVector3 right = rotMat.getColumn(0);
    btVector3 up = rotMat.getColumn(1);
    btVector3 fwd = rotMat.getColumn(2) * -1.0f; // Forward is -Z

    D3DXMATRIXA16 view;

    view._11 = (FLOAT)right.x(); view._12 = (FLOAT)up.x(); view._13 = -(FLOAT)fwd.x(); view._14 = 0.0f;
    view._21 = (FLOAT)right.y(); view._22 = (FLOAT)up.y(); view._23 = -(FLOAT)fwd.y(); view._24 = 0.0f;
    view._31 = (FLOAT)right.z(); view._32 = (FLOAT)up.z(); view._33 = -(FLOAT)fwd.z(); view._34 = 0.0f;

    view._41 = -(FLOAT)pos.dot(right);
    view._42 = -(FLOAT)pos.dot(up);
    view._43 = (FLOAT)pos.dot(fwd); // Note sign change for RH Z-axis
    view._44 = 1.0f;

    return view;
}

void COrbitCamera::Render(IDirect3DDevice9* device, float dt)
{
    //m_aspectRatio = aspectRatio;
    SetAspectRatio((float)d3d9->GetWidth(), (float)d3d9->GetHeight());

    D3DXMATRIXA16 view = GetViewMatrix();
    device->SetTransform(D3DTS_VIEW, &view);

    D3DXMATRIXA16 proj;
    D3DXMatrixPerspectiveFovRH(&proj, m_fovY, m_aspectRatio, m_zNear, m_zFar);
    device->SetTransform(D3DTS_PROJECTION, &proj);
}