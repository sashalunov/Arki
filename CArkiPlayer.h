#pragma once
#include "CRigidBody.h"
#include "CArkiBlock.h"
#include "CArkiBullet.h"
#include "CArkiBall.h"
#include "CSkybox.h"
#include "Sounds.h"

class CArkiPlayer
{
public:
    btRigidBody* m_pBody;
    float m_speed;
    btDiscreteDynamicsWorld* m_pDynamicsWorld; // Store reference to world to spawn bullets
	IDirect3DDevice9* m_pDevice;
    // Gun Management
    std::vector<CArkiBullet*> m_bullets;
	std::vector<CArkiBall*> m_balls; // Balls spawned from power-ups

    float m_shootCooldown;

    float m_health;
    float m_maxHealth;
    bool m_isDead;
    float m_invulnerabilityTimer; // Prevent instant death from multiple hits
    // Visual feedback for damage
    D3DXCOLOR m_color;

public:
    CArkiPlayer(btDiscreteDynamicsWorld* dynamicsWorld, IDirect3DDevice9* device, D3DXVECTOR3 startPos);
    
    ~CArkiPlayer()
    {
        // Cleanup Bullets
        for (auto* b : m_bullets) {
            m_pDynamicsWorld->removeRigidBody(b->m_pBody);
            delete b;
        }
        m_bullets.clear();
		for (auto* ball : m_balls) {
			delete ball;
		}
		m_balls.clear();
        // ... existing cleanup ...
        if (m_pBody && m_pBody->getUserPointer())
        {
            delete (PhysicsData*)m_pBody->getUserPointer();
        }
    }

    void AddBall()
    {
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        btVector3 spawnPos = trans.getOrigin();
        // Spawn slightly above the paddle so it doesn't get stuck inside it
        spawnPos.setY(spawnPos.getY() + 1.0f);
		D3DXVECTOR3 spawnPosVec((FLOAT)spawnPos.getX(), (FLOAT)spawnPos.getY(), (FLOAT)spawnPos.getZ());
		CArkiBall* ball = new CArkiBall(m_pDynamicsWorld, m_pDevice, spawnPosVec, 0.3f, 20.0f);
        m_balls.push_back(ball);
	}

    void LaunchBalls()
    {
        for (auto* ball : m_balls)
        {
            ball->Launch();
        }
	}   

    void Shoot()
    {
        if (m_shootCooldown > 0) return;
        //PlayAudioFromMemory(GeneratePlasmaShot());

        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        btVector3 spawnPos = trans.getOrigin();

        // Spawn slightly above the paddle so it doesn't get stuck inside it
        spawnPos.setY(spawnPos.getY() + 1.0f);

        CArkiBullet* newBullet = new CArkiBullet(m_pDynamicsWorld, spawnPos);
        m_bullets.push_back(newBullet);

        m_shootCooldown = 0.25f; // Delay between shots
    }

    void Update(float deltaTime, bool moveLeft, bool moveRight)
    {
        // If dead, do not move or update logic
        if (m_isDead) return;

        // --- NEW: Invulnerability Logic ---
        if (m_invulnerabilityTimer > 0.0f)
        {
            m_invulnerabilityTimer -= deltaTime;

            // Flash effect: Toggle opacity or color interpolation
            // Simple linear interpolation back to white
            float t = 1.0f - (m_invulnerabilityTimer / 2.0f); // 0.0 to 1.0
			D3DXCOLOR r = D3DXCOLOR(1, 0, 0, 1);
			D3DXCOLOR w = D3DXCOLOR(1, 1, 1, 1);
            D3DXColorLerp(&m_color, &r, &w, t);
        }
        else
        {
            m_color = D3DXCOLOR(1, 1, 1, 1);
        }


        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        btVector3 origin = trans.getOrigin();

        if (moveLeft)  origin -= btVector3(m_speed * deltaTime,0,0);
        if (moveRight) origin += btVector3(m_speed * deltaTime, 0, 0);

        // Apply boundaries if needed
        // if (origin.getX() < -10) origin.setX(-10);

        trans.setOrigin(origin);
        m_pBody->getMotionState()->setWorldTransform(trans);

        // 2. Handle Shooting
        if (m_shootCooldown > 0) m_shootCooldown -= deltaTime;
        //if (fireInput) Shoot();

		for (auto* ball : m_balls)
        {
            if(ball->m_isActive)
                ball->Update(deltaTime);
            else
            {
				ball->SetPosition((FLOAT)origin.getX(), (FLOAT)origin.getY() + 1.0f, (FLOAT)origin.getZ());
            }
        }

       // 3. Update Bullet Timers (BUT DO NOT DELETE HERE)
        for (auto* b : m_bullets)
        {
            b->Update(deltaTime);

            // Boundary Check
            if (b->m_pBody->getWorldTransform().getOrigin().getY() > 50.0f) {
                b->m_markForDelete = true;
            }
        }
    }

    // Call this AFTER CheckCollisions()
    void CleanupBullets()
    {
        for (int i = 0; i < m_bullets.size(); i++)
        {
            CArkiBullet* b = m_bullets[i];
            if (b->m_markForDelete)
            {
                m_pDynamicsWorld->removeRigidBody(b->m_pBody);
                delete b;
                m_bullets.erase(m_bullets.begin() + i);
                i--;
            }
        }

        for (int i = 0; i < m_balls.size(); i++)
        {
            CArkiBall* ball = m_balls[i];
            if (ball->m_lifetime == 0.0f)
            {
                m_pDynamicsWorld->removeRigidBody(ball->m_pBody);
                delete ball;
                m_balls.erase(m_balls.begin() + i);
                i--;
            }
		}
    }
    D3DXVECTOR3 GetPosition()
    {
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        return D3DXVECTOR3((FLOAT)trans.getOrigin().getX(), (FLOAT)trans.getOrigin().getY(), (FLOAT)trans.getOrigin().getZ());
    }

    void Render(CSkybox* sky, D3DXMATRIXA16 view)
    {
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);

        // ... Get Transform from Bullet and Draw Cube ...
        D3DXMATRIXA16 matWorld, matRot, matScale, matTrans;

        D3DXMatrixScaling(&matScale, 4, 0.5f, 1);
        D3DXMatrixTranslation(&matTrans, (FLOAT)trans.getOrigin().getX(), (FLOAT)trans.getOrigin().getY(), 0);
        matWorld = matScale * matTrans;
        m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        CRigidBody::s_pRigidBodyBoxMesh->DrawSubset(0);


        for (auto* ball : m_balls)
        {
            ball->Render(m_pDevice, sky->GetTexture(), view, sky->GetRotationX());
		}

        // Render Bullets
        for (auto* b : m_bullets)
        {
            b->Render(m_pDevice);
        }
    }

    void OnDeviceLost()
    {
        for (auto* ball : m_balls)
        {
            ball->OnDeviceLost();
        }
	}

    void OnDeviceReset()
    {
        for (auto* ball : m_balls)
        {
            ball->OnDeviceReset(m_pDevice);
        }
	}

    void TakeDamage(float damage);

};