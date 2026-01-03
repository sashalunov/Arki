
#include "stdafx.h"
#include "CEnemyBullet.h"

CEnemyBullet::CEnemyBullet(btDiscreteDynamicsWorld* world, CXMesh* mesh, D3DXVECTOR3 startPos, D3DXVECTOR3 targetPos, EProjectileType type)
{
    m_pWorld = world;
    m_pMesh = mesh;
    m_type = type;
    m_toBeDeleted = false;
    m_lifeTime = 5.0f;      // Default: destroy after 5 seconds
    m_stateTimer = 0.0f;

    // 1. Convert D3D Vectors to Bullet Vectors
    btVector3 start(startPos.x, startPos.y, startPos.z);
    btVector3 target(targetPos.x, targetPos.y, targetPos.z);

    // Store target for Homing missiles
    m_targetPos = target;

    // 2. Calculate Initial Direction
    btVector3 dir = target - start;
    if (dir.length() > 0) dir.normalize();
    else dir = btVector3(0, -1, 0); // Fail-safe: shoot down

    // 3. Setup Speed based on Type
    switch (m_type) {
    case PROJ_ACCELERATING: m_speed = 5.0f;  break; // Start Slow
    case PROJ_HOMING:       m_speed = 12.0f; break; // Moderate
    case PROJ_WOBBLE:       m_speed = 10.0f; break;
    default:                m_speed = 18.0f; break; // Linear is fast
    }

    m_velocity = dir * m_speed;

    // 4. Setup Physics Body (Kinematic Sphere)
    btCollisionShape* shape = new btSphereShape(0.2f); // Tiny hitbox
    btTransform t;
    t.setIdentity();
    t.setOrigin(start);

    btDefaultMotionState* ms = new btDefaultMotionState(t);
    m_pBody = new btRigidBody(0, ms, shape);

    // KINEMATIC: We move it via code, Physics detects collisions
    m_pBody->setCollisionFlags(m_pBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_pBody->setActivationState(DISABLE_DEACTIVATION);
    m_pBody->setUserPointer(this); // Important for Collision Callbacks

    m_pWorld->addRigidBody(m_pBody);
}

CEnemyBullet::~CEnemyBullet()
{
    if (m_pBody) {
        m_pWorld->removeRigidBody(m_pBody);
        delete m_pBody->getMotionState();
        delete m_pBody->getCollisionShape(); // Clean up shape here or in manager depending on design
        delete m_pBody;
    }
}

void CEnemyBullet::Update(float dt, D3DXVECTOR3 currentPlayerPos)
{
    m_lifeTime -= dt;
    m_stateTimer += dt;

    if (m_lifeTime <= 0.0f) {
        m_toBeDeleted = true;
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
        m_speed += acceleration * dt;

        // Re-normalize velocity to apply new speed
        btVector3 dir = m_velocity.normalized();
        m_velocity = dir * m_speed;
    }
    break;

    case PROJ_HOMING:
    case PROJ_WOBBLE_HOMING: // Both types use steering
    {
        btVector3 target(currentPlayerPos.x, currentPlayerPos.y, currentPlayerPos.z);
        btVector3 desiredDir = target - cleanPos; // Use cleanPos for better accuracy
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
    btTransform trans = m_pBody->getWorldTransform();
    btVector3 pos = trans.getOrigin();

    //m_pMesh->SetPosition(D3DXVECTOR3(pos.x(), pos.y(), pos.z()));

    // --- NEAT FEATURE: AUTO-ROTATION ---
    // Rotate the mesh to face the direction it is traveling.
    // Calculate angle from Velocity using atan2
    // Note: atan2(y, x) gives angle in radians.

    float angleZ = (FLOAT)atan2(m_velocity.y(), m_velocity.x());

    // Depending on your mesh, you might need to add/subtract 90 degrees (1.57 rads)
    // assuming your mesh points "Up" by default.
    angleZ -= 1.57f;

    // Apply rotation (Roll/Pitch/Yaw)
    //m_pMesh->SetRotation(D3DXVECTOR3(0, 0, angleZ));

    m_pMesh->Render();
}