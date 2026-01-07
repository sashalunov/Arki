#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <omp.h>

struct SmoothKey
{
    float x, y, z;
    float nx, ny, nz;

    // Comparison operator needed for std::map
    bool operator<(const SmoothKey& o) const
    {
        const float EPSILON = 0.001f;
        // Compare Position
        if (fabs(x - o.x) > EPSILON) return x < o.x;
        if (fabs(y - o.y) > EPSILON) return y < o.y;
        if (fabs(z - o.z) > EPSILON) return z < o.z;

        // Compare Normal (Round to 1 decimal place to group similar angles)
        // This acts as a "Smoothing Threshold"
        if (fabs(nx - o.nx) > 0.1f) return nx < o.nx;
        if (fabs(ny - o.ny) > 0.1f) return ny < o.ny;
        return nz < o.nz;
    }
};

struct ColorAccumulator
{
    float r = 0, g = 0, b = 0;
    int count = 0;

    void Add(const D3DXCOLOR& c) {
        r += c.r; g += c.g; b += c.b;
        count++;
    }

    D3DCOLOR GetAverage() const {
        if (count == 0) return 0xFF000000;
        return D3DCOLOR_COLORVALUE(r / count, g / count, b / count, 1.0f);
    }
};

// 1. Define a simple struct to avoid D3DX overhead in the loop
struct Point { float x, y, z; };

// 2. Inline this helper to calculate distance from plane
// Plane equation: Ax + By + Cz + D = 0
inline float GetSignedDistance(const Point& p, const D3DXPLANE& plane) {
    return (plane.a * p.x) + (plane.b * p.y) + (plane.c * p.z) + plane.d;
}

// 3. The Main Check
// Returns true ONLY if an intersection occurs, and outputs the split point.
inline bool GetSplitPoint(const Point& p1, const Point& p2, const D3DXPLANE& plane, Point& outIntersection)
{
    float d1 = GetSignedDistance(p1, plane);
    float d2 = GetSignedDistance(p2, plane);
    // OPTIMIZATION: 
    // If both have the same sign (both > 0 or both < 0), the line does not cross the plane.
    // We avoid the expensive division entirely.
    if (d1 * d2 > 0.0f) {
        return false;
    }
    // Exact zero checks (optional, depends on your epsilon needs for "on plane")
    // If one is 0, the vertex touches the plane, usually handled as "no split" or special case.
    // ---------------------------------------------------------
    // If we get here, they are on opposite sides. Calculate intersection.
    // T = d1 / (d1 - d2)
    // ---------------------------------------------------------
    float t = d1 / (d1 - d2);

    outIntersection.x = p1.x + (p2.x - p1.x) * t;
    outIntersection.y = p1.y + (p2.y - p1.y) * t;
    outIntersection.z = p1.z + (p2.z - p1.z) * t;

    return true;
}

// Helper: Convert HSL to RGB (or just make a simple gradient)
inline D3DXCOLOR GetColorForDepth(int depth)
{
    // Simple Gradient: Red (Top) -> Green (Middle) -> Blue (Deep)
    float r = 0.0f, g = 0.0f, b = 0.0f;

    if (depth < 5) {
        // Red to Yellow
        r = 1.0f;
        g = depth / 5.0f;
    }
    else if (depth < 10) {
        // Yellow to Green
        r = 1.0f - ((depth - 5) / 5.0f);
        g = 1.0f;
    }
    else {
        // Green to Blue
        g = 1.0f - ((depth - 10) / 10.0f);
        if (g < 0) g = 0;
        b = (depth - 10) / 10.0f;
        if (b > 1.0f) b = 1.0f;
    }
    return D3DXCOLOR(r, g, b, 1.0f);
}

// Helper: Random color based on Node Index
inline D3DXCOLOR GetRandomColor(int index)
{
    // Use the index as a seed to get consistent random colors
    srand(index * 12345);
    float r = (rand() % 255) / 255.0f;
    float g = (rand() % 255) / 255.0f;
    float b = (rand() % 255) / 255.0f;
    return D3DXCOLOR(r, g, b, 1.0f);
}

// Fast "Shadow Ray" Intersection (Möller–Trumbore)
// Returns TRUE if we hit something strictly within range [0, maxDist]
inline bool IntersectTriangleShadow(
    const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float maxDist,
    const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
{
    const float EPSILON = 0.000001f;
    D3DXVECTOR3 edge1 = v1 - v0;
    D3DXVECTOR3 edge2 = v2 - v0;
    D3DXVECTOR3 pvec;
    D3DXVec3Cross(&pvec, &dir, &edge2);
    float det = D3DXVec3Dot(&edge1, &pvec);

    // Double-sided check: if det is close to 0, ray is parallel
    if (det > -EPSILON && det < EPSILON) return false;
    float invDet = 1.0f / det;

    D3DXVECTOR3 tvec = orig - v0;
    float u = D3DXVec3Dot(&tvec, &pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    D3DXVECTOR3 qvec;
    D3DXVec3Cross(&qvec, &tvec, &edge1);
    float v = D3DXVec3Dot(&dir, &qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = D3DXVec3Dot(&edge2, &qvec) * invDet;

    // Check strict distance range
    return (t > EPSILON && t < maxDist);
}

enum eBuildState
{
    BS_IDLE,
    BS_BUILDING_BSP,
    BS_CALC_RAD,
    BS_READY
};

enum eSide
{
    S_FRONT,
    S_BACK,
    S_COPLANAR,
    S_SPLIT
};
struct BSPMaterial
{
    D3DXCOLOR diffuse;  // Kd
    D3DXCOLOR emissive; // Ke
    float     power;    // Optional intensity multiplier
};
struct OBJVertex 
{
    float x, y, z;
    float nx, ny, nz;
    DWORD color;
    float u, v;
};
#define FVF_OBJVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX0)

//struct BSPVertex {
//    float x, y, z;
//    float nx, ny, nz;
//    float u, v;
//};
struct BSPTriangle 
{
    OBJVertex v[3]; // The 3 corners of the triangle
    int matIndex;
    // Helper: Compute the plane defined by this triangle
    D3DXPLANE GetPlane() const 
    {
        D3DXPLANE plane;
        D3DXVECTOR3 p0(v[0].x, v[0].y, v[0].z);
        D3DXVECTOR3 p1(v[1].x, v[1].y, v[1].z);
        D3DXVECTOR3 p2(v[2].x, v[2].y, v[2].z);

        D3DXPlaneFromPoints(&plane, &p0, &p1, &p2);
        return plane;
    }
};
// Use indices instead of pointers
struct BSPNode 
{
    int iFront = -1; // -1 means NULL (no child)
    int iBack = -1;
    D3DXPLANE plane;
    // Polygons that are on this plane (or if it's a leaf, all polys in this volume)
    std::vector<BSPTriangle> members;
    bool isLeaf = false;
};
// -----------------------------------------------------------------------
// RADIOSITY STRUCTURES
// -----------------------------------------------------------------------
struct RADPATCH
{
    // Geometry
    D3DXVECTOR3 center;
    D3DXVECTOR3 normal;
    float       area;

    // Material (0.0 to 1.0)
    D3DXCOLOR   reflectivity; // Color of the wall (Diffuse)
    D3DXCOLOR   emission;     // Self-illumination (Light bulbs)

    // Lighting Data
    D3DXVECTOR3 accumulated;  // Total light collected so far (B)
    D3DXVECTOR3 unshot;       // Light waiting to be shot (dB)

    // LINKING: Where does this patch live in the BSP Tree?
    int nodeIndex; // Index in nodePool
    int triIndex;  // Index in nodePool[nodeIndex].members vector
};


class CBSPlevel
{
private:
    const float MAX_EDGE_SQ = 100.0f * 100.0f;
    const float MIN_EDGE_LENGTH_SQ = 10.0f * 10.0f;

    IDirect3DVertexBuffer9* m_pMeshVB;
    int m_iNumTriangles;

    std::vector<OBJVertex> mObjVertices;
    std::vector<unsigned long> mObjIndices;
    std::vector<BSPMaterial> m_materials; // Stores all loaded materials
    // Fast & Cache Friendly
    std::vector<BSPNode> nodePool;
    std::vector<BSPTriangle> m_triangles; // Store this for BSP building
    std::vector<BSPTriangle> m_subd_triangles;
    std::vector<RADPATCH> m_patches;
    // Optimization: Precomputed Random Directions
    std::vector<D3DXVECTOR3> m_randomDirTable;

	BOOL BuildTree(UINT nodeIndex, std::vector<BSPTriangle>& polys);
    //void ExtractTriangles();
    eSide ClassifyTriangle(const BSPTriangle& tri, const D3DXPLANE& plane);
    UINT ChooseSplitter(std::vector<BSPTriangle>& polys);
    void Split(const D3DXPLANE& plane, const BSPTriangle& inTri,
        std::vector<BSPTriangle>& outFront,
        std::vector<BSPTriangle>& outBack);

    int AllocateNode();
    OBJVertex LerpVertex(const OBJVertex& v1, const OBJVertex& v2, float t);
    void RenderNodeGeometry(IDirect3DDevice9* device, const BSPNode& node);
    void RenderBSP(IDirect3DDevice9* device, int nodeIndex, const D3DXVECTOR3& cameraPos, int depth, int debugMode);
    BOOL RadIteration();
    // RAD Helpers
    float CalculateArea(const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2);
    float CalculateFormFactor(const RADPATCH& src, const RADPATCH& dest);
    int   FindBrightestPatch();
    // RAD Main Functions
    void  PrepareRadiosity(); // Call this AFTER LoadOBJ
    BOOL  RunRadiosityIteration(); // Call this in a loop (e.g., 100 times)
    void  ApplyRadiosityToMesh(); // Bake colors to Vertex Buffer
    void SubdivideGeometry();

    // New Helper: Recursive BSP Raycast
    bool CheckNodeVisibility(int nodeIndex, const D3DXVECTOR3& start, const D3DXVECTOR3& end);
    // Update the main function to use the tree
    bool RayCastAny(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float length);
    void AddHemisphereLight(const D3DXCOLOR& skyColor, float intensity, int numSamples);
    D3DXVECTOR3 GetRandomHemisphereVector(const D3DXVECTOR3& normal);
    BOOL IntersectTriDoubleSided(
        const D3DXVECTOR3& orig, const D3DXVECTOR3& dir,
        const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2,
        float& dist, float& u, float& v);

    // --- THREADING MEMBERS ---
    std::thread m_workerThread;
    std::atomic<eBuildState> m_eState;
    std::atomic<float> m_fProgress; // 0.0f to 1.0f
    std::atomic<bool> m_bStopRequested;
    // Helper function that runs inside the new thread
    void ThreadWorker();

public:
    CBSPlevel();
    ~CBSPlevel();

	BOOL LoadOBJ(const std::string);
	void BuildBSP();
    void BuildRAD();
    void StartBackgroundBuild();

    // Check this in your Main Loop to draw a progress bar
    float GetProgress() const { return m_fProgress; }
    eBuildState GetState() const { return m_eState; }

	void Render(IDirect3DDevice9* device, const D3DXVECTOR3& cameraPos);
};

