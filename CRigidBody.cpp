#include "stdafx.h"
#include "CRigidBody.h"

// IMPORTANT: Define the static variable in your .cpp file (outside any function)
ID3DXMesh* CRigidBody::s_pRigidBodyBoxMesh = NULL;
ID3DXMesh* CRigidBody::s_pRigidBodySphereMesh = NULL;
ID3DXMesh* CRigidBody::s_pRigidBodyCapsuleMesh = NULL;
ID3DXMesh* CRigidBody::s_pRigidBodyCylinderMesh = NULL;

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
CRigidBody::CRigidBody()
    : m_position(0, 0, 0)
    , m_scale(1, 1, 1)
    , m_rigidBody(nullptr)
    , m_shape(nullptr)
    , m_world(nullptr)
    , m_ownsShape(false)
    , m_name("object")
    , m_eulerAngles(0, 0, 0)
    , m_isVisible(true)
{
    D3DXQuaternionIdentity(&m_rotation);
    m_type = RB_BOX;
}

// --- 1. SETUP: Now with Active/Passive Control ---
// isDynamic = false (Default): Mass is forced to 0. Object is STATIC.
// isDynamic = true:            Mass is used. Object is ACTIVE.
void CRigidBody::InitPhysics(btDynamicsWorld* world, btCollisionShape* shape, float mass, bool isDynamic = false, bool ownsShape = true)
{
    m_world = world;
    m_shape = shape;
    m_ownsShape = ownsShape;

    // LOGIC: If passive (not dynamic), mass must be 0.0f in Bullet.
    float finalMass = isDynamic ? mass : 0.0f;

    // 1. Calculate Inertia
    btVector3 localInertia(0, 0, 0);
    if (finalMass > 0.0f) {
        m_shape->calculateLocalInertia(finalMass, localInertia);
    }

    // 2. Create Body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(finalMass, this, m_shape, localInertia);
    m_rigidBody = new btRigidBody(rbInfo);
    m_rigidBody->setRestitution(1.0f);
    m_rigidBody->setFriction(0.0f);
    m_rigidBody->setRollingFriction(0.0f);
    m_rigidBody->setDamping(0.0f, 0.0f);
    m_rigidBody->setLinearFactor(btVector3(1, 1, 0));
    // 3. Set User Pointer for collision tracking
    m_pPhysicsData = new PhysicsData{ TYPE_WALL, this };
    m_rigidBody->setUserPointer(m_pPhysicsData);
    // 3. Add to World
    if (m_world) {
        m_world->addRigidBody(m_rigidBody, COL_WALL, COL_BALL|COL_POWERUP|COL_BULLET| COL_WALL| COL_BLOCK);
    }

    m_isSelected = false;
    RescaleObject(btVector3(m_scale.x, m_scale.y, m_scale.z));
}

// Helper: Create a box (Passive by default)
void CRigidBody::InitBox(btDynamicsWorld* world, D3DXVECTOR3 size, float mass, bool isDynamic = false)
{
	m_type = RB_BOX;
    btCollisionShape* boxShape = new btBoxShape(btVector3(1, 1, 1));
    // We pass 'true' for ownsShape because we just created it here
    m_scale = size;
    InitPhysics(world, boxShape, mass, isDynamic, true);
}
// 2. SPHERE: simple radius
void CRigidBody::InitSphere(btDynamicsWorld* world, float radius, float mass, bool isDynamic = false)
{
    m_type = RB_BALL;

    btCollisionShape* shape = new btSphereShape(radius);
    m_scale = D3DXVECTOR3(radius , radius , radius ) * 2.0f;
    InitPhysics(world, shape, mass, isDynamic, true);

}

// 3. CAPSULE: Good for characters. Aligned on Y-axis.
// Note: 'height' is the height of the cylinder part only. Total height = height + 2 * radius.
void CRigidBody::InitCapsule(btDynamicsWorld* world, float radius, float height, float mass, bool isDynamic = false)
{
	m_type = RB_CAPSULE;
    btCollisionShape* shape = new btCapsuleShape(radius, height);
    InitPhysics(world, shape, mass, isDynamic, true);
}

// 4. CYLINDER: Aligned on Y-axis.
void CRigidBody::InitCylinder(btDynamicsWorld* world, float radius, float height, float mass, bool isDynamic = false)
{
	m_type = RB_ROD;
    // Bullet defines cylinders by "Half Extents".
    // X = radius, Y = half-height, Z = radius
    btCollisionShape* shape = new btCylinderShape(btVector3(radius, height * 0.5f, radius));
    InitPhysics(world, shape, mass, isDynamic, true);
}
// --- 2. CLEANUP ---
void CRigidBody::DestroyPhysics()
{
    // 1. Remove from Physics World
    if (m_rigidBody && m_world) {
        m_world->removeRigidBody(m_rigidBody);
    }
    // 2. Clean up physics data
    if (m_pPhysicsData) {
        delete m_pPhysicsData;
        m_pPhysicsData = nullptr;
    }
    if (m_rigidBody) {
        m_rigidBody->setMotionState(nullptr);
        delete m_rigidBody->getMotionState();
        delete m_rigidBody;
        m_rigidBody = nullptr;
    }
    if (m_shape && m_ownsShape) {
        delete m_shape;
        m_shape = nullptr;
    }
}
