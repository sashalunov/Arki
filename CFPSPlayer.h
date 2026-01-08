#pragma once
#include <btBulletDynamicsCommon.h>
#include "CQuatCamera.h"

class CFPSPlayer
{
private:
    // Physics
    btRigidBody* m_pBody;
    btCapsuleShape* m_pShape;
    btDynamicsWorld* m_pWorld;

    // Link to your existing Camera
    CQuatCamera* m_pCamera;

    // Settings
    float m_speed;
    float m_jumpForce;
    float m_eyeHeight; // Distance from center of capsule to eyes

public:
    CFPSPlayer();
    ~CFPSPlayer();

    // Initialize Physics (Capsule)
    void Init(btDynamicsWorld* world, CQuatCamera* camera, const btVector3& startPos);
    // Process Movement inputs and sync Camera
    void Update(double deltaTime);
    void Render();
    // Get the raw body if needed
    btRigidBody* GetRigidBody() { return m_pBody; }
};
