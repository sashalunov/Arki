
#include "stdafx.h"
#include "CEnemyBullet.h"

CEnemyBullet::CEnemyBullet(btDiscreteDynamicsWorld* world, D3DXVECTOR3 startPos, D3DXVECTOR3 targetPos, EProjectileType type)
    : CBullet(world, startPos, TYPE_ENEMY_BULLET)
{
    m_projtype = type;
    m_lifeTime = 5.0f;      // Default: destroy after 5 seconds
    m_stateTimer = 0.0f;

    // 1. Convert D3D Vectors to Bullet Vectors
    btVector3 start(startPos.x, startPos.y, startPos.z);
    btVector3 target(targetPos.x, targetPos.y, targetPos.z);

    // Store target for Homing missiles
    m_targetPos = target;
	m_lastOffset = btVector3(0, 0, 0);

    // 2. Calculate Initial Direction
    //btVector3 dir = target - start;
    //if (dir.length() > 0) dir.normalize();
    btVector3 dir = btVector3(0, -1, 0); // Fail-safe: shoot down

    // 3. Setup Speed based on Type
    switch (m_type) {
    case PROJ_ACCELERATING: m_speed = 5.0f;  break; // Start Slow
    case PROJ_HOMING:       m_speed = 15.0f; break; // Moderate
    case PROJ_WOBBLE:       m_speed = 10.0f; break;
    default:                m_speed = 20.0f; break; // Linear is fast
    }

    m_velocity = dir * m_speed;
}

void CEnemyBullet::Update(double dt)
{
    if (m_markForDelete) return;

    m_lifeTime -= (FLOAT)dt;
    m_stateTimer += (FLOAT)dt;

    if (m_lifeTime <= 0.0f) {
        m_markForDelete = true;
        return;
    }
    // -------------------------------------------------------------
    // STEP 1: RECOVER THE "BASE" POSITION
    // -------------------------------------------------------------
    // The physics body is currently at (Base + Wobble). 
    // We subtract the wobble from the last frame to get the true "Center Line" position.
    btTransform trans = m_pBody->getWorldTransform();
    btVector3 currentPos = trans.getOrigin();

    // "cleanPos" is where the bullet would be if it wasn't wobbling
    btVector3 cleanPos = currentPos - m_lastOffset;
    // --- BEHAVIOR LOGIC ---
    switch (m_type)
    {
    case PROJ_ACCELERATING:
    {
        // Increase speed significantly over time
        float acceleration = 25.0f;
        m_speed += acceleration * (FLOAT)dt;

        // Re-normalize velocity to apply new speed
        btVector3 dir = m_velocity.normalized();
        m_velocity = dir * m_speed;
    }
    break;

    case PROJ_HOMING:
    {
        // Steering logic...
        //btVector3 target(m_pPlayerTarget->x, m_pPlayerTarget->y, m_pPlayerTarget->z);
        btVector3 desired = (m_targetPos - currentPos).normalized();
        m_velocity = m_velocity.normalized().lerp(desired, 3.0f * dt).normalized() * m_speed;
    
    }
    break;
    case PROJ_WOBBLE_HOMING: // Both types use steering
    {
        //btVector3 target(currentPlayerPos.x, currentPlayerPos.y, currentPlayerPos.z);
        btVector3 desiredDir = m_targetPos - cleanPos; // Use cleanPos for better accuracy
        desiredDir.normalize();

        // Steer towards target
        btVector3 currentDir = m_velocity.normalized();
        // Wobble Homing is slightly lazier (2.0f) to emphasize the snake movement
        float turnRate = (m_type == PROJ_WOBBLE_HOMING) ? 2.0f : 4.0f;

        btVector3 newDir = currentDir.lerp(desiredDir, turnRate * dt);
        newDir.normalize();
        m_velocity = newDir * m_speed;
    }
    break;

    case PROJ_WOBBLE:
    {
        // Add a perpendicular force to the straight line
        // This requires modifying position directly rather than just velocity
        // But for simplicity, we keep linear velocity and add offset in movement step below
        // (See step 5)
    }
    break;

    case PROJ_LINEAR:
    default:
        // Velocity remains constant
        break;
    }
    // -------------------------------------------------------------
    // STEP 3: MOVE "BASE" POSITION FORWARD
    // -------------------------------------------------------------
    cleanPos += m_velocity * dt;
    // -------------------------------------------------------------
    // STEP 4: CALCULATE NEW WOBBLE OFFSET
    // -------------------------------------------------------------
    btVector3 newOffset(0, 0, 0);
    if (m_type == PROJ_WOBBLE || m_type == PROJ_WOBBLE_HOMING)
    {
        // A. Parameters
        float frequency = 10.0f; // How fast it snakes
        float amplitude = 1.5f;  // How wide it snakes

        // B. Calculate "Right" Vector
        // We need a direction perpendicular to velocity to apply the offset.
        // Cross Product of Forward (Velocity) and Up (0,1,0) gives Right.
        btVector3 forward = m_velocity.normalized();
        btVector3 up(0, 1, 0);

        // Safety: If looking straight up/down, cross product fails. Use X-axis instead.
        if (fabs(forward.y()) > 0.95f) up = btVector3(1, 0, 0);

        btVector3 right = forward.cross(up);
        right.normalize();

        // C. Calculate Sine Wave
        float wave = sin(m_stateTimer * frequency) * amplitude;

        // D. Create the Offset Vector
        newOffset = right * wave;
    }

    // Store for next frame
    m_lastOffset = newOffset;

    // -------------------------------------------------------------
    // STEP 5: APPLY FINAL POSITION
    // -------------------------------------------------------------
    btVector3 finalPos = cleanPos + newOffset;

    trans.setOrigin(finalPos);
    m_pBody->getMotionState()->setWorldTransform(trans);
    m_pBody->setWorldTransform(trans);
}

void CEnemyBullet::Render(IDirect3DDevice9* device)
{
	if (m_markForDelete) return;
    btTransform trans = m_pBody->getWorldTransform();
    btVector3 pos = trans.getOrigin();

	D3DXMATRIXA16 matTrans, matScale;
	// Scale down the sphere mesh to be bullet sized
	D3DXMatrixScaling(&matScale, 0.4f, 0.4f, 0.4f);
    D3DXMatrixTranslation(&matTrans, (FLOAT)pos.getX(), (FLOAT)pos.getY(), (FLOAT)pos.getZ());
	matTrans = matScale * matTrans;
	device->SetTransform(D3DTS_WORLD, &matTrans);

    //m_pMesh->SetPosition(D3DXVECTOR3(pos.x(), pos.y(), pos.z()));

    // --- NEAT FEATURE: AUTO-ROTATION ---
    // Rotate the mesh to face the direction it is traveling.
    // Calculate angle from Velocity using atan2
    // Note: atan2(y, x) gives angle in radians.

    //float angleZ = (FLOAT)atan2(m_velocity.y(), m_velocity.x());

    // Depending on your mesh, you might need to add/subtract 90 degrees (1.57 rads)
    // assuming your mesh points "Up" by default.
    //angleZ -= 1.57f;

    // Apply rotation (Roll/Pitch/Yaw)
    //m_pMesh->SetRotation(D3DXVECTOR3(0, 0, angleZ));
    //D3DXA
	//D3DXComputeNormals(CRigidBody::s_pRigidBodySphereMesh, NULL);
    D3DMATERIAL9 mat;
	InitMaterialS(mat, 1.0f, 1.0f, 1.0f, 1.0f, 64.0f); // Red Bullet
	mat.Emissive = D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f); // Slight glow
	device->SetMaterial(&mat);
	device->SetTexture(0, NULL);
	CRigidBody::s_pRigidBodySphereMesh->DrawSubset(0);
    //m_pMesh->Render();
}