#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "CFlyingEnemy.h"

#include "MovementStrategy.h"


// ========================================================
// STRATEGY 1: SINE WAVE (ATTACK)
// ========================================================
void CSineMove::OnEnter(CFlyingEnemy* enemy)
{
    m_totalTime = 0.0f;
    enemy->m_shootTimer = 1.0f; // Reset gun
}

bool CSineMove::Update(CFlyingEnemy* enemy, double dt)
{
    m_totalTime += (FLOAT)dt;

    // 1. Calculate Sine Movement
    float speedY = 3.5f; // Descent speed
    float sweepSpeed = 2.0f;
    float sweepWidth = 8.0f;

    float nextY = enemy->m_startPos.y - (m_totalTime * speedY);
    float nextX = enemy->m_startPos.x + sin(m_totalTime * sweepSpeed) * sweepWidth;

    enemy->SetPosition(nextX, nextY);

    // 2. Shoot Logic (Only while attacking)
    enemy->m_shootTimer -= (FLOAT)dt;
    if (enemy->m_shootTimer <= 0.0f) {
        enemy->Shoot();
        enemy->m_shootTimer = 2.0f + (rand() % 100) / 100.0f;
    }

    // 3. Trigger Condition: Did we hit the bottom?
    if (nextY < -5.0f) {
        return true; // "I am done, switch me"
    }
    return false;
}
// ========================================================
// STRATEGY: STEERING SINE (Organic "Drifty" Wave)
// ========================================================
void CSineMove2::OnEnter(CFlyingEnemy* enemy)
{
    m_totalTime = 0.0f;
    // Start with some downward momentum so it doesn't have to accelerate from 0
    m_velocity = D3DXVECTOR3(0, -5.0f, 0);
    enemy->m_shootTimer = 1.0f;
}

bool CSineMove2::Update(CFlyingEnemy* enemy, double dt)
{
    m_totalTime += (FLOAT)dt;

    // --- CONFIGURATION ---
    float maxSpeed = 155.0f;      // Maximum flight speed
    float maxForce = 20.0f;      // Turn Rate (Lower = Drifty/Icy, Higher = Snappy)
    float lookAhead = 0.2f;      // How far into the future we aim (0.1s to 0.5s)

    // Sine Wave Params
    float descentSpeed = 1.0f;
    float sweepSpeed = 1.0f;
    float sweepWidth = 8.0f;

    // 1. Calculate the "Ghost" Target
    // This is the ideal position on the Sine Wave curve in the near future.
    float futureTime = m_totalTime + lookAhead;

    float targetY = enemy->m_startPos.y - (futureTime * descentSpeed);
    float targetX = enemy->m_startPos.x + sin(futureTime * sweepSpeed) * sweepWidth;

    D3DXVECTOR3 targetPos(targetX, targetY, enemy->m_startPos.z);

    // 2. STEERING LOGIC (Seek Behavior)

    // A. Get Desired Velocity (Vector to Target)
    D3DXVECTOR3 currentPos = enemy->m_currentPos;
    D3DXVECTOR3 desiredVel = targetPos - currentPos;

    // B. Check distance (Optional optimization)
    // float dist = D3DXVec3Length(&desiredVel);

    // C. Scale to Max Speed
    D3DXVec3Normalize(&desiredVel, &desiredVel);
    desiredVel *= maxSpeed;

    // D. Calculate Steering Force (Desired - Current)
    D3DXVECTOR3 steering = desiredVel - m_velocity;

    // E. Clamp Force (Preserves momentum/drift)
    float forceLen = D3DXVec3Length(&steering);
    if (forceLen > maxForce) {
        D3DXVec3Normalize(&steering, &steering);
        steering *= maxForce;
    }

    // 3. APPLY PHYSICS
    // Velocity += Acceleration * dt
    m_velocity += steering * (FLOAT)dt;

    // Position += Velocity * dt
    D3DXVECTOR3 newPos = currentPos + m_velocity * (FLOAT)dt;

    enemy->SetPosition(newPos.x, newPos.y);

    // 4. VISUALS: Bank into the turn
    // Since this is physics-based, the velocity tells us exactly how to bank.
    // X velocity determines roll.
    //float rollAngle = -m_velocity.x * 0.08f;
    //enemy->m_pMesh->SetRotation(D3DXVECTOR3(0, 0, rollAngle));

    // 5. SHOOTING
    enemy->m_shootTimer -= (FLOAT)dt;
    if (enemy->m_shootTimer <= 0.0f) {
        //enemy->Shoot(); // You can pass player pos here if you added aiming
        enemy->m_shootTimer = 1.5f + (rand() % 100) / 100.0f;
    }

    // 6. FINISH CONDITION
    return (newPos.y < -5.0f);
}

// ========================================================
// STRATEGY 2: RETURN (RETREAT)
// ========================================================
void CReturnMove::OnEnter(CFlyingEnemy* enemy)
{
    m_t = 0.0f;
    m_p0 = enemy->m_currentPos; // Current Location
    m_p2 = enemy->m_startPos;   // Home Location

    // --- CALCULATE CONTROL POINT (P1) ---
    // We want the enemy to "arc" outwards before coming back.
    // If we are to the LEFT of home, arc further LEFT.
    // If we are to the RIGHT of home, arc further RIGHT.

    D3DXVECTOR3 midPoint = (m_p0 + m_p2) * 0.5f;

    float arcWidth = 12.0f; // How "wide" the curve is

    // If we are to the left, curve further left. If right, curve right.
    float dir = (m_p0.x < m_p2.x) ? -1.0f : 1.0f;

    // IMPORTANT: PRESERVE THE Z AXIS!
    // If you leave Z as 0.0f, the enemy will fly "into the screen" and get clipped by the camera.
    m_p1 = D3DXVECTOR3(midPoint.x + (arcWidth * dir), midPoint.y, enemy->m_startPos.z);
}

bool CReturnMove::Update(CFlyingEnemy* enemy, double dt)
{
    // 1. Advance Timer
       // Speed determines how fast 't' goes from 0 to 1.
       // Distance / Speed approximation or just fixed time.
    float flightDuration = 3.0f; // Takes 2 seconds to fly home
    m_t += (FLOAT)dt / flightDuration;

    // 2. Check Arrival
    if (m_t >= 1.0f) {
        enemy->SetPosition(m_p2.x, m_p2.y); // Snap to exact home
        return true; // Done!
    }

    // 3. Bezier Math
    // Formula: (1-t)^2 * P0  +  2(1-t)t * P1  +  t^2 * P2
    float u = 1.0f - m_t; // "u" is the remaining time
    float tt = m_t * m_t;
    float uu = u * u;

    D3DXVECTOR3 p = (uu * m_p0) + (2.0f * u * m_t * m_p1) + (tt * m_p2);

    enemy->SetPosition(p.x, p.y);

    // Optional: Bank the ship visually based on the curve?
    // You can calculate the derivative of the curve to get the facing angle.

    return false;
}

// ========================================================
// STRATEGY 6: STEERING RETURN (Physics-based Arrival)
// ========================================================
void CReturnMove2::OnEnter(CFlyingEnemy* enemy)
{
    // 1. Inherit momentum? 
    // Ideally, you'd calculate this based on the previous frame's movement.
    // For now, we assume we just finished a dive, so we have downward momentum.
    m_velocity = D3DXVECTOR3(0, -5.0f, 0);
}

bool CReturnMove2::Update(CFlyingEnemy* enemy, double dt)
{
    // CONFIGURATION
    float maxSpeed = 20.0f;       // How fast it flies up
    float maxForce = 15.0f;       // "Turning Radius" (Lower = wider turns, Higher = snappier)
    float slowingRadius = 8.0f;   // Distance to start braking
    float stopTolerance = 0.21f;   // How close is "Close enough"?

    // 1. Get Vector to Target
    D3DXVECTOR3 targetPos = enemy->m_startPos;
    D3DXVECTOR3 currentPos = enemy->m_currentPos;
    D3DXVECTOR3 desiredVel = targetPos - currentPos;

    // 2. Calculate Distance
    float distance = D3DXVec3Length(&desiredVel);

    // 3. ARRIVAL LOGIC (The "Magic")
    if (distance < stopTolerance) {
        // We arrived! Snap to exact pos and finish.
        enemy->SetPosition(targetPos.x, targetPos.y);
        //enemy->m_pMesh->SetRotation(D3DXVECTOR3(0, 0, 0)); // Reset bank
        return true;
    }

    // Normalize desired velocity direction
    D3DXVec3Normalize(&desiredVel, &desiredVel);

    // 4. Calculate Speed (Ramp down if inside slowing radius)
    if (distance < slowingRadius) {
        // Scale speed down: (distance / radius) * maxSpeed
        desiredVel *= maxSpeed * (distance / slowingRadius);
    }
    else {
        // Full speed
        desiredVel *= maxSpeed;
    }

    // 5. Calculate Steering Force
    // Force = Desired - Current
    D3DXVECTOR3 steering = desiredVel - m_velocity;

    // 6. Cap the Steering Force (This creates the "Curve")
    // If we don't cap this, the enemy turns instantly (robotic).
    // By capping it, we force it to loop around if it's going too fast.
    float steeringForce = D3DXVec3Length(&steering);
    if (steeringForce > maxForce) {
        D3DXVec3Normalize(&steering, &steering);
        steering *= maxForce;
    }

    // 7. Apply Physics (Euler Integration)
    // Velocity += Acceleration * dt
    m_velocity += steering * (FLOAT)dt;

    // Position += Velocity * dt
    D3DXVECTOR3 newPos = currentPos + m_velocity * (FLOAT)dt;

    enemy->SetPosition(newPos.x, newPos.y);

    // 8. VISUAL FLAIR: Banking
    // Rotate the ship based on X-velocity to look like it's turning
    // Negative X velocity = Bank Left (Positive Rot), Positive X = Bank Right (Negative Rot)
    //float bankAngle = -m_velocity.x * 0.05f;
    //enemy->m_pMesh->SetRotation(D3DXVECTOR3(0, 0, bankAngle));

    return false;
}
