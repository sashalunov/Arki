#include "stdafx.h" 
#include "EnemySpawner.h"

CEnemySpawner::CEnemySpawner(btDiscreteDynamicsWorld* world, CBulletManager* manager, CXMesh* enemyMesh)
    : m_pWorld(world),
	m_pBulletMan(manager),
    m_pEnemyMesh(enemyMesh)
{
    m_currentWaveIndex = -1; // Not started
    m_waveTimer = 0.0f;
    m_isWaveActive = false;
    m_timeBetweenWaves = 3.0f; // 3 Seconds rest between waves
    // --- DEFINE YOUR LEVELS HERE ---
    // Wave 1: 5 Enemies in a Grid, simple Sine movement
    //m_waves.push_back({ 5, FORM_GRID, STATE_ATTACK, 1.0f });
    // Wave 2: 7 Enemies in V-Shape, ZigZag movement
    m_waves.push_back({ 5, FORM_V_SHAPE, STATE_ATTACK, 1.0f }); // STATE_ATTACK maps to ZigZag if randomized
    // Wave 3: 8 Enemies in a Circle, Spiral movement
    //m_waves.push_back({ 8, FORM_CIRCLE, STATE_RETREAT, 1.5f });
}

CEnemySpawner::~CEnemySpawner() 
{
    // Cleanup Logic (Delete all pointers)
    for (auto e : m_enemies) delete e;
    m_enemies.clear();
}

// -------------------------------------------------------------
// ALGORITHM: FORMATION CALCULATOR
// Returns a list of positions based on the pattern type
// -------------------------------------------------------------
std::vector<D3DXVECTOR3> CEnemySpawner::CalculateFormation(EFormationType type, int count, D3DXVECTOR3 center)
{
    std::vector<D3DXVECTOR3> positions;
    float spacing = 3.5f; // Distance between enemies

    switch (type)
    {
    case FORM_GRID:
        // A simple row or double row
        for (int i = 0; i < count; i++) {
            // Center the line: (i - count/2)
            float x = center.x + (i - (count / 2.0f)) * spacing;
            positions.push_back(D3DXVECTOR3(x, center.y, center.z));
        }
        break;

    case FORM_V_SHAPE:
        // Flying V (Ducks/Jets)
        for (int i = 0; i < count; i++) {
            // Offset from center index
            float offset = (float)(i - count / 2);
            float x = center.x + offset * spacing;
            // The further from center, the higher up (abs value)
            float y = center.y + fabs(offset) * 2.0f;
            positions.push_back(D3DXVECTOR3(x, y, center.z));
        }
        break;

    case FORM_CIRCLE:
        // Perfect Ring
        for (int i = 0; i < count; i++) {
            float angle = (360.0f / count) * i * (3.14159f / 180.0f);
            float radius = 8.0f;
            float x = center.x + cos(angle) * radius;
            float y = center.y + sin(angle) * radius;
            positions.push_back(D3DXVECTOR3(x, y, center.z));
        }
        break;

    case FORM_DUAL_LINE:
        // Two columns
        for (int i = 0; i < count; i++) {
            float x = (i % 2 == 0) ? -5.0f : 5.0f; // Left or Right
            float y = center.y + (i / 2) * spacing;
            positions.push_back(D3DXVECTOR3(x, y, center.z));
        }
        break;
    }
    return positions;
}

void CEnemySpawner::StartNextWave()
{
    m_currentWaveIndex++;
    if (m_currentWaveIndex >= m_waves.size()) {
        m_currentWaveIndex = 0; // Loop game or End game
    }

    WaveDef& wave = m_waves[m_currentWaveIndex];

    // 1. Calculate Positions
    D3DXVECTOR3 spawnCenter(0, 20, 0); // High up
    auto positions = CalculateFormation(wave.formation, wave.enemyCount, spawnCenter);

    // 2. Spawn Enemies
    for (D3DXVECTOR3 pos : positions)
    {
        // Create the enemy
        CFlyingEnemy* newEnemy = new CFlyingEnemy(m_pWorld, m_pBulletMan, pos, m_pEnemyMesh);
        // Setup their specific behavior for this wave
        // (Assuming you added a SetDifficulty or similar method to CFlyingEnemy)
        // newEnemy->SetSpeed(wave.difficultySpeed); 
        m_enemies.push_back(newEnemy);
    }
    m_isWaveActive = true;
}

void CEnemySpawner::Update(double dt, D3DXVECTOR3 playerPos)
{
    // --- 1. WAVE MANAGEMENT ---
    if (m_enemies.empty() && m_isWaveActive) 
    {
        // Wave Defeated! Start Cooldown
        m_isWaveActive = false;
        m_waveTimer = m_timeBetweenWaves;
    }

    if (!m_isWaveActive) 
    {
        m_waveTimer -= (FLOAT)dt;
        if (m_waveTimer <= 0.0f) {
            StartNextWave();
        }
    }

    // --- 2. UPDATE ENEMIES ---
    // Using simple loop and manual erasure for clarity
    for (int i = 0; i < m_enemies.size(); i++)
    {
        CFlyingEnemy* e = m_enemies[i];
        // Pass player pos for aiming
        // Update logic (Movement, Shooting)
        // Note: You might need to refactor CFlyingEnemy::Update to accept playerPos 
        // if you want them to track the player every frame.
        e->Update(dt);

        if (e->m_isDead) 
        {
            delete e; // Physics cleanup handles itself in destructor
            m_enemies.erase(m_enemies.begin() + i);
            i--;
        }
    }
}

void CEnemySpawner::Render(IDirect3DDevice9* device)
{
    for (auto e : m_enemies) e->Render(device);
}