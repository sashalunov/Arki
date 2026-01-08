#include "StdAfx.h"
#include "CQuatCamera.h"

CQuatCamera::CQuatCamera()
{
    m_position = btVector3(0, 0, 0);
    // Initialize rotation to identity (facing forward, no roll)
    m_rotation = btQuaternion::getIdentity();
    m_accumulatedPitch = 0.0f; // Start looking straight ahead

    // Field of View (FOV): Vertical angle in radians. 
    // D3DX_PI / 4 is 45 degrees (Standard). 
    // D3DX_PI / 2 is 90 degrees (Quake pro).
    fovY = D3DX_PI / 4.0f;
    aspectRatio = 1;// (float)d3d9->GetWidth() / (float)d3d9->GetHeight();
    zNear = 0.01f;
    zFar = 1000.0f;

    m_shakeDuration = 0.0f;
    m_shakeStrength = 0.0f;
    m_shakeOffset = btVector3(0, 0, 0);

    // Initialize tween to a "finished" state (0 to 0) so it doesn't shake at start
    m_shakeTween = tweeny::from(0.0f).to(0.0f).during(0);

}

void CQuatCamera::MoveLocal(float distForward, float distRight, float distUp)
{
    // Convert quaternion to a rotation matrix to get direction vectors
    btMatrix3x3 matRot(m_rotation);

    // Get Local Basis Vectors
    // In Bullet: Column 0 = Right (X), Column 1 = Up (Y), Column 2 = Back (+Z) or Fwd (-Z)
    // Assuming Standard RH: Forward is -Z
    btVector3 vRight = matRot.getColumn(0);
    btVector3 vUp = matRot.getColumn(1);
    btVector3 vFwd = matRot.getColumn(2) * -1.0f;

    btVector3 displacement = (vFwd * distForward) + (vRight * distRight) + (vUp * distUp);
    m_position += displacement;
}
void CQuatCamera::MoveLocal(btVector3 dist)
{
    // Convert quaternion to a rotation matrix to get direction vectors
    btMatrix3x3 matRot(m_rotation);

    // Get Local Basis Vectors
    // In Bullet: Column 0 = Right (X), Column 1 = Up (Y), Column 2 = Back (+Z) or Fwd (-Z)
    // Assuming Standard RH: Forward is -Z
    btVector3 vRight = matRot.getColumn(0);
    btVector3 vUp = matRot.getColumn(1);
    btVector3 vFwd = matRot.getColumn(2) * -1.0f;

    btVector3 displacement = (vFwd * dist.z()) + (vRight * dist.x()) + (vUp * dist.y());
    m_position += displacement;
}

// 6DOF Rotation (Space Ship style)
void CQuatCamera::RotateLocal(float pitch, float yaw, float roll)
{
    // Create rotation quaternions for each axis
    // Note: These axes are (1,0,0), (0,1,0), (0,0,1) because we are multiplying
    // ON THE RIGHT side of m_rotation, which applies it in Local Space.

    btQuaternion qPitch(btVector3(1, 0, 0), pitch);
    btQuaternion qYaw(btVector3(0, 1, 0), yaw);
    btQuaternion qRoll(btVector3(0, 0, 1), roll);

    // Combine them (Order: Yaw -> Pitch -> Roll usually feels best)
    btQuaternion change = qYaw * qPitch * qRoll;

    // Apply to current rotation
    m_rotation = m_rotation * change;

    // Normalize to prevent floating point drift over time
    m_rotation.normalize();
}

// FPS Rotation (Turret style)
void CQuatCamera::RotateFPS(float pitch, float globalYaw)
{
    // Define the limit (89 degrees in radians is roughly 1.55)
    // We use slightly less than 90 (PI/2) to avoid "Gimbal Lock" glitches at the poles.
    const float PITCH_LIMIT = 1.55f;

    // 1. Calculate what the new pitch WOULD be
    float newPitch = m_accumulatedPitch + pitch;

    //// 1. Pitch is LOCAL (Head nods up/down relative to body)
    //// Multiply on the RIGHT
    //btQuaternion qPitch(btVector3(1, 0, 0), pitch);
    //m_rotation = m_rotation * qPitch;

    //// 2. Yaw is GLOBAL (Body spins around world Up)
    //// Multiply on the LEFT
    //btQuaternion qYaw(btVector3(0, 1, 0), globalYaw);
    //m_rotation = qYaw * m_rotation;
    // 2. Clamp the new pitch to the range [-89, 89] degrees
    if (newPitch > PITCH_LIMIT)
    {
        // If we are trying to look too far up, correct the input delta
        // so we land exactly AT the limit, not past it.
        pitch = PITCH_LIMIT - m_accumulatedPitch;
        m_accumulatedPitch = PITCH_LIMIT;
    }
    else if (newPitch < -PITCH_LIMIT)
    {
        // Same for looking down
        pitch = -PITCH_LIMIT - m_accumulatedPitch;
        m_accumulatedPitch = -PITCH_LIMIT;
    }
    else
    {
        // If we are within limits, just update the accumulator normally
        m_accumulatedPitch = newPitch;
    }

    // 3. Apply the CLAMPED Pitch locally (Right side multiply)
    if (fabs(pitch) > 0.0001f) // Optimization: skip if no rotation needed
    {
        btQuaternion qPitch(btVector3(1, 0, 0), pitch);
        m_rotation = m_rotation * qPitch;
    }

    // 4. Apply Yaw globally (Left side multiply)
    // Yaw usually doesn't need clamping
    if (fabs(globalYaw) > 0.0001f)
    {
        btQuaternion qYaw(btVector3(0, 1, 0), globalYaw);
        m_rotation = qYaw * m_rotation;
    }

    m_rotation.normalize();
}

void CQuatCamera::LookAt(const btVector3& target)
{
    btVector3 forward = target - m_position;

    // Handle degenerate case (target is exactly at position)
    if (forward.length2() < 0.0001f) return;

    forward.normalize();

    // 1. Standard LookAt Quat math (same as before)
    btVector3 defaultForward(0, 0, -1);
    m_rotation = shortestArcQuat(defaultForward, forward);
    // Standard Right-Handed Setup:
    // Forward = -Z
    // Up = +Y
    // We want to calculate the rotation that aligns -Z with 'forward'

    // Note: Calculating a quaternion from direction vectors is complex math.
    // Bullet provides the ShortestArcQuat, but it's often easier to use a matrix helper.
    // However, here is the pure vector approach:

    // 1. Calculate the rotation from default Forward (0,0,-1) to new Forward
    //btVector3 defaultForward(0, 0, -1);
    //btQuaternion rotQuat = shortestArcQuat(defaultForward, forward);

   //m_rotation = rotQuat;
    // 2. [NEW] Sync the accumulator!
    // We calculate the pitch from the new Forward vector.
    // In Bullet (Y-up), Pitch is the angle between Forward and the XZ plane.
    // Conveniently, asin(forward.y) gives us exactly that angle (in radians).
    // Note: Since Forward is -Z in this system, we might need to check sign depending on your exact setup,
    // but typically:
    m_accumulatedPitch = asin(-(FLOAT)forward.y());
}

void CQuatCamera::SlerpTo(const btQuaternion& targetRot, float t)
{
    // Smoothly interpolate current rotation towards target
    m_rotation = m_rotation.slerp(targetRot, t);
}

D3DXMATRIXA16 CQuatCamera::GetViewMatrix() const
{
    // 1. Convert Bullet Quaternion to Matrix
    btMatrix3x3 rotMat(m_rotation);
    // 2. Extract Basis Vectors
    btVector3 right = rotMat.getColumn(0);
    btVector3 up = rotMat.getColumn(1);
    btVector3 fwd = rotMat.getColumn(2) * -1.0f; // RH Forward is -Z

    // Apply the Shake Offset here
    btVector3 finalPos = m_position + m_shakeOffset;
    // 3. Build View Matrix manually 
    // The View Matrix is the Inverse of the World Matrix.
    // Since it's a rotation+translation, Inverse = Transpose(Rotation) * Translation(-Pos)
    D3DXMATRIXA16 view;

    // Row 1: Right Vector & Dot Product
    view._11 = (FLOAT)right.x(); view._12 = (FLOAT)up.x(); view._13 = -(FLOAT)fwd.x(); view._14 = 0.0f;
    // Row 2: Up Vector & Dot Product
    view._21 = (FLOAT)right.y(); view._22 = (FLOAT)up.y(); view._23 = -(FLOAT)fwd.y(); view._24 = 0.0f;
    // Row 3: Forward Vector & Dot Product (Note the -fwd for Z axis inversion in LookAt logic)
    view._31 = (FLOAT)right.z(); view._32 = (FLOAT)up.z(); view._33 = -(FLOAT)fwd.z(); view._34 = 0.0f;
    // Row 4: Translation (Dot products of Pos and Axis)
    // In a View Matrix, the translation part is: -dot(Pos, Axis)
    view._41 = -(FLOAT)finalPos.dot(right);
    view._42 = -(FLOAT)finalPos.dot(up);
    view._43 = (FLOAT)finalPos.dot(fwd); // Positive here because fwd is -Z
    view._44 = 1.0f;

    return view;
}

void CQuatCamera::Update(double dt)
{
    // 1. Convert dt to milliseconds for tweeny
    int dtMs = (int)(dt * 1000.0f);

    // 2. Step the animation
    // 'step' advances the time and returns the current interpolated value (current strength)
    float currentStrength = m_shakeTween.step(dtMs);

    // 3. Apply Offset if still shaking
    if (m_shakeTween.progress() < 1.0f && currentStrength > 0.001f)
    {
        // Generate random vector (-1.0 to 1.0)
        float rx = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float ry = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float rz = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        // Apply the tweened strength
        m_shakeOffset = btVector3(rx, ry, rz) * currentStrength;
    }
    else
    {
        // Ensure perfect zero when finished
        m_shakeOffset = btVector3(0, 0, 0);
    }
}

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
void CQuatCamera::Render(IDirect3DDevice9* device)
{
    
    SetAspectRatio((float)d3d9->GetWidth() , (float)d3d9->GetHeight());

    // This moves the "World" around you.
    D3DXMATRIXA16 view = GetViewMatrix();
    device->SetTransform(D3DTS_VIEW, &view);
    // Use the RH (Right-Handed) Function -
    // This matches the Bullet Physics coordinate system.
    D3DXMATRIXA16 proj;
    D3DXMatrixPerspectiveFovRH(&proj, fovY, aspectRatio, zNear, zFar);
    device->SetTransform(D3DTS_PROJECTION, &proj);

}
// Helper for finding quaternion between vectors (if not using btVector3::shortestArc)
// Included implicitly in newer Bullet versions, but logic is:
// cross(v1, v2) = axis, dot(v1, v2) = angle info.

void CQuatCamera::SetAspectRatio(float width, float height)
{
    if (height > 0.0f) // Prevent divide by zero
        aspectRatio = width / height;
}

void CQuatCamera::Shake(float strength, float duration)
{
    // tweeny works in milliseconds (usually int), so convert seconds -> ms
    int durationMs = (int)(duration * 1000.0f);

    // Configure the Tween:
    // 1. Start at 'strength'
    // 2. Go to '0.0f'
    // 3. Take 'durationMs' time
    // 4. Use Quadratic Easing for smooth fade-out
    m_shakeTween = tweeny::from(strength)
        .to(0.0f)
        .during(durationMs)
        .via(tweeny::easing::quadraticOut);
}