#pragma once
#include "CBulletDebugDrawer.h"
#include "Logger.h"
#include "D3DRender.h"
#include "CQuatCamera.h"
#include "XMesh.h"
#include "CParticleSystem.h"
#include "CRigidBody.h"
#include "CSkybox.h"
#include "CArkiBlock.h"
#include "CArkiBall.h"
#include "CArkiPlayer.h"
#include "CArkiLevel.h"
#include "CSpriteFont.h"
#include "CMainMenu.h"
#include "CArkiPowerup.h"
#include "CGrid.h"
#include "CGizmo.h"
#include "COrbitCamera.h"
#include "CArkiCliffTreadmill.h"
#include "EnemySpawner.h"
#include "BulletManager.h"
#include "CHUD.h"
#include "CBSPlevel.h"
#include "tweeny.h"
using tweeny::easing;

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,    
    STATE_EDITOR,    
    STATE_SETTINGS,
    STATE_LEVEL_COMPLETE
};
// ------------------------------------------------------------------------------------
struct FloatingText3D {
    // The Tween Object: Handles 3 values: Y-Offset, Alpha, Scale
    tweeny::tween<float, float, float> anim;
	std::wstring scoreText;
    D3DXVECTOR3 startPos;
    bool isActive;

    void Spawn(D3DXVECTOR3 pos, const std::wstring& text)
    {
        startPos = pos;
        isActive = true;
		scoreText = text;
        // Setup the animation:
        // Value 1 (Y-Offset): From 0.0 to 20.0 (Float up)
        // Value 2 (Alpha):    From 255.0 to 0.0 (Fade out)
        // Value 3 (Scale):    From 0.05 to 0.10 (Grow)
        anim = tweeny::from(0.0f, 255.0f, 0.012f)
            .to(1.0f, 0.0f, 0.026f)
            .during(60) // 60 Frames (approx 1 sec at 60fps)
            .via(tweeny::easing::linear); // Nice "pop" effect
    }

    void Update(float dt) {
        if (!isActive) return;

        // Advance 1 frame
        anim.step(1);

        // Check completion
        if (anim.progress() >= 1.0f) {
            isActive = false;
        }
    }

    void Render(CSpriteFont* font) {
        if (!isActive) return;

        // Get current values
        // peek() returns a tuple, usually we use values struct or direct access
        auto values = anim.peek();
        float yOffset = std::get<0>(values);
        float alpha   = std::get<1>(values);
        float scale   = std::get<2>(values);

        // Calculate current position
        D3DXVECTOR3 drawPos = startPos;
        drawPos.y += yOffset; // Move up
        //drawPos.z += yOffset; // Move up


        // Draw
        // Note: You need to construct Color with the dynamic Alpha
        D3DCOLOR color = D3DCOLOR_ARGB((int)alpha, 255, 255, 0); // Yellow fading out

        font->DrawString3D(drawPos, scale, scoreText, color);
    }
};
// ------------------------------------------------------------------------------------
class ArkiGame
{
public:
    bool m_isPaused;
	bool m_isEditorMode;
    float scrollAmount;
private:
	CBSPlevel* m_bspLevel;
    LevelParams p;
    CEnemySpawner* m_spawner;
	CBulletManager* m_bulletManager;
    CMainMenu* m_mainMenu;
    CHUD* g_HUD;
    GameState m_gameState;
	CGrid* m_grid;
    CGizmo* g_gizmo;
    CTimer m_Timer;
    CSkybox* m_pSkybox; // Pointer allows it to be NULL if level has no sky
    CQuatCamera* m_pCam0;
	COrbitCamera* m_pCamEditor;
    CXMesh* m_pBodyMesh;
    CXMesh* m_pSkullMesh;
    CXMesh* m_pTeapotMesh;
	// rigid bodies
    CRigidBody* m_floor;    CRigidBody* m_top;
    CRigidBody* m_wallL;    CRigidBody* m_wallR;
	ID3DXMesh* m_roundedBoxMesh;

    std::vector<CRigidBody*> m_sceneObjects;
    std::vector<CFlyingEnemy*> m_enemies;
    CRigidBody* g_selected;
    // powerups
    std::vector<CArkiPowerup*> m_powerups;
    std::vector<bool> m_dropDeck;
    int m_deckIndex;
    std::mt19937 m_rng;              // High-quality Random Number Generator

    // particle systems
    ParticleEmitter* emmiter1;
    BoidEmitter* eb;
	SphereEmitter* es;
    std::vector<CArkiCliffTreadmill*> m_leftWalls;
    std::vector<CArkiCliffTreadmill*> m_rightWalls;

    long mouseDeltaX;
    long mouseDeltaY;
    double fFrameTime;
    double fDeltaTime;

    // --- Fixed Update Constants ---
    const double FIXED_DT = 1.0 / 60.0;    // Target 60 updates per second (0.0166s)
    const double MAX_FRAME_TIME = 0.25;    // Cap time to prevent "spiral of death" if game lags
    double g_accumulator = 0.0;            // Stores accumulated time
    
    std::vector<FloatingText3D*> m_ftext;

    D3DLIGHT9 m_LightDefault;
	D3DMATERIAL9 m_MaterialDefault;

    // bullet physics
    btBroadphaseInterface* g_broadphase;
    btDefaultCollisionConfiguration* g_collisionConfiguration;
    btCollisionDispatcher* g_dispatcher;
    btSequentialImpulseConstraintSolver* g_solver;
    btDiscreteDynamicsWorld* g_dynamicsWorld;

    CBulletDebugDrawer* btDebugDrawer;
    bool m_debugdraw = FALSE;
    bool m_pfxdraw = FALSE;
    bool m_usemouse = FALSE;
    bool m_usesnap = FALSE;
    float m_val = 0.0f;
    bool m_inputLeft;
	bool m_inputRight;
    bool m_showGrid;
    //arkanoid game
    CArkiLevel* m_currentLevel;
    // Initialization
	CArkiPlayer* m_player;// = new CArkiPlayer(g_DynamicsWorld, D3DXVECTOR3(0, -10, 0), D3DXVECTOR3(3.0f, 0.5f, 0.5f));
    CSpriteFont* m_font;

    CSpriteBatch* m_sbatch;
    // ... Texture loading ...
    IDirect3DTexture9* m_radialTex;
    D3DXVECTOR3 vNear, vFar, rayDir;


public:
    ArkiGame();
    ~ArkiGame() {};
    
    void LoadLevel(int levelID);
    bool Init();       // Setup DX9, Physics, Assets
    void Run();        // The Infinite Loop
    void Shutdown();   // Cleanup
    BOOL ResetDevice();
	void SetMouseDeltas(long dx, long dy) { mouseDeltaX += dx; mouseDeltaY += dy; }
    void RestartLevel();

    void AddCube(btDiscreteDynamicsWorld* dynamicsWorld, IDirect3DDevice9* device);

private:
    void FixedUpdate(double fixedDeltaTime);
    void Update(double DeltaTime);
    void Render(double DeltaTime);
	void RenderEditorScene();
    void InitPhysics(); // Helper to setup Bullet   
    void ShutdownPhysics();
    void InitGUI(); // Helper to setup Bullet   
    void RenderGUI();
    void OnLostDevice();
	void OnResetDevice();
    void ProcessGameInput(double DeltaTime);
	void ProcessEditorInput(double DeltaTime);
    void CheckCollisions(btDiscreteDynamicsWorld* dynamicsWorld);
	void SetRenderStateDefaults();
    bool RaycastFromMouse(int mouseX, int mouseY, btVector3& outHitPoint, btRigidBody*& outBody);
    PhysicsData* RaycastWithMask(btDynamicsWorld* world, btVector3 start, btVector3 end, int rayGroup, int rayMask);

    void RenderObjectProperties();
    void UpdateWalls(double DeltaTime);

    // Helper functions
    void InitBagSystem();
    bool PullFromDeck();              // Draws the next card
    PowerupType PickWeightedType();  // Decides WHICH powerup it is
};