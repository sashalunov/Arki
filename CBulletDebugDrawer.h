#pragma once


#include <btBulletDynamicsCommon.h>

// Custom Vertex Format for our lines (Position + Color)
struct DebugVertex {
    float x, y, z;
    DWORD color;

    // FVF (Flexible Vertex Format) code
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};

class CBulletDebugDrawer: public btIDebugDraw
{
private:
    LPDIRECT3DDEVICE9 m_pDevice;
    int m_debugMode;

    // The buffer to hold lines before we draw them
    std::vector<DebugVertex> m_lineBuffer;

public:
    CBulletDebugDrawer(LPDIRECT3DDEVICE9 device);

    // --- Core btIDebugDraw Methods ---
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
    virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);

    // --- Boilerplate Methods (Required by Bullet) ---
    virtual void reportErrorWarning(const char* warningString);
    virtual void draw3dText(const btVector3& location, const char* textString);
    virtual void setDebugMode(int debugMode);
    virtual int  getDebugMode() const;

    // --- Our Custom Method ---
    // Call this at the end of your render loop to actually put pixels on screen
    void Draw();
};