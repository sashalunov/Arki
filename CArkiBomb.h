#pragma once
#include "stdafx.h"
#include "CXAudio.h"
#include "CQuatCamera.h"
#include "CArkiPlayer.h"
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btMinMax.h>

// Assuming you have an Enum for types in CArkiBlock.h or similar
// If not, just ensure Player is identified correctly via UserPointer
// #include "CArkiPlayer.h" 
class CArkiBomb : public CRigidBody
{
private:
    double m_lifeTimer;       // Fuse timer
    float m_explodeRadius;   // How far the explosion reaches
    float m_maxForce;        // Physical push strength
    float m_maxDamage;       // Damage at center of explosion
    bool  m_hasExploded;     // State tracker
    CQuatCamera* m_cam;
	CArkiPlayer* m_player;
    tweeny::tween<float, float> m_explTween;
	float m_animScale;
    float m_animAlpha;
public:
    CArkiBomb(btDynamicsWorld* world, D3DXVECTOR3 startPos, CQuatCamera* c)
    {
        m_hasExploded = false;
        m_lifeTimer = 3.0f;       // 5 Seconds Fuse
        m_explodeRadius = 15.0f;  // Range
        m_maxForce = 80.0f;       // Push Strength
        m_maxDamage = 100.0f;     // Max HP damage
        m_cam = c;
        m_position = startPos;
        m_rotation = D3DXQUATERNION(0, 0, 0, 1);
       // m_scale = D3DXVECTOR3(1, 1, 1);
        m_isVisible = true;
        m_animAlpha = 0.0f;
		m_animScale = 0.0f;
        // 1. Init as a Dynamic Sphere (Radius 0.5, Mass 10.0)
        // Mass must be > 0 for gravity to affect it
        InitSphere(world, 0.5f, 10.0f, true);

        // 2. Set Position Physics
        SetPosition(startPos.x, startPos.y, startPos.z);

        // 3. User Pointer for ID (Optional, helps if Bomb is hit by other things)
        // Assuming TYPE_BOMB exists in your enum, otherwise use a generic ID
        PhysicsData* pData = new PhysicsData{ TYPE_BLOCK, this };
        m_rigidBody->setUserPointer(pData);
        m_pPhysicsData = pData; // Keep track for cleanup in CRigidBody destructor

        // tweeny works in milliseconds (usually int), so convert seconds -> ms
        int durationMs = (int)(1.0f * 1000.0f);

		InitMaterialS(m_material, 1.0f, 0.05f, 0.02f, 0.02f, 0.8f); // Slightly shiny orange material
		m_material.Power = 60.0f;
		m_material.Ambient = D3DXCOLOR(0.1f, 0.0f, 0.0f, 1.0f);
        m_material.Emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);

        // Configure the Tween:
        // 1. Start at 'strength'
        // 2. Go to '0.0f'
        // 3. Take 'durationMs' time
        // 4. Use Quadratic Easing for smooth fade-out
        m_explTween = tweeny::from(0.98f, 0.0f)
            .to(m_explodeRadius, 1.0f)
            .during(durationMs)
            .via(tweeny::easing::exponentialIn);


    }
    void Render(IDirect3DDevice9* device)
    {
        CRigidBody::Render(device); // Call base class render (if any)
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

        // Important: Disable Texture if you just want a colored glass look
        device->SetTexture(0, NULL);
        D3DXMATRIXA16 matScale;
        D3DXMATRIXA16 worldMat = GetWorldMatrix();
        D3DXMatrixScaling(&matScale, m_animScale, m_animScale, m_animScale);
        D3DXMATRIXA16 finalMat = matScale * worldMat;
        device->SetTransform(D3DTS_WORLD, &finalMat);
        s_pRigidBodySphereMesh->DrawSubset(0);

        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    }

    // Call this every frame from Game Loop
    void Update(double dt)
    {
        if (m_hasExploded) return;

        m_lifeTimer -= dt;

        // Flash Color or Pulse size logic could go here
        if (m_lifeTimer < 1.0f)
        {
            // Example: Pulse size
            //float scaleFactor = 1.0f + 0.6f * sinf(m_lifeTimer * 20.0f);
			m_material.Diffuse = D3DXCOLOR(0.75f, 0.45f, 0.0f, 1.0f) *  (FLOAT)btClamped((1.0 - m_lifeTimer),0.0,1.0); // Glow more as it nears explosion
            m_material.Emissive = D3DXCOLOR(0.75f, 0.45f, 0.0f, 1.0f) * (FLOAT)btClamped((1.0 - m_lifeTimer), 0.0, 1.0); // Glow more as it nears explosion

           // m_scale = D3DXVECTOR3(scaleFactor, scaleFactor, scaleFactor);
            //RescaleObject(btVector3(m_scale.x, m_scale.y, m_scale.z));

                // 1. Convert dt to milliseconds for tweeny
            int dtMs = (int)(dt * 1000);

            // 2. Step the animation
            // 'step' advances the time and returns the current interpolated value (current strength)
            auto values = m_explTween.step(dtMs);
            m_animScale = std::get<0>(values);
            m_animAlpha = std::get<1>(values); // Scale up for visual effect
            //RescaleObject(btVector3(currentScale, currentScale, currentScale));

        }
        if (m_lifeTimer <= 0.0f)
        {
            Explode();
        }
    }

    void Explode()
    {
        if (m_hasExploded) return;
        m_hasExploded = true;

        // 1. Hide the bomb mesh (it's gone)
        m_isVisible = false;

        // 2. Get Bomb Position in Bullet coordinates
        btVector3 bombPos = m_rigidBody->getCenterOfMassPosition();

        // 3. Iterate over ALL objects in the physics world
        // Note: m_world was stored in CRigidBody base class
        int numObjects = m_world->getNumCollisionObjects();
        btAlignedObjectArray<btCollisionObject*> objects = m_world->getCollisionObjectArray();

        for (int i = 0; i < numObjects; i++)
        {
            btCollisionObject* colObj = objects[i];
            btRigidBody* body = btRigidBody::upcast(colObj);

            // Skip self and non-rigid bodies
            if (!body || body == m_rigidBody) continue;

            // Calculate Distance
            btVector3 targetPos = body->getCenterOfMassPosition();
            FLOAT dist = (FLOAT)bombPos.distance(targetPos);

            // Check if within Explosion Radius
            if (dist < m_explodeRadius)
            {
                // Calculate Falloff (1.0 at center, 0.0 at edge)
                // Use slightly offset distance to prevent DivideByZero force
                float impactFactor = 1.0f - (dist / m_explodeRadius);
                if (impactFactor < 0.0f) impactFactor = 0.0f;

                // A. PHYSICS PUSH
                // Direction from Bomb -> Object
                btVector3 dir = targetPos - bombPos;
                dir.normalize();

                // Apply Impulse (Instant force)
                // We scale force by impactFactor so closer objects fly faster
                body->activate(true); // Wake up sleeping objects
                body->applyCentralImpulse(dir * m_maxForce * impactFactor);

                // B. PLAYER DAMAGE
                // Check if this object is the Player via UserPointer
                PhysicsData* pData = (PhysicsData*)body->getUserPointer();
                if (pData && pData->type == TYPE_PLAYER)
                {
                    // Calculate linear falloff damage
                    float damage = m_maxDamage * impactFactor;
                    if (m_cam)m_cam->Shake(impactFactor, 1.0f);

                    // Assuming player has a method TakeDamage(float amount)
                    CArkiPlayer* player = (CArkiPlayer*)pData->pObject;
					m_player = player;
                    player->TakeDamage(damage);

                    // printf("Player hit for %f damage!\n", damage);
                }

                // Optional: Destroy destructible blocks nearby
                // if (pData && pData->type == TYPE_BLOCK) { ... }
            }
        }

        // 4. Cleanup Physics immediately so it stops blocking player movement
        // We assume 'this' object might be deleted by a manager later, 
        // but we can remove the body from simulation now.
        m_world->removeRigidBody(m_rigidBody);

        // Setup visual effect (particle system) here if you have one
        xau->Play3D("explo1", bombPos);


    }

    bool IsDead() const { return m_hasExploded; }
};