// --------------------------------------------------------------------------------
//  created: 20.12.2025 13:28 by JIyHb
// --------------------------------------------------------------------------------

#pragma once
#include "Logger.h"
#include "CSpriteBatch.h" // Ensure this is included!
#include "TextureTools.h"
    
struct Particle{
    D3DXVECTOR3 position;
    D3DXVECTOR3 velocity;   // Direction and speed
    D3DXCOLOR   color;      // Current color

    float       lifeTime;   // How long it lives (in seconds)
    float       startLife;  // Initial life (to calculate percentage for fading)
    float       size;       // Current size
};

struct Boid {
    D3DXVECTOR3 position;
    D3DXVECTOR3 velocity;
    D3DXVECTOR3 acceleration;
    float       life;      // 1.0 -> 0.0
    float       maxLife;

    // New: Trail History
    std::deque<D3DXVECTOR3> trail;
    D3DXVECTOR3 lastRecordedPos;
};

// --------------------------------------------------------------------------------
class BoidEmitter
{
private:
    std::vector<Boid> m_boids;

    // Settings
    float m_neighborDist; // How far can they see?
    float m_maxSpeed;
    float m_maxForce;

public:
    BoidEmitter(int maxCount);


    // The Magic: Calculates Flocking logic
    void Update(float deltaTime);

    // NEW: Render directly to the SpriteBatch
    void Render(CSpriteBatch* batch, IDirect3DTexture9* texture)
    {
        for (const auto& b : m_boids)
        {
            // 1. Convert D3DXVECTOR3 to btVector3
            btVector3 pos(b.position.x, b.position.y, b.position.z);

            // 2. Determine Orientation
            // Boids usually look better if they face their travel direction (Y-Axis Billboarding)
            // or simply act as billboards. Let's use BILLBOARD for generic usage.
            // If you want them to point forward, you'd use SPRITE_AXIS_Y 
            // and set rotation logic inside the batcher, or pass a specific rotation.

            // 3. Draw
            // Using a fixed size for boids (or add b.size to struct)
            float boidSize = 1.0f;

            batch->Draw(
                texture,
                pos,
                btVector2(boidSize, boidSize),
                0xFFFFFFFF, // White (or boid specific color)
                SPRITE_BILLBOARD
            );
        }
    }

    // Spawn 'count' particles at a specific position
    void Spawn(int count, D3DXVECTOR3 origin, float radius, float lifetime);

};

// --------------------------------------------------------------------------------
class ParticleEmitter
{
private:
    std::vector<Particle> m_particles;
    float m_spawnRate;      // How many particles per second
    float m_spawnAccumulator; // Internal timer

    // Emitter Settings
    D3DXVECTOR3 m_origin;
    D3DXVECTOR3 m_gravity; 
	D3DXCOLOR   m_startColor;
    float       m_maxLife;
    float       m_speed;
    float       m_startSize;
    float       m_endSize; // To shrink/grow particles

public:
    ParticleEmitter(D3DXVECTOR3 origin, float rate)
        : m_origin(origin), m_spawnRate(rate), m_spawnAccumulator(0.0f)
    {
        // Default settings
        m_maxLife = 3.0f;
        m_speed = 12.0f;
        m_startSize = 1.0f;
        m_endSize = 0.0f;
		m_startColor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f); // white
        m_gravity = D3DXVECTOR3(0.0f, -9.81f, 0.0f);
    }

    // 1. UPDATE: Run physics and spawn new particles
    void Update(float deltaTime)
    {
        // --- A. Spawning Logic ---
        m_spawnAccumulator += deltaTime;
        float timePerParticle = 1.0f / m_spawnRate;

        while (m_spawnAccumulator > timePerParticle)
        {
            SpawnParticle();
            m_spawnAccumulator -= timePerParticle;
        }

        // --- B. Physics & Aging Logic ---
        for (auto& p : m_particles)
        {
            // Move
            p.position += p.velocity * deltaTime;

            // Age
            p.lifeTime -= deltaTime;

            // Velocity increases downwards over time
            p.velocity += m_gravity * deltaTime;

            // Optional: Interpolate Size and Alpha based on age
            float ageRatio = p.lifeTime / p.startLife; // 1.0 = born, 0.0 = dead
            p.size = m_endSize + (m_startSize - m_endSize) * ageRatio;
            //p.color.a = ageRatio; // Fade out alpha
        }

        // --- C. Cleanup Dead Particles ---
        // Efficiently remove particles with life <= 0
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                [](const Particle& p) { return p.lifeTime <= 0.0f; }),
            m_particles.end()
        );
    }

    // 2. RENDER: Send data to the batch
    void Render(CSpriteBatch* batch, IDirect3DTexture9* texture)
    {
        for (const auto& p : m_particles)
        {
            // Convert types
            btVector3 pos(p.position.x, p.position.y, p.position.z);

            // Draw
            batch->Draw(
                texture,
                pos,
                btVector2(p.size, p.size),
                (D3DCOLOR)p.color,
                SPRITE_BILLBOARD
            );
        }
    }
    // SAVE: Writes current settings to disk
    void SaveConfig(const char* filename)
    {
        json j;
        // 1. Map C++ variables to JSON fields
        j["SpawnRate"] = m_spawnRate;
        j["Speed"] = m_speed;
        j["Lifetime"] = m_maxLife;
        j["StartSize"] = m_startSize;
        j["EndSize"] = m_endSize;
        // Arrays for Vector/Color
        j["Gravity"] = { m_gravity.x, m_gravity.y, m_gravity.z };
        j["Color"] = { m_startColor.r, m_startColor.g, m_startColor.b, m_startColor.a };

        // 2. Write to file
        std::ofstream file(filename);
        if (file.is_open()) {
            file << j.dump(4); // 4 = indentation spaces (pretty print)
            file.close();
        }
    }
private:
    void SpawnParticle()
    {
        Particle p;
        p.position = m_origin;

        // Random Velocity (Simple generic explosion/fountain)
        // Helper to get random float -1.0 to 1.0
        float rX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float rY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // vertical
        float rZ = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        D3DXVECTOR3 randDir(rX, rY + 1.0f, rZ); // +1 Y to shoot up
        D3DXVec3Normalize(&randDir, &randDir);

        p.velocity = randDir * m_speed;
        p.color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f); // Orange fire color
        p.lifeTime = m_maxLife;
        p.startLife = m_maxLife;
        p.size = m_startSize;

        m_particles.push_back(p);
    }
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// A dedicated emitter that spawns particles inside or on the surface of a sphere
class SphereEmitter
{
private:
    std::vector<Particle> m_particles;

    // Emitter Configuration
    btVector3 m_center;
    float     m_radius;
    float     m_spawnRate;
    float     m_spawnAccumulator;
    bool      m_surfaceOnly; // If true, spawns only on shell. If false, spawns inside volume.

    // Particle Settings
    float     m_speed;
    float     m_particleLife;
    float     m_startSize;
    float     m_endSize;
    D3DXCOLOR  m_startColor;
    D3DXCOLOR  m_endColor;

public:
    SphereEmitter(const btVector3& center, float radius, float rate)
        : m_center(center), m_radius(radius), m_spawnRate(rate), m_spawnAccumulator(0.0f)
    {
        // Defaults
        m_surfaceOnly = false; // Spawn inside volume by default
        m_speed = 1.0f;
        m_particleLife = 2.0f;
        m_startSize = 0.5f;
        m_endSize = 2.0f;
        m_startColor = D3DXCOLOR(1.0f, 1.0f, 0.5f, 1.0f); 
        m_endColor = D3DXCOLOR(0.0f, 1.0f, 0.5f, 0.5f);   
    }

    // Toggle between volume (solid sphere) or surface (hollow shell) spawning
    void SetSurfaceOnly(bool surface) { m_surfaceOnly = surface; }

    void Update(float dt)
    {
        // 1. Spawn New Particles
        m_spawnAccumulator += dt;
        float timePerParticle = 1.0f / m_spawnRate;

        while (m_spawnAccumulator > timePerParticle)
        {
            SpawnParticle();
            m_spawnAccumulator -= timePerParticle;
        }

        // 2. Update Existing Particles
        for (auto& p : m_particles)
        {
            p.position += p.velocity * dt;
            p.lifeTime -= dt;

            // Calculate normalized age 't'
            float t = 1.0f - (p.lifeTime / p.startLife);// 0.0 (born) -> 1.0 (dead)

            // Simple interpolation for size
            p.size = m_startSize + (m_endSize - m_startSize) * t;


            // Simple interpolation for color (Alpha fade)
            // Note: For complex color lerping, you'd separate ARGB channels.
            // Here we just fade Alpha for performance.
            //int alpha = (int)(255.0f * (1.0f - t));
            //if (alpha < 0) alpha = 0;

            // Keep RGB, modify A
            //p.color = (m_startColor & 0x00FFFFFF) | (alpha << 24);
			p.color = m_startColor + (m_endColor - m_startColor) * t;
            
        }

        // 3. Cleanup Dead
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                [](const Particle& p) { return p.lifeTime <= 0.0f; }),
            m_particles.end()
        );
    }

    void Render(CSpriteBatch* batch, IDirect3DTexture9* tex)
    {
        for (const auto& p : m_particles)
        {
			btVector3 pos(p.position.x, p.position.y, p.position.z);

            batch->Draw(
                tex,
                pos,
                btVector2(p.size, p.size),
                (D3DCOLOR)p.color,
                SPRITE_BILLBOARD
            );
        }
    }

private:
    void SpawnParticle()
    {
        Particle p;

        // --- SPHERE MATH ---
        // Generate a random point in a unit sphere
        float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
        float phi = acos(2.0f * ((float)rand() / RAND_MAX) - 1.0f);
        float u = (float)rand() / RAND_MAX; // Random 0..1

        float r = m_radius;
        if (!m_surfaceOnly)
        {
            // For uniform distribution inside volume, radius is cube root of random
            r *= pow(u, 1.0f / 3.0f);
        }

        // Convert Spherical to Cartesian
        float x = r * sin(phi) * cos(theta);
        float y = r * sin(phi) * sin(theta);
        float z = r * cos(phi);

        btVector3 offset(x, y, z);
        p.position = D3DXVECTOR3((const FLOAT)m_center.x(), (const FLOAT)m_center.y(), (const FLOAT)m_center.z()) + D3DXVECTOR3((const FLOAT)offset.x(), (const FLOAT)offset.y(), (const FLOAT)offset.z());

        // Velocity: Move outward from center
        btVector3 direction = offset;
        if (direction.length2() > 0.0001f) direction.normalize();
        else direction = btVector3(0, 1, 0); // Safety for center spawn

        p.velocity = D3DXVECTOR3((const FLOAT)direction.x(), (const FLOAT)direction.y(), (const FLOAT)direction.z()) * m_speed;

        // Init State
        p.lifeTime = m_particleLife;
        p.startLife = m_particleLife;
        p.size = m_startSize;
        p.color = m_startColor;

        m_particles.push_back(p);
    }
};
