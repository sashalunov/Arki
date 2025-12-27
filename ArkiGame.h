#pragma once
#include <btBulletDynamicsCommon.h>
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

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,    
    STATE_EDITOR,    
    STATE_SETTINGS,
    STATE_LEVEL_COMPLETE
};

// Helper: Opens a Windows file picker (Wide String Version)
std::wstring OpenFileDialog( HWND owner );
// Helper: Opens a Windows "Save As" file picker (Wide String Version)
std::wstring SaveFileDialog( HWND owner );


class ArkiGame
{
public:
    bool m_isPaused;
	bool m_isEditorMode;
    float scrollAmount;
private:
    CMainMenu* m_mainMenu;
    GameState m_gameState;
	CGrid* m_grid;
    CTimer m_Timer;
    CSkybox* m_pSkybox; // Pointer allows it to be NULL if level has no sky
    CQuatCamera* m_pCam0;
	COrbitCamera* m_pCamEditor;
    CXMesh* m_pBodyMesh;
    CXMesh* m_pSkullMesh;
    CXMesh* m_pTeapotMesh;
	// rigid bodies
    //CRigidBody* m_ball;
    CRigidBody* m_floor;    CRigidBody* m_top;
    CRigidBody* m_wallL;    CRigidBody* m_wallR;

    std::vector<CRigidBody*> m_sceneObjects;
    CRigidBody* g_selected;
    CGizmo* g_gizmo;
    // powerups
    std::vector<CArkiPowerup*> m_powerups;
    // particle systems
    ParticleEmitter* emmiter1;
    BoidEmitter* eb;
	SphereEmitter* es;

    long mouseDeltaX;
    long mouseDeltaY;
    double fFrameTime;
    double fDeltaTime;

    // --- Fixed Update Constants ---
    const double FIXED_DT = 1.0 / 60.0;    // Target 60 updates per second (0.0166s)
    const double MAX_FRAME_TIME = 0.25;    // Cap time to prevent "spiral of death" if game lags
    double g_accumulator = 0.0;            // Stores accumulated time


    D3DLIGHT9 m_LightDefault;
	D3DMATERIAL9 m_MaterialDefault;

    // bullet physics
    btBroadphaseInterface* g_broadphase;
    btDefaultCollisionConfiguration* g_collisionConfiguration;
    btCollisionDispatcher* g_dispatcher;
    btSequentialImpulseConstraintSolver* g_solver;
    btDiscreteDynamicsWorld* g_dynamicsWorld;
    btAlignedObjectArray<btCollisionShape*> collisionShapes;
    CBulletDebugDrawer* btDebugDrawer;
    bool m_debugdraw = TRUE;
    bool m_usemouse = FALSE;
    bool m_usesnap = FALSE;
    float m_val = 0.0f;
    bool m_inputLeft;
	bool m_inputRight;

    //arkanoid game
    CArkiLevel* m_currentLevel;
    // Initialization
    CArkiBall* m_ball;// = new CArkiBall(g_DynamicsWorld, D3DXVECTOR3(0, -8, 0), 0.5f, 25.0f);
    bool isBallActive = FALSE;
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
    void RenderObjectProperties();
};