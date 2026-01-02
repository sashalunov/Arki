// --------------------------------------------------------------------------------
//	file: CArkiLevel.h
//---------------------------------------------------------------------------------
//  created: 22.12.2025 8:49 by JIyHb
// --------------------------------------------------------------------------------
#pragma once
#include <btBulletDynamicsCommon.h>
#include "CArkiBlock.h"


enum ELevelFormula {
    FORMULA_RINGS = 0,
    FORMULA_WAVES = 1,
    FORMULA_PLASMA = 2,
    FORMULA_SIERPINSKI = 3, 
    FORMULA_SIERPINSKI_CARPET = 4,
    FORMULA_LIQUID = 5,     
	FORMULA_JULIA = 6,
    FORMULA_MANDELBROT = 7  
};
// --- The Recipe for a level ---
struct LevelParams {
    int seed;           // The "DNA" of the level
    int rows;
    int cols;
    int formulaType;    // ELevelFormula
    float scaleX;
    float scaleY;
    int offsetX;        // Fractal Panning
    int offsetY;

    // Helper: Create a random configuration
    static LevelParams Random(int r, int c, ELevelFormula f)
    {
        LevelParams p;
        p.rows = r;
        p.cols = c;
        p.seed = rand(); // Generate a new random seed

        // Use the new seed to pick randoms
        srand(p.seed);

        p.formulaType = f;// rand() % 7; // 0 to 6
        p.scaleX = (rand() % 5 + 3) / 10.0f;
        p.scaleY = (rand() % 5 + 3) / 10.0f;
        p.offsetX = rand() % 1000;
        p.offsetY = rand() % 1000;

        // Auto-fix scales for specific fractals
        if ( p.formulaType == FORMULA_SIERPINSKI_CARPET) {
            p.scaleX = 1.0f; p.scaleY = 1.0f;
        }
        if (p.formulaType == FORMULA_MANDELBROT) {
            p.scaleX = 3.0f / c; p.scaleY = 2.5f / r;
        }
		if (p.formulaType == FORMULA_JULIA || p.formulaType == FORMULA_SIERPINSKI) {
            p.offsetX = 0.0f; p.offsetY = 0.0f ;
        }
        return p;
    }
};

// --- NEW: This macro automatically creates to_json() and from_json() functions ---
// Place this OUTSIDE the struct definition but inside the header
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LevelParams, seed, rows, cols, formulaType, scaleX, scaleY, offsetX, offsetY)

class CArkiLevel
{

private:
    std::vector<CArkiBlock*> m_blocks;
    btDiscreteDynamicsWorld* m_pDynamicsWorld; // Reference to physics world
public:
    LevelParams m_currentParams;

public:
    CArkiLevel(btDiscreteDynamicsWorld* world) : m_pDynamicsWorld(world) { m_currentParams = LevelParams::Random(21,21, FORMULA_RINGS); }
    ~CArkiLevel() { Clear(); }

    void Clear()
    {
        for (auto b : m_blocks) {
            // Remove from physics world
            m_pDynamicsWorld->removeRigidBody(b->m_pBody);
            delete b;
        }
        m_blocks.clear();
    }
    void GenerateRandomLevel(int rows, int cols, bool symmetric);
    void GenerateIslandLevel(int rows, int cols, int numIslands);
    void GenerateMathLevel(int rows, int cols);
    void GenerateMathLevel(int rows, int cols, int patternType);
    void GenerateMathLevel(const LevelParams& params);

    void GenerateFractalLevel(int rows, int cols);
    void GenerateMengerLevel(int rows, int cols);
    void GenerateCaveLevel(int rows, int cols);
    // Helper to get current params (e.g. for UI display)
   // LevelParams GetCurrentParams() const { return m_currentParams; }
    void UpdateParameters(const LevelParams& params);
    // --- JSON SAVE ---
    void SaveLevel(const std::wstring& filename);
    //{
    //    json j;
    //    j["blocks"] = json::array();

    //    for (auto b : m_blocks)
    //    {
    //        if (b->m_isDestroyed) continue; // Don't save destroyed blocks

    //        j["blocks"].push_back({
    //            {"x", b->m_pos.x},
    //            {"y", b->m_pos.y},
    //            {"z", b->m_pos.z},
    //            { "c", (unsigned int)b->m_color } ,// Save color as unsigned int
    //            {"s", b->scoreValue}
    //            });
    //    }

    //    std::ofstream file(filename);
    //    file << j.dump(4); // Pretty print with 4 spaces
    //}

    // --- JSON LOAD ---
    void LoadLevel(const std::wstring& filename);
    //{
    //    Clear(); // Reset existing level

    //    std::ifstream file(filename);
    //    if (!file.is_open()) return;

    //    json j;
    //    file >> j;

    //    D3DXVECTOR3 halfSize(1.0f, 0.5f, 0.5f); // Standard block size

    //    for (const auto& item : j["blocks"])
    //    {
    //        float x = item["x"];
    //        float y = item["y"];
    //        float z = item["z"];
    //        // Load color (default to White if missing)
    //        D3DCOLOR col = item.contains("c") ? (D3DCOLOR)item["c"] : D3DCOLOR_XRGB(255, 255, 255);
    //        // Load score, default to 10 if loading an old file
    //        int score = item.contains("s") ? (int)item["s"] : 10;

    //        CArkiBlock* newBlock = new CArkiBlock(m_pDynamicsWorld, D3DXVECTOR3(x, y, z), halfSize, col, score);

    //        m_blocks.push_back(newBlock);
    //    }
    //}

    void Update()
    {
        for (auto b : m_blocks)
        {
            if (b->m_pendingDestruction && !b->m_isDestroyed)
            {
                // The physics engine has now finished the bounce (1 frame later).
                // NOW we can safely remove the block.

                b->m_isDestroyed = true; // Stops Rendering

                // Move body away instantly
                btTransform t;
                t.setIdentity();
                t.setOrigin(btVector3(0, -999, 0));
                b->m_pBody->setWorldTransform(t);

                // Disable physics for this block completely
                b->m_pBody->setActivationState(DISABLE_SIMULATION);
                b->m_pBody->setCollisionFlags(b->m_pBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
            }
        }

        // Iterate through the vector safely
        //for (auto it = m_blocks.begin(); it != m_blocks.end(); )
        //{
        //    CArkiBlock* b = *it;

        //    if (b->m_pendingDestruction)
        //    {
        //        // 1. Remove from Bullet Physics World
        //        // This stops the physics engine from tracking it immediately
        //        if (b->m_pBody) {
        //            m_pDynamicsWorld->removeRigidBody(b->m_pBody);
        //        }
        //        b->m_isDestroyed = true; // Stops Rendering
        //        // 2. Delete the Object from Memory
        //        // This frees the RAM and calls the destructor
        //        delete b;

        //        // 3. Remove from the std::vector
        //        // erase() returns the iterator to the *next* element, keeping the loop valid
        //        it = m_blocks.erase(it);
        //    }
        //    else
        //    {
        //        // Only advance if we didn't delete anything
        //        ++it;
        //    }
        //}
    }
    void CleanupBlocks()
    {
        for (int i = 0; i < m_blocks.size(); i++)
        {
            CArkiBlock* b = m_blocks[i];
            if (b->m_isDestroyed)
            {
                m_pDynamicsWorld->removeRigidBody(b->m_pBody);
                delete b;
                m_blocks.erase(m_blocks.begin() + i);
                i--;
            }
        }
    }

    void Render(IDirect3DDevice9* device, CSpriteFont* font, IDirect3DCubeTexture9* pReflectionTexture, float rotationAngle)
    {
        for (auto b : m_blocks)
        {
            b->Render(device, font , pReflectionTexture, rotationAngle);
        }
    }
};