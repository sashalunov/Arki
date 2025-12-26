// --------------------------------------------------------------------------------
//  created: 20.12.2025 13:28 by JIyHb
// --------------------------------------------------------------------------------

#include "StdAfx.h"

#include "CParticleSystem.h"
// Helper to get random float -1.0 to 1.0
float RandomFloat() { return ((float)rand() / RAND_MAX) * 2.0f - 1.0f; }
// Helper for 0.0 to 1.0 (Put this at top of file)
float Random01() { return (float)rand() / (float)RAND_MAX; }


BoidEmitter::BoidEmitter(int maxCount)
{
    m_boids.reserve(maxCount);

    // Tuning these values is the "Art" of boids
    m_neighborDist = 0.3f; 
    m_maxSpeed = 1.5f;
    m_maxForce = 50.03f;
}

void BoidEmitter::Spawn(int count, D3DXVECTOR3 origin, float radius, float lifetime)
{
    for (int i = 0; i < count; i++)
    {
        Boid b;
        b.position = origin;

        // --- SPHERE SPAWN LOGIC ---
        // 1. Pick a random direction
        D3DXVECTOR3 randomDir(RandomFloat(), RandomFloat(), RandomFloat());
        D3DXVec3Normalize(&randomDir, &randomDir);
        // 2. Pick a random distance (0 to Radius)
        // Note: Using simple Random01() creates a "Dense Core" look (more particles in center).
        // If you want perfectly uniform distribution, use: cbrt(Random01()) * radius
        float distance = Random01() * radius;
        // 3. Calculate final position
        b.position = origin + (randomDir * distance);

        // Initialize Trail Logic (Crucial for Ribbons!)
        b.lastRecordedPos = b.position;
        b.trail.clear(); // Clear any old junk memory
        b.trail.push_back(b.position);

        // Random velocity spread
        b.velocity = D3DXVECTOR3(RandomFloat(), RandomFloat(), RandomFloat());
        D3DXVec3Normalize(&b.velocity, &b.velocity);
        b.velocity *= m_maxSpeed;

        b.acceleration = D3DXVECTOR3(0, 0, 0);

        // --- NEW LIFETIME LOGIC ---
        // We add a small random variance (+/- 20%) so they don't all die at once.
        float variance = (RandomFloat() * 2.2f) * lifetime;
        b.maxLife = lifetime + variance;
        // Sanity check: ensure life is never negative
        if (b.maxLife < 0.1f) b.maxLife = 0.1f;
        b.life = b.maxLife;


        m_boids.push_back(b);
    }
}



void BoidEmitter::Update(float deltaTime)
{
    // Pre-calculate squared neighbor distance to avoid sqrt() in loop
    float neighborDistSq = m_neighborDist * m_neighborDist;

    // Target for seeking (Pre-normalize if needed)
    D3DXVECTOR3 targetPos(0, 10, 0);

    for (size_t i = 0; i < m_boids.size(); i++)
    {
        Boid& current = m_boids[i];

        D3DXVECTOR3 sep(0, 0, 0);
        D3DXVECTOR3 ali(0, 0, 0);
        D3DXVECTOR3 coh(0, 0, 0);
        int neighbors = 0;

        // Optimization: Calculate Seek once, use LengthSq
        D3DXVECTOR3 seek = targetPos - current.position;
        // Don't normalize yet if not strictly needed, but if you do:
        if (D3DXVec3LengthSq(&seek) > 0.001f) D3DXVec3Normalize(&seek, &seek);

        // --- INNER LOOP (The Hot Path) ---
        for (size_t j = 0; j < m_boids.size(); j++)
        {
            if (i == j) continue;

            D3DXVECTOR3 diff = current.position - m_boids[j].position;

            // OPTIMIZATION 1: Use LengthSq (No Square Root!)
            float distSq = D3DXVec3LengthSq(&diff);

            if (distSq < neighborDistSq && distSq > 0.0001f)
            {
                // Separation:
                // Old: Normalize(diff) / dist
                // New: diff / distSq (Mathematically equivalent, much faster)
                sep += diff / distSq;

                // Alignment:
                ali += m_boids[j].velocity;

                // Cohesion:
                coh += m_boids[j].position;

                neighbors++;
            }
        }

        if (neighbors > 0)
        {
            // Alignment & Cohesion Averages
            ali /= (float)neighbors;
            coh /= (float)neighbors;
            coh -= current.position; // Vector to center of mass

            // Normalize results (Only do sqrt HERE, once per boid, not per neighbor)
            if (D3DXVec3LengthSq(&sep) > 0) D3DXVec3Normalize(&sep, &sep);
            if (D3DXVec3LengthSq(&ali) > 0) D3DXVec3Normalize(&ali, &ali);
            if (D3DXVec3LengthSq(&coh) > 0) D3DXVec3Normalize(&coh, &coh);
        }

        // Apply Weights
        D3DXVECTOR3 steeringForce = (sep * 1.5f) + (ali * 1.0f) + (coh * 1.0f) + (seek * 0.5f);

        // Clamp Max Force
        if (D3DXVec3LengthSq(&steeringForce) > m_maxForce * m_maxForce)
        {
            D3DXVec3Normalize(&steeringForce, &steeringForce);
            steeringForce *= m_maxForce;
        }

        // Apply Physics
        current.acceleration += steeringForce;
        current.velocity += current.acceleration * deltaTime;

        // Limit Speed
        if (D3DXVec3LengthSq(&current.velocity) > m_maxSpeed * m_maxSpeed)
        {
            D3DXVec3Normalize(&current.velocity, &current.velocity);
            current.velocity *= m_maxSpeed;
        }

        current.position += current.velocity * deltaTime;
        current.acceleration = D3DXVECTOR3(0, 0, 0);

        // --- TRAIL UPDATE ---
        current.life -= deltaTime;

        // ... (Keep your existing trail logic here) ...
        D3DXVECTOR3 diff = current.position - current.lastRecordedPos;
        if (D3DXVec3LengthSq(&diff) > (0.2f * 0.2f))
        {
            current.trail.push_front(current.position);
            current.lastRecordedPos = current.position;
            if (current.trail.size() > 20) current.trail.pop_back();
        }
    }

    // OPTIMIZATION 2: Efficient Removal (Swap and Pop)
    // Avoids shifting the whole vector when a particle dies
    for (int i = 0; i < m_boids.size(); ) // Note: No i++ here
    {
        if (m_boids[i].life <= 0)
        {
            // Swap current dead boid with the last one in the list
            std::swap(m_boids[i], m_boids.back());
            // Remove the last one (which is now the dead one)
            m_boids.pop_back();
            // Do NOT increment i, because we need to check the boid we just swapped in!
        }
        else
        {
            i++;
        }
    }
}