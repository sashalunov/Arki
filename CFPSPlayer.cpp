#include "stdafx.h"
#include "CFPSPlayer.h"

CFPSPlayer::CFPSPlayer()
{
    m_pBody = nullptr;
    m_pShape = nullptr;
    m_pWorld = nullptr;
    m_pCamera = nullptr;

    m_speed = 6.0f;       // Running speed
    m_jumpForce = 6.0f;   // Upward velocity
    m_eyeHeight = 0.6f;   // Eyes are slightly above center of capsule
}

CFPSPlayer::~CFPSPlayer()
{
    // Cleanup Physics
    if (m_pWorld && m_pBody) {
        m_pWorld->removeRigidBody(m_pBody);
        delete m_pBody->getMotionState();
        delete m_pBody;
    }
    if (m_pShape) delete m_pShape;
}

void CFPSPlayer::Init(btDynamicsWorld* world, CQuatCamera* camera, const btVector3& startPos)
{
    m_pWorld = world;
    m_pCamera = camera;

    // 1. Create Capsule Shape
    // Width (Radius) = 0.5, Total Height = 1.8 (Height 0.8 + 2*Radius)
    m_pShape = new btCapsuleShape(0.5f, 0.8f);

    // 2. Calculate Inertia
    btScalar mass = 80.0f; // 80kg Human
    btVector3 localInertia(0, 0, 0);
    m_pShape->calculateLocalInertia(mass, localInertia);

    // 3. Create Rigid Body
    btDefaultMotionState* myMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), startPos));
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, m_pShape, localInertia);

    m_pBody = new btRigidBody(rbInfo);

    // ---------------------------------------------------------
    // CRITICAL FPS SETTINGS
    // ---------------------------------------------------------
    // A. Prevent Tipping: Lock rotation on X, Y, and Z axes. 
    // The capsule stays upright forever.
    m_pBody->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));
    // B. Disable Sleeping: We don't want the player to "freeze" if they stand still.
    m_pBody->setActivationState(DISABLE_DEACTIVATION);
    // C. Friction: High friction so we stop instantly when input stops
    m_pBody->setFriction(0.5f);

    // Add to world
    m_pWorld->addRigidBody(m_pBody);
}

void CFPSPlayer::Update(double dt)
{
    if (!m_pBody || !m_pCamera) return;

    // ---------------------------------------------------------
    // 1. GET INPUT DIRECTION RELATIVE TO CAMERA
    // ---------------------------------------------------------

    // Get Camera vectors
    // Assuming GetForwardVector() returns the Z-direction of camera
    // We need to construct them from the Camera's rotation if methods don't exist
    btVector3 camForward = m_pCamera->GetForwardVector(); // Or GetRotation().getColumn(2) * -1
    btVector3 camRight = m_pCamera->GetRightVector();   // Or GetRotation().getColumn(0)

    // FLATTEN VECTORS: We walk on the floor, we don't fly up/down
    camForward.setY(0.0f);
    camForward.normalize();
    camRight.setY(0.0f);
    camRight.normalize();

    btVector3 walkDir(0, 0, 0);

    // Simple Input Check (Win32)
    if (GetAsyncKeyState('W') & 0x8000) walkDir += camForward;
    if (GetAsyncKeyState('S') & 0x8000) walkDir -= camForward;
    if (GetAsyncKeyState('D') & 0x8000) walkDir += camRight;
    if (GetAsyncKeyState('A') & 0x8000) walkDir -= camRight;

    // Normalize so diagonal movement isn't faster
    if (walkDir.length2() > 0) {
        walkDir.normalize();
    }

    // ---------------------------------------------------------
    // 2. APPLY VELOCITY TO PHYSICS BODY
    // ---------------------------------------------------------

    // Get current velocity to preserve falling speed (Gravity)
    btVector3 currentVel = m_pBody->getLinearVelocity();

    // Calculate new velocity
    // We overwrite X and Z (Walking), but keep Y (Gravity/Jumping)
    btVector3 newVel = walkDir * m_speed;
    newVel.setY(currentVel.y());

    // Handle Jumping (Only if touching ground approx)
    // Simple check: Is vertical velocity near zero?
    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && fabs(currentVel.y()) < 0.1f)
    {
        newVel.setY(m_jumpForce);
    }

    // Apply to Body
    m_pBody->setLinearVelocity(newVel);

    // ---------------------------------------------------------
    // 3. SYNC CAMERA TO BODY
    // ---------------------------------------------------------

    btTransform trans;
    m_pBody->getMotionState()->getWorldTransform(trans);
    btVector3 bodyPos = trans.getOrigin();

    // Snap camera to body position + eye offset
    btVector3 eyePos = bodyPos + btVector3(0, m_eyeHeight, 0);

    m_pCamera->SetPosition(eyePos);
}