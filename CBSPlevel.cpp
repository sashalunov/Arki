#include "stdafx.h"
#include "Logger.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "CBSPlevel.h"

#ifndef FtoDW
#define FtoDW(f) (*(DWORD*)&(f))
#endif



CBSPlevel::CBSPlevel()
{
    nodePool.clear();
	nodePool.reserve(500000);
	m_pMeshVB = NULL;
    // Initialize Threading Vars
    m_eState = BS_IDLE;
    m_fProgress = 0.0f;
    m_bStopRequested = false;
    m_offset = btVector3(0, 0, 0);
	m_pLevelObject = nullptr;
	m_pdworld = nullptr;
	m_pTriangleMesh = nullptr;
	m_pCollisionShape = nullptr;

}

CBSPlevel::~CBSPlevel()
{
	SAFE_RELEASE(m_pMeshVB);

	CleanupPhysics();
    // Signal thread to stop
    m_bStopRequested = true;
    // Wait for thread to finish if it's running
    if (m_workerThread.joinable()) 
    {
        m_workerThread.join();
    }
}
void CBSPlevel::StartBackgroundBuild()
{
    if (m_eState != BS_IDLE && m_eState != BS_READY) 
    {
        _log(L"Build already in progress!\n");
        return;
    }
    // Reset State
    m_fProgress = 0.0f;
    m_bStopRequested = false;
    // If a thread is already finished but not joined, join it now
    if (m_workerThread.joinable()) 
    {
        m_workerThread.join();
    }

    // Launch the worker
    m_workerThread = std::thread(&CBSPlevel::ThreadWorker, this);
}
BOOL CBSPlevel::LoadOBJ(btDynamicsWorld* dynamicsWorld, const std::string filename)
{
    // Define your scale factor
    float importScale = 0.01f;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
	if (!warn.empty()) {
		_log(L"WARNING: %S\n", warn.c_str());
	}
	if (!err.empty()) {
		_log(L"ERROR: %S\n", err.c_str());
	}
	if (!ret) {
		_log(L"Failed to load/parse .obj file: %s\n", filename.c_str());
		return FALSE;
	}
	_log(L"Loaded .obj file: %hs \n",filename.c_str());
	_log(L"Vertices: %d \n", (int)(attrib.vertices.size() / 3));
	_log(L"Shapes: %d \n", (int)shapes.size());
	_log(L"Materials: %d \n", (int)materials.size());
    // -------------------------------------------------------
    // 1. CONVERT MATERIALS
    // -------------------------------------------------------
    m_materials.clear();
    for (const auto& mat : materials)
    {
        BSPMaterial bspMat;
        // Diffuse (Kd)
        bspMat.diffuse = D3DXCOLOR(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f);
        // Emissive (Ke) - THE KEY PART
        // We multiply by a scalar (e.g., 5.0f) because raw 1.0 is often too dim for global illumination
        float intensity = 1.0f;
        bspMat.emissive = D3DXCOLOR(mat.ambient[0] * intensity, mat.ambient[1] * intensity, mat.ambient[2] * intensity, 1.0f);
        m_materials.push_back(bspMat);
    }
    // Default material (Index 0) if none exist
    if (m_materials.empty()) {
        BSPMaterial def;
        def.diffuse = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
        def.emissive = D3DXCOLOR(0, 0, 0, 0);
        m_materials.push_back(def);
    }
	mObjVertices.clear();
    m_triangles.clear();
	m_triangles_temp.clear();
    // Loop over shapes (the file might contain multiple objects)
    for (const auto& shape : shapes) 
    {
        size_t index_offset = 0;
        // Loop over FACES, not just vertices
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            int fv = shape.mesh.num_face_vertices[f]; // Should be 3 (triangulated)

            // Get Material ID for this face
            int matId = -1;
            if (f < shape.mesh.material_ids.size())
                matId = shape.mesh.material_ids[f];

            // Handle negative/missing IDs
            if (matId < 0 || matId >= m_materials.size()) matId = 0;

            BSPTriangle tri;
            tri.matIndex = matId; // <--- Store it!

            // Loop over the 3 vertices of this face
            for (size_t v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                OBJVertex vert;
                // ... [Copy Position, Normal, UV as before] ...
                vert.x = attrib.vertices[3 * idx.vertex_index + 0] * importScale;
                vert.y = attrib.vertices[3 * idx.vertex_index + 1] * importScale;
                vert.z = attrib.vertices[3 * idx.vertex_index + 2] * importScale;

                if (idx.normal_index >= 0) {
                    vert.nx = attrib.normals[3 * idx.normal_index + 0];
                    vert.ny = attrib.normals[3 * idx.normal_index + 1];
                    vert.nz = attrib.normals[3 * idx.normal_index + 2];
                }
                else { vert.nx = 0; vert.ny = 1; vert.nz = 0; }

                if (idx.texcoord_index >= 0) {
                    vert.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vert.v = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
                }
                else { vert.u = 0; vert.v = 0; }

                vert.color = 0xFFFFFFFF; // Base white

                // Store in our temporary triangle
                tri.v[v] = vert;

                // Keep the flat list for legacy render if needed
                mObjVertices.push_back(vert);
            }

            m_triangles.push_back(tri);
            index_offset += fv;
        }
    }

	_log(L"Total Triangles Loaded: %d \n", (int)m_triangles.size());
	InitPhysics(dynamicsWorld);
	return TRUE;
}

//void CBSPlevel::ExtractTriangles()
//{
//    m_triangles.clear();
//    int triCount = (int)mObjVertices.size() / 3;
//    m_triangles.reserve(triCount);
//
//    for (int i = 0; i < triCount; i++)
//    {
//        BSPTriangle tri;
//        // Copy 3 vertices at a time
//        tri.v[0] = mObjVertices[i * 3 + 0];
//        tri.v[1] = mObjVertices[i * 3 + 1];
//        tri.v[2] = mObjVertices[i * 3 + 2];
//
//        m_triangles.push_back(tri);
//    }
//    _log(L"Triangles: %d \n", (int)m_triangles.size());
//
//}

void CBSPlevel::SubdivideGeometry(std::vector<BSPTriangle>& tris)
{
    // We will build a new list of triangles
	m_subd_triangles.clear();
    m_subd_triangles.reserve(tris.size() * 4);

    int splitCount = 0;

    for (const auto& tri : tris)
    {
        D3DXVECTOR3 p0(tri.v[0].x, tri.v[0].y, tri.v[0].z);
        D3DXVECTOR3 p1(tri.v[1].x, tri.v[1].y, tri.v[1].z);
        D3DXVECTOR3 p2(tri.v[2].x, tri.v[2].y, tri.v[2].z);

        // Check edge lengths
        float d1 = D3DXVec3LengthSq(&(p0 - p1));
        float d2 = D3DXVec3LengthSq(&(p1 - p2));
        float d3 = D3DXVec3LengthSq(&(p2 - p0));

        // If any edge is too long, we subdivide into 4
        if ((d1 > MAX_EDGE_SQ || d2 > MAX_EDGE_SQ || d3 > MAX_EDGE_SQ) && (d1 > MIN_EDGE_LENGTH_SQ && d2 > MIN_EDGE_LENGTH_SQ && d3 > MIN_EDGE_LENGTH_SQ))
        {
            // Calculate Midpoints
            OBJVertex m01 = LerpVertex(tri.v[0], tri.v[1], 0.5f);
            OBJVertex m12 = LerpVertex(tri.v[1], tri.v[2], 0.5f);
            OBJVertex m20 = LerpVertex(tri.v[2], tri.v[0], 0.5f);

            // Push 4 new triangles (Triangle 1, 2, 3, 4)
            // 1. Top
            BSPTriangle t1; t1.v[0] = tri.v[0]; t1.v[1] = m01; t1.v[2] = m20; t1.matIndex = tri.matIndex;
            m_subd_triangles.push_back(t1);

            // 2. Left
            BSPTriangle t2; t2.v[0] = m01; t2.v[1] = tri.v[1]; t2.v[2] = m12; t2.matIndex = tri.matIndex;
            m_subd_triangles.push_back(t2);

            // 3. Right
            BSPTriangle t3; t3.v[0] = m20; t3.v[1] = m12; t3.v[2] = tri.v[2]; t3.matIndex = tri.matIndex;
            m_subd_triangles.push_back(t3);

            // 4. Center
            BSPTriangle t4; t4.v[0] = m01; t4.v[1] = m12; t4.v[2] = m20; t4.matIndex = tri.matIndex;
            m_subd_triangles.push_back(t4);

            splitCount++;
        }
        else
        {
            // Small enough, keep original
            m_subd_triangles.push_back(tri);
        }
    }

    // Replace old list
    m_triangles_temp = m_subd_triangles;

    // Optimization: If we split anything, we might need to check again 
    // (Recursive approach is better, but this simple pass works for now)
   // Safety: Limit recursion depth to prevent crashes
    static int depth = 0;
    if (splitCount > 0 && depth < 64)
    {
        depth++;
        SubdivideGeometry(m_triangles_temp);
        depth--;
    }
}

// Reuse your existing eSide enum
eSide  CBSPlevel::ClassifyTriangle(const BSPTriangle& tri, const D3DXPLANE& plane)
{
    int front = 0;
    int back = 0;
    int onPlane = 0;

    // Check all 3 vertices
    for (int i = 0; i < 3; i++) 
    {
        Point p = { tri.v[i].x, tri.v[i].y, tri.v[i].z };
        // Use your existing helper
        float dist = GetSignedDistance(p, plane);

        if (dist > 0.001f)       front++;
        else if (dist < -0.001f) back++;
        else                     onPlane++;
    }

    // Logic to determine the fate of the WHOLE triangle
    if (onPlane == 3)             return S_COPLANAR;
    if (back == 0)                return S_FRONT; // All front or on plane
    if (front == 0)               return S_BACK;  // All back or on plane

    return S_SPLIT; // Some front, some back -> Needs Cutting!
}

void CBSPlevel::Render(IDirect3DDevice9* device, const D3DXVECTOR3& cameraPos)
{
   

    // 1. Check State
    eBuildState currentState = m_eState; // Atomic load
    if (currentState != BS_READY)
    {
        // Debug Color based on state
        D3DXCOLOR debugColor = (currentState == BS_BUILDING_BSP) ? D3DXCOLOR(1, 0, 0, 1) : D3DXCOLOR(1, 1, 0, 1);
        // Only draw points if we have them
        if (!mObjVertices.empty()) {
            // ... [Insert your simple point drawing code here] ...
            // You can use DrawPrimitiveUP with mObjVertices
            //device->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
            //device->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&ptSize));

            //device->DrawPrimitiveUP(D3DPT_POINTLIST, (int)mObjVertices.size(),
			//	mObjVertices.data(), sizeof(OBJVertex));
               

        }
        //return;
    }
    // Simple rendering of the loaded OBJ vertices as a point cloud
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_COLORVERTEX, TRUE);

    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW); // Cull back faces
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE); // 'DIFFUSE' here means Vertex Color
   
    if (!nodePool.empty()) 
    {
        RenderBSP(device, 0, cameraPos, 0, 2);
    }
}

void CBSPlevel::RenderBSP(IDirect3DDevice9* device, int nodeIndex, const D3DXVECTOR3& cameraPos, int depth, int debugMode)
{
    // 1. Safety Check
    if (nodeIndex == -1 || nodeIndex >= nodePool.size()) return;

    BSPNode& node = nodePool[nodeIndex];

    // -----------------------------------------------------
    // VISUALIZATION LOGIC
    // -----------------------------------------------------
    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));

    D3DXCOLOR debugColor;

    
    if (debugMode == 1) // DEPTH MODE
    {
        debugColor = GetColorForDepth(depth);
    }
    else if (debugMode == 2) // LEAF MODE
    {
        if (node.isLeaf)
            debugColor = GetRandomColor(nodeIndex);
        else
            debugColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f); // Grey for branches
    }

    // Set Emissive so it glows without light sources
    mtrl.Emissive = debugColor;
    mtrl.Diffuse = debugColor;
    //device->SetMaterial(&mtrl);
	device->SetTexture(0, NULL); // Disable texture

    // 2. Leaf Node Case
    // If it's a leaf, it has no children. Just draw it.
    if (node.isLeaf)
    {
        RenderNodeGeometry(device, node);
        return;
    }

    // 3. Node Case: Determine Camera Position relative to this node's plane
    // We reuse your math helper logic here
    float dist = node.plane.a * cameraPos.x +
        node.plane.b * cameraPos.y +
        node.plane.c * cameraPos.z +
        node.plane.d;

    if (dist > 0) // Camera is IN FRONT of the plane
    {
        // Draw Order: Back Child -> Node Geometry -> Front Child
        RenderBSP(device, node.iBack, cameraPos, depth + 1, debugMode);      // Draw far stuff
        RenderNodeGeometry(device, node);              // Draw this wall
        RenderBSP(device, node.iFront, cameraPos, depth + 1, debugMode);     // Draw near stuff
    }
    else // Camera is BEHIND the plane
    {
        // Draw Order: Front Child -> Node Geometry -> Back Child
        RenderBSP(device, node.iFront, cameraPos, depth + 1, debugMode);     // Draw far stuff
        RenderNodeGeometry(device, node);              // Draw this wall
        RenderBSP(device, node.iBack, cameraPos, depth + 1, debugMode);      // Draw near stuff
    }
}
// Helper to actually issue the Draw command
void CBSPlevel::RenderNodeGeometry(IDirect3DDevice9* device, const BSPNode& node)
{
    if (node.members.empty()) return;

    // 1. Create a temporary buffer for PURE vertex data
    // Static vector prevents re-allocating memory every single frame (Optimization)
    static std::vector<OBJVertex> renderBuffer;
    renderBuffer.clear();
    renderBuffer.reserve(node.members.size() * 3);

    // 2. Extract only the vertices, skipping the 'matIndex' integer
    for (const auto& tri : node.members)
    {
		OBJVertex v0 = tri.v[0];
        OBJVertex v1 = tri.v[1];
        OBJVertex v2 = tri.v[2];
		// apply offset
		v0.x += (FLOAT)m_offset.getX();
        v0.y += (FLOAT)m_offset.getY();
        v0.z += (FLOAT)m_offset.getZ();
        v1.x += (FLOAT)m_offset.getX();
        v1.y += (FLOAT)m_offset.getY();
        v1.z += (FLOAT)m_offset.getZ();
        v2.x += (FLOAT)m_offset.getX();
        v2.y += (FLOAT)m_offset.getY();
        v2.z += (FLOAT)m_offset.getZ();

        renderBuffer.push_back(v0);
        renderBuffer.push_back(v1);
        renderBuffer.push_back(v2);
    }
    if (bPointsDraw) {
        device->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
        device->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&ptSize));

        device->DrawPrimitiveUP(D3DPT_POINTLIST, (UINT)node.members.size()*3,
            renderBuffer.data(), sizeof(OBJVertex));

    }
    else {
        // Because 'BSPTriangle' is just 3 'OBJVertex' structs back-to-back,
        // the memory layout is identical to a standard Vertex Buffer.
        // We can cast the pointer directly.
        device->SetFVF(FVF_OBJVERTEX);
        // DrawPrimitiveUP is slow for final games, but perfect for this stage.
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
            (UINT)node.members.size(),        // Primitive Count (Triangles)
            renderBuffer.data(),        // Pointer to vertex data
            sizeof(OBJVertex));         // Stride
    }
}

UINT CBSPlevel::ChooseSplitter(std::vector<BSPTriangle>& polys)
{
	UINT nBestScore = 0xffffffff;
	UINT nBestIndex = 0;
	UINT nSplitMul = 8;
	UINT uTestCount = 1;

	for (UINT i = 0; i < polys.size(); i += uTestCount)
	{
		UINT nFront = 0;
		UINT nBack = 0;
		UINT nSplits = 0;
		UINT nScore = 0;

		//if (polys[i].bUsed)
		//	continue;

		D3DXPLANE plane = polys[i].GetPlane();

		for (UINT j = 0; j < polys.size(); j++)
		{
			if (i == j)continue;

			eSide res = ClassifyTriangle(polys[j], plane);

			switch (res)
			{
			case S_FRONT:
				nFront += 1;
				break;

			case S_BACK:
				nBack += 1;
				break;

			case S_SPLIT:
				nSplits += 1;
				nFront += 1;
				nBack += 1;
				break;
			}
		}

		//if (nBack < 1)
		//	continue;

		nScore = abs((int)nFront - (int)nBack) + (nSplits * nSplitMul);
		//_log("%d: Front %d, Back %d, Coplanar %d, Splits %d   Score %d\r\n", i, nFront, nBack, nCoplanar, nSplits, nScore);

		if (nScore < nBestScore)
		{
			nBestScore = nScore;
			nBestIndex = i;
            if (nScore < 5)break;
		}
	}
	//_log(L"Best splitter: Index %d, Score %d\r", nBestIndex, nBestScore);

	return nBestIndex;
}
// Helper: Linear Interpolation between two vertices
OBJVertex CBSPlevel::LerpVertex(const OBJVertex& v1, const OBJVertex& v2, float t)
{
    OBJVertex out;
    // 1. Position
    out.x = v1.x + (v2.x - v1.x) * t;
    out.y = v1.y + (v2.y - v1.y) * t;
    out.z = v1.z + (v2.z - v1.z) * t;

    // 2. Normal (Normalize after lerp is technically better, but lerp is fine for BSP)
    //out.nx = v1.nx + (v2.nx - v1.nx) * t;
    //out.ny = v1.ny + (v2.ny - v1.ny) * t;
    //out.nz = v1.nz + (v2.nz - v1.nz) * t;
    D3DXVECTOR3 n;
    n.x = v1.nx + v2.nx;
    n.y = v1.ny + v2.ny;
    n.z = v1.nz + v2.nz;
    float lenSq = D3DXVec3LengthSq(&n);
    //// 2. Only normalize if length is valid (not close to zero)
    if (lenSq > 0.1f)
    {
        D3DXVec3Normalize(&n, &n);
    }
    else
    {
        n = D3DXVECTOR3(v1.nx, v1.ny, v1.nz);
    }
    out.nx = n.x; 
    out.ny = n.y; 
    out.nz = n.z;

    D3DXCOLOR cout(1,1,1,1);
    D3DXColorLerp(&cout,
        (D3DXCOLOR*)&v1.color,
        (D3DXCOLOR*)&v2.color,
        t);
	out.color = *((DWORD*)&cout);
    // 3. UVs
    out.u = v1.u + (v2.u - v1.u) * t;
    out.v = v1.v + (v2.v - v1.v) * t;

    return out;
}

// Helper to add a fresh node to the pool and return its index
int CBSPlevel::AllocateNode()
{
    if (m_bStopRequested) return -1; // Fail gracefully

    BSPNode newNode;
    newNode.iFront = -1;
    newNode.iBack = -1;
    newNode.isLeaf = false;
    // Important: Reserve space in member vector to avoid small re-allocations
    newNode.members.reserve(4);
    nodePool.push_back(newNode);
    return (int)nodePool.size() - 1;
}

void CBSPlevel::Split(const D3DXPLANE& plane, const BSPTriangle& inTri, std::vector<BSPTriangle>& outFront,std::vector<BSPTriangle>& outBack)
{
    // Temporary storage for the polygon loops (could be 3 or 4 points)
    std::vector<OBJVertex> frontPoly;
    std::vector<OBJVertex> backPoly;
    // Test the 3 edges: (0->1), (1->2), (2->0)
    OBJVertex a = inTri.v[0];
    OBJVertex b = inTri.v[1];
    OBJVertex c = inTri.v[2];
    OBJVertex pts[3] = { a, b, c };
    // Calculate distances for all 3 points once
    float dists[3];
    for (int i = 0; i < 3; i++) 
    {
        dists[i] = plane.a * pts[i].x + plane.b * pts[i].y + plane.c * pts[i].z + plane.d;
    }

    // Loop through edges
    for (int i = 0; i < 3; i++)
    {
        int j = (i + 1) % 3; // Next vertex index

        OBJVertex v1 = pts[i];
        OBJVertex v2 = pts[j];
        float d1 = dists[i];
        float d2 = dists[j];

        // 1. Put current vertex in the correct list
        if (d1 >= 0) frontPoly.push_back(v1);
        if (d1 <= 0) backPoly.push_back(v1);

        // 2. Check for intersection (crossing the plane)
        // If signs are different (one pos, one neg)
        if (d1 * d2 < 0)
        {
            // Calculate T (0.0 to 1.0)
            float t = d1 / (d1 - d2);

            // Create the split vertex
            OBJVertex vMid = LerpVertex(v1, v2, t);

            // Add to BOTH lists (it's the hinge)
            frontPoly.push_back(vMid);
            backPoly.push_back(vMid);
        }
    }
    // ---------------------------------------------------------
    // Triangulate the resulting polygons (Turn Quads into Triangles)
    // ---------------------------------------------------------
    // Front Side
    if (frontPoly.size() >= 3) {
        // Fan Triangulation: Connect vertex 0 to i and i+1
        for (size_t i = 1; i < frontPoly.size() - 1; i++) {
            BSPTriangle newTri;
			newTri.matIndex = inTri.matIndex;
            newTri.v[0] = frontPoly[0];
            newTri.v[1] = frontPoly[i];
            newTri.v[2] = frontPoly[i + 1];
            outFront.push_back(newTri);
        }
    }

    // Back Side
    if (backPoly.size() >= 3) {
        for (size_t i = 1; i < backPoly.size() - 1; i++) {
            BSPTriangle newTri;
            newTri.matIndex = inTri.matIndex;
            newTri.v[0] = backPoly[0];
            newTri.v[1] = backPoly[i];
            newTri.v[2] = backPoly[i + 1];
            outBack.push_back(newTri);
        }
    }
}

BOOL CBSPlevel::BuildTree(UINT nodeIndex, std::vector<BSPTriangle>& polys)
{
    if (m_bStopRequested) return FALSE;
    // 1. STOP CONDITION: If few polys or max depth, make this a LEAF.
    // (Adjust '10' and '20' based on your needs)
    if (polys.size() <= 10 )//|| depth > 20)
    {
        nodePool[nodeIndex].isLeaf = true;
        nodePool[nodeIndex].members = polys; // Store all remaining polys here
        return TRUE;
    }

    UINT splitterIndex = ChooseSplitter(polys);
    // 3. Store the Splitter Plane in the Node
    D3DXPLANE splitPlane = polys[splitterIndex].GetPlane();
    nodePool[nodeIndex].plane = splitPlane;

    std::vector<BSPTriangle> frontList, backList;
    // We reserve memory to prevent constant resizing during push_back
    frontList.reserve(polys.size());
    backList.reserve(polys.size());

    for (UINT i = 0; i < polys.size(); i++)
    {
        eSide res = ClassifyTriangle(polys[i], polys[splitterIndex].GetPlane());

        switch (res)
        {
        case S_FRONT:
            frontList.push_back(polys[i]);
        break;

        case S_BACK:
            backList.push_back(polys[i]);
        break;

        case S_COPLANAR:
            // These form the "wall" at this split.
            nodePool[nodeIndex].members.push_back(polys[i]);
        break;

        case S_SPLIT:
        {
            std::vector<BSPTriangle> splitFront, splitBack;
            Split(splitPlane, polys[i], splitFront, splitBack);
            // Append ALL resulting triangles to the main lists
            frontList.insert(frontList.end(), splitFront.begin(), splitFront.end());
            backList.insert(backList.end(), splitBack.begin(), splitBack.end());
        }
        break;
        }
    }
    // Free memory of the source list (optional, but good for recursion depth memory usage)
    polys.clear();

    // 5. Recursion - Front
    if (frontList.size() > 0)
    {
        int newFrontIndex = AllocateNode();
        // IMPORTANT: Set link immediately via index
        nodePool[nodeIndex].iFront = newFrontIndex;
        BuildTree(newFrontIndex, frontList);
    }
    // 6. Recursion - Back
    if (backList.size() > 0)
    {
        int newBackIndex = AllocateNode();
        nodePool[nodeIndex].iBack = newBackIndex;
        BuildTree(newBackIndex, backList);
    }

    return TRUE;
}


void CBSPlevel::BuildBSP()
{
    //ExtractTriangles();
    _log(L"Building BSP Tree...\n");

	SubdivideGeometry(m_triangles);
    
	// Example: Choose a splitter for the entire set
	UINT splitterIndex = ChooseSplitter(m_subd_triangles);
	_log(L"Chosen Splitter Index: %d\n", splitterIndex);
	// Further BSP construction logic would go here...

    nodePool.clear();
    nodePool.reserve(500000);

    // Create Root Node
    int rootIndex = AllocateNode();

    // Start Recursion
    BuildTree(rootIndex, m_subd_triangles);
}

void CBSPlevel::BuildRAD()
{
    _log(L"Preparing Radiosity...\n");
    PrepareRadiosity();

    _log(L"Running Radiosity Iterations...\n");
    const int maxIterations = 256;
    for (int i = 0; i < maxIterations; i++)
    {
        if (!RunRadiosityIteration())
            break; // No more energy to distribute
        _log(L"Completed Radiosity Iteration %d\n", i + 1);
    }
    _log(L"Applying Radiosity to Mesh...\n");
    
	ApplyRadiosityToMesh();
}

// convert m_triangles into "Patches" that store light energy.
float CBSPlevel::CalculateArea(const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
{
    D3DXVECTOR3 edge1 = v1 - v0;
    D3DXVECTOR3 edge2 = v2 - v0;
    D3DXVECTOR3 cross;
    D3DXVec3Cross(&cross, &edge1, &edge2);
    return D3DXVec3Length(&cross) * 0.5f;
}

float CBSPlevel::CalculateFormFactor(const RADPATCH& src, const RADPATCH& dest)
{
    // 1. Vector Setup
    D3DXVECTOR3 vec = dest.center - src.center;
    float distSq = D3DXVec3LengthSq(&vec);
    //if (distSq > 100.0f) return 0.0f;

    float dist = sqrtf(distSq);
    // Safety: Don't calculate if patches are on top of each other
    //if (dist < 0.001f) return 0.0f;

    // Normalize direction vector (Source -> Dest)
    D3DXVECTOR3 dir = vec / dist;

    // 2. Orientation Checks (Cosine Law)
    // Angle at Shooter (Lambert's cosine law for emission)
    float cosSrc = D3DXVec3Dot(&src.normal, &dir);
    // Angle at Receiver (Lambert's cosine law for incidence)
    // We negate 'dir' because the receiver normal should point TOWARDS the shooter
    float cosDest = D3DXVec3Dot(&dest.normal, &-dir);
    // Backface Culling:
    // If Source is facing away (cosSrc < 0) or Dest is facing away (cosDest < 0),
    // light cannot physically travel between them.
    if (cosSrc <= 0.0f || cosDest <= 0.0f) return 0.0f;
    // 3. Visibility Check (The Optimization)
    // We need to check if a wall exists between Src and Dest.
    // 
    // OPTIMIZATION 2: Calculate Potential Factor BEFORE Raycast
    // Formula: (cos1 * cos2 * Area) / (PI * r^2)
    float potentialFactor = (cosSrc * cosDest * dest.area) / (D3DX_PI * distSq + dest.area);
    // If the energy transfer is tiny, don't waste time checking walls
    //if (potentialFactor < 0.00001f) return 0.0f;
    // A. Start slightly off the Source surface (0.05 units) to avoid hitting itself.
    D3DXVECTOR3 startPos = src.center + (src.normal * 0.05f);

    // B. End slightly before the Dest surface (0.1 units total) to avoid hitting the destination.
    float checkLength = dist - 0.001f;

    // C. Perform the BSP Raycast
    if (checkLength > 0.0f)
    {
        // If RayCastAny returns true, it means we hit a wall -> Occluded
        if (RayCastAny(startPos, dir, checkLength))
        {
            return 0.0f;
        }
    }
    // Safety clamp
    //if (potentialFactor > 1.0f) potentialFactor = 1.0f;

    return potentialFactor;
}



int CBSPlevel::FindBrightestPatch()
{
    int bestIndex = -1;
    float maxEnergy = 0.0f;

    for (int i = 0; i < m_patches.size(); i++)
    {
        // Energy = Intensity (r+g+b)
        float energy = m_patches[i].unshot.x + m_patches[i].unshot.y + m_patches[i].unshot.z;
        if (energy > maxEnergy) {
            maxEnergy = energy;
            bestIndex = i;
        }
    }
    return bestIndex;
}

void CBSPlevel::PrepareRadiosity()
{
    m_patches.clear();
    // Reserve estimate: Assuming roughly 1 patch per triangle in the pool
    m_patches.reserve(nodePool.size() * 4);

    // 1. PRE-CALCULATE RANDOM VECTORS (Optimization)
    m_randomDirTable.clear();
    m_randomDirTable.reserve(4096);

    for (int i = 0; i < 4096; i++)
    {
        D3DXVECTOR3 v;
        // Generate uniform points on sphere correctly
        do {
            // Random float -1.0 to 1.0
            float x = ((rand() % 2000) / 1000.0f) - 1.0f;
            float y = ((rand() % 2000) / 1000.0f) - 1.0f;
            float z = ((rand() % 2000) / 1000.0f) - 1.0f;
            v = D3DXVECTOR3(x, y, z);
        } while (D3DXVec3LengthSq(&v) > 1.0f || D3DXVec3LengthSq(&v) < 0.01f); // Rejection

        D3DXVec3Normalize(&v, &v);
        m_randomDirTable.push_back(v);
    }

    // Iterate over the ENTIRE BSP Tree
    for (size_t n = 0; n < nodePool.size(); n++)
    {
        BSPNode& node = nodePool[n];
        // Skip nodes that have no geometry
        if (node.members.empty()) continue;

        // Iterate over all triangles in this node
        for (size_t t = 0; t < node.members.size(); t++)
        {
            const BSPTriangle& tri = node.members[t];
            RADPATCH patch;

            // 1. Geometry Setup
            D3DXVECTOR3 p0(tri.v[0].x, tri.v[0].y, tri.v[0].z);
            D3DXVECTOR3 p1(tri.v[1].x, tri.v[1].y, tri.v[1].z);
            D3DXVECTOR3 p2(tri.v[2].x, tri.v[2].y, tri.v[2].z);

            patch.area = CalculateArea(p0, p1, p2);
            //if (patch.area <= 0.0001f) continue; // Skip degenerate/tiny slivers

            patch.center = (p0 + p1 + p2) / 3.0f;

            // Calculate Normal
            D3DXVECTOR3 e1 = p1 - p0;
            D3DXVECTOR3 e2 = p2 - p0;
            D3DXVec3Cross(&patch.normal, &e1, &e2);
            D3DXVec3Normalize(&patch.normal, &patch.normal);
			//patch.normal = D3DXVECTOR3(tri.v[1].nx, tri.v[1].ny, tri.v[1].nz);

            // Helper Lambda to setup a patch
            auto CreatePatch = [&](D3DXVECTOR3 no, D3DXVECTOR3 co, bool isBackFace)
                {
                    RADPATCH autopatch;
                    autopatch.area = patch.area;
                    autopatch.normal = no;
                    autopatch.center = co;
                    // A. Material Logic
                    // If it's a backface, we usually assume it's NOT a light source.
                    // Simple logic: Ceiling lights (pointing down) are emissive.
                    if (isBackFace ) 
                    {
                        autopatch.emission = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
                        autopatch.reflectivity = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
                    }
                    else 
                    {
                        // LOOKUP MATERIAL
                        if (tri.matIndex >= 0 && tri.matIndex < m_materials.size())
                        {
                            const BSPMaterial& mat = m_materials[tri.matIndex];
                            autopatch.emission = mat.emissive;
                            autopatch.reflectivity = mat.diffuse;
                        }
                        else
                        {
                            autopatch.emission = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
                            autopatch.reflectivity = D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f); // White wall
                        }
                    }
                    // B. Initialize Energy
                    autopatch.accumulated = D3DXVECTOR3(autopatch.emission.r, autopatch.emission.g, autopatch.emission.b);
                    autopatch.unshot = D3DXVECTOR3(autopatch.emission.r, autopatch.emission.g, autopatch.emission.b) * autopatch.area;
                    // C. Link to BSP
                    autopatch.nodeIndex = (int)n;
                    if (isBackFace) 
                    {
                        autopatch.triIndex = -1; // Critical: Mark as Backface so ApplyRadiosityToMesh ignores it!
                    }
                    else {
                        autopatch.triIndex = (int)t;
                    }
                    return autopatch;
                };

            // -----------------------------------------------------
            // 2. CREATE FRONT PATCH
            // -----------------------------------------------------
            m_patches.push_back(CreatePatch(patch.normal, patch.center, false));
            // -----------------------------------------------------
            // 3. CREATE VIRTUAL BACK PATCH
            // -----------------------------------------------------
            // Flip Normal
            D3DXVECTOR3 backNormal = -patch.normal;
            // Offset Center BACKWARDS slightly
            // This prevents the front and back patches from effectively occupying 
            // the exact same space, which helps RayCastAny precision.
            D3DXVECTOR3 backCenter = patch.center + (backNormal * 0.01f);
            //m_patches.push_back(CreatePatch(backNormal, backCenter, true));

        }
    }
    // 2. Inject Skylight
    // Color: Light Blue
    // Intensity: 2.0f (Make it bright so it bounces nicely)
    AddHemisphereLight(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f), 4.0f, SKY_SAMPLES);
}

BOOL CBSPlevel::RunRadiosityIteration()
{
    if (m_bStopRequested) return FALSE;
    // 1. Pick the Shooter
    int shooterIdx = FindBrightestPatch();
    if (shooterIdx == -1) return FALSE; // Convergence reached

    RADPATCH& shooter = m_patches[shooterIdx];

    // If energy is tiny, stop to save time
    if ((shooter.unshot.x + shooter.unshot.y + shooter.unshot.z) < 0.001f)
        return FALSE;
    // Cache shooter values to avoid reading shared memory constantly
    D3DXVECTOR3 shooterUnshot = shooter.unshot;

    // PARALLEL LOOP
    // We cast .size() to int because OpenMP works best with signed integers
    #pragma omp parallel for schedule(dynamic)
    // 2. Shoot to everyone else
    for (int i = 0; i < m_patches.size(); i++)
    {
        if (i == shooterIdx) continue;
        RADPATCH& receiver = m_patches[i];

        float ff = CalculateFormFactor(shooter, receiver);
        if (ff <= 0.0f) continue;
        // ENERGY TRANSFER FORMULA:
        // DeltaReceived = (ShooterUnshotEnergy * FormFactor) * ReceiverReflectivity
        // Note: FormFactor here accounts for Receiver Area implicitly via Reciprocity logic used above
        D3DXVECTOR3 incidentLight = shooter.unshot * ff;

        // Apply material color (Reflectivity)
        D3DXVECTOR3 reflectedLight;
        reflectedLight.x = incidentLight.x * receiver.reflectivity.r;
        reflectedLight.y = incidentLight.y * receiver.reflectivity.g;
        reflectedLight.z = incidentLight.z * receiver.reflectivity.b;
        // 1. Convert Flux to Radiosity for the visual mesh (Divide by Area)
            // Safety check to avoid divide by zero, though unlikely with your setup
        if (receiver.area > 1e-6f)
        {
            receiver.accumulated += reflectedLight / receiver.area;
        }        // Add to unshot (so it can bounce next frame)
        receiver.unshot += reflectedLight;
    }

    // 3. Reset Shooter
    shooter.unshot = D3DXVECTOR3(0, 0, 0);

    return TRUE; // Continue iterating
}

void CBSPlevel::ApplyRadiosityToMesh()
{
    // Map to store sum of colors for every unique vertex
    std::map<SmoothKey, ColorAccumulator> smoothMap;

    // ---------------------------------------------------------
    // PASS 1: GATHER COLORS
    // ---------------------------------------------------------
    for (const auto& patch : m_patches)
    {
        if (patch.triIndex == -1) continue; // Skip backfaces
        if (patch.nodeIndex >= nodePool.size()) continue;

        BSPNode& node = nodePool[patch.nodeIndex];
        if (patch.triIndex >= node.members.size()) continue;

        // Get the triangle
        BSPTriangle& tri = node.members[patch.triIndex];

        // Calculate Tone-Mapped Color for this PATCH
        D3DXVECTOR3 finalColor = patch.accumulated;
        // Reinhard Tone Mapping
        finalColor.x = finalColor.x / (1.0f + finalColor.x);
        finalColor.y = finalColor.y / (1.0f + finalColor.y);
        finalColor.z = finalColor.z / (1.0f + finalColor.z);
        D3DXCOLOR c(finalColor.x, finalColor.y, finalColor.z, 1.0f);

        // Add this color to all 3 vertices in our map
        for (int i = 0; i < 3; i++)
        {
            SmoothKey k;
            k.x = tri.v[i].x; k.y = tri.v[i].y; k.z = tri.v[i].z;
            k.nx = tri.v[i].nx; k.ny = tri.v[i].ny; k.nz = tri.v[i].nz;

            smoothMap[k].Add(c);
        }
    }

    // ---------------------------------------------------------
    // PASS 2: APPLY AVERAGED COLORS
    // ---------------------------------------------------------
    for (auto& node : nodePool)
    {
        for (auto& tri : node.members)
        {
            for (int i = 0; i < 3; i++)
            {
                SmoothKey k;
                k.x = tri.v[i].x; k.y = tri.v[i].y; k.z = tri.v[i].z;
                k.nx = tri.v[i].nx; k.ny = tri.v[i].ny; k.nz = tri.v[i].nz;

                // Find the accumulated color in the map
                auto it = smoothMap.find(k);
                if (it != smoothMap.end())
                {
                    tri.v[i].color = it->second.GetAverage();
                }
            }
        }
    }
	// sharp shading for debugging
    //for (const auto& patch : m_patches)
    //{
    //    // 1. SKIP VIRTUAL BACK PATCHES
    //    // Since they don't exist in the real mesh, we can't color them.
    //    if (patch.triIndex == -1) continue;

    //    // 2. Retrieve the actual triangle from the BSP Tree
    //    if (patch.nodeIndex >= nodePool.size()) continue;
    //    BSPNode& node = nodePool[patch.nodeIndex];

    //    if (patch.triIndex >= node.members.size()) continue;
    //    BSPTriangle& tri = node.members[patch.triIndex];

    //    // 2. Tone Mapping (Reinhard: c / (1 + c)) to handle high dynamic range
    //    D3DXVECTOR3 finalColor = patch.accumulated;
    //    finalColor.x = finalColor.x / (1.0f + finalColor.x);
    //    finalColor.y = finalColor.y / (1.0f + finalColor.y);
    //    finalColor.z = finalColor.z / (1.0f + finalColor.z);

    //    // 3. Convert to DWORD ARGB
    //    D3DCOLOR c = D3DCOLOR_COLORVALUE(finalColor.x, finalColor.y, finalColor.z, 1.0f);
    //    // ...
    //    tri.v[0].color = c;
    //    tri.v[1].color = c;
    //    tri.v[2].color = c;
    //}
}

// --------------------------------------------------------------------------
// OPTIMIZED RAYCAST: Uses BSP Tree O(log N) instead of O(N)
// --------------------------------------------------------------------------
bool CBSPlevel::RayCastAny(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float length)
{
    if (nodePool.empty()) return false;

    D3DXVECTOR3 end = start + (dir * length);
    D3DXVECTOR3 rayDir = dir;
    // Start recursion at Root (Node 0)
    return CheckNodeVisibility(0, start, end);
}

bool CBSPlevel::CheckNodeVisibility(int nodeIndex,  const D3DXVECTOR3& start, const D3DXVECTOR3& end)
{
    const float EPSILON = 0.00001f;

    // 1. Safety / Empty Check
    if (nodeIndex == -1) return false;
    const BSPNode& node = nodePool[nodeIndex];

    if (!node.members.empty())
    {
        D3DXVECTOR3 dir = end - start;
        float rayLen = D3DXVec3Length(&dir);
        D3DXVec3Normalize(&dir, &dir);

        for (const auto& tri : node.members)
        {
            D3DXVECTOR3 v0(tri.v[0].x, tri.v[0].y, tri.v[0].z);
            D3DXVECTOR3 v1(tri.v[1].x, tri.v[1].y, tri.v[1].z);
            D3DXVECTOR3 v2(tri.v[2].x, tri.v[2].y, tri.v[2].z);

            float u, v, dist;
            // Use your DoubleSided intersection if you have it, otherwise D3DXIntersectTri
            //if (IntersectTriDoubleSided(start, dir,  v0, v1, v2, dist, u,v))
            if (IntersectTriangleShadow(start, dir, rayLen + EPSILON, v0, v1, v2))
            {
                // Check if hit is strictly between Start and End
                //if ( dist < rayLen)
               //{
                    return true; // BLOCKED

                //}
            }
        }
    }

    // If it's a leaf, we are done (we checked members above)
    if (node.isLeaf) return false;
    
    // Calculate distance of Start and End points from the splitter plane
    float d1 = node.plane.a * start.x + node.plane.b * start.y + node.plane.c * start.z + node.plane.d;
    float d2 = node.plane.a * end.x + node.plane.b * end.y + node.plane.c * end.z + node.plane.d;

    // 1. Fully in Front? -> Check Front Child only
    if (d1 >= -EPSILON && d2 >= -EPSILON)
    {
        return CheckNodeVisibility(node.iFront, start, end);

    }

    // 2. Fully Behind? -> Check Back Child only
    if (d1 < EPSILON && d2 < EPSILON)
    {
        return CheckNodeVisibility(node.iBack, start, end);

    }

    // 3. Spanning? (Crosses the plane) -> Check First side, then Second side
    // We need to find the intersection point on the plane to split the ray.
    // T = d1 / (d1 - d2)
    float t = d1 / (d1 - d2); // 0.0 to 1.0
    // Intersection Point 'Mid'
    D3DXVECTOR3 mid;
    D3DXVec3Lerp(&mid, &start, &end, t);

    // Determine order: Visit the side containing 'start' first.
    // This allows us to "Early Out" if we hit something close.
    if (d1 > 0)
    {
        // Start is in Front: Check Front(Start->Mid) then Back(Mid->End)

        if (CheckNodeVisibility(node.iFront, start, mid)) return true;

        return CheckNodeVisibility(node.iBack, mid, end);

    }
    else
    {
        // Start is in Back: Check Back(Start->Mid) then Front(Mid->End)

        if (CheckNodeVisibility(node.iBack, start, mid)) return true;

        return CheckNodeVisibility(node.iFront, mid, end);

    }
}

void CBSPlevel::AddHemisphereLight(const D3DXCOLOR& skyColor, float intensity, int numSamples)
{
    float skyDist = 100000.0f; // Far away
    if (m_bStopRequested) return;

    #pragma omp parallel for schedule(dynamic)

    for (int j = 0; j < m_patches.size(); j++)
    {
        RADPATCH& patch = m_patches[j];
        D3DXVECTOR3 incomingLight(0, 0, 0);
        int hitsSky = 0;

        for (int i = 0; i < numSamples; i++)
        {
            // 1. Get Random Direction
            D3DXVECTOR3 dir = GetRandomHemisphereVector(patch.normal);
            // 2. Raycast (Offset start slightly to avoid self-intersection)
            D3DXVECTOR3 start = patch.center + (patch.normal * 0.005f);

            // Use your BSP Optimized RayCast
            if (!RayCastAny(start, dir, skyDist))
            {
                D3DXCOLOR sampleColor = D3DXCOLOR(0,0,0,1);
                // HIT SKY!
                // Determine color based on Y direction of the ray
                // dir.y = 1.0 is straight up, dir.y = -1.0 is straight down
                /*if (dir.y > 0.0f) {
                    sampleColor = skyColor;
                }
                else {
                    sampleColor = D3DXCOLOR(0.00f, 0.00f, 0.00f, 0.0f);
                }*/
                D3DXCOLOR groundColor = D3DXCOLOR(0.99f, 0.00f, 0.00f, 1.0f);
                float t = 0.5f * (dir.y + 1.0f); // Map -1...1 to 0...1
                sampleColor.r = groundColor.r + t * (skyColor.r - groundColor.r);
                sampleColor.g = groundColor.g + t * (skyColor.g - groundColor.g);
                sampleColor.b = groundColor.b + t * (skyColor.b - groundColor.b);
                // Lambert's Law: Light hitting at an angle is weaker
                // (Light coming from straight up is stronger than light from the horizon)
                float NdotL = D3DXVec3Dot(&patch.normal, &dir);
                if (NdotL < 0.0f) NdotL = 0.0f; // Should not happen due to GetRandomHemisphereVector, but safety first

                incomingLight.x += sampleColor.r * NdotL;
                incomingLight.y += sampleColor.g * NdotL;
                incomingLight.z += sampleColor.b * NdotL;

                hitsSky++;
            }
        }

        // 3. Average the result
        if (hitsSky > 0)
        {
            // Divide total gathered light by the number of samples we took
            // Multiply by intensity scalar
            //float scale = intensity / ((float)numSamples * D3DX_PI); // Added PI here
            float scale = (intensity * 2.0f * D3DX_PI) / (float)numSamples;
            D3DXVECTOR3 incomingFlux = incomingLight * scale * patch.area;
            // 2. Apply Reflectivity (Albedo)
            D3DXVECTOR3 reflectedFlux;
            reflectedFlux.x = incomingFlux.x * patch.reflectivity.r;
            reflectedFlux.y = incomingFlux.y * patch.reflectivity.g;
            reflectedFlux.z = incomingFlux.z * patch.reflectivity.b;

            // 3. Update Visuals(Radiosity = Flux / Area)
                // This makes large walls dimmer and small detailed spots brighter for the same energy.
                if (patch.area > 1e-6f)
                {
                    patch.accumulated += reflectedFlux / patch.area;
                }

            // 4. Update Physics (Unshot Energy = Flux)
            patch.unshot += reflectedFlux;
        }
    }
}

// Helper: Returns a random normalized vector inside the hemisphere of the normal
//D3DXVECTOR3 CBSPlevel::GetRandomHemisphereVector(const D3DXVECTOR3& normal)
//{
//    D3DXVECTOR3 v;
//    // 1. Generate random point in unit cube [-1, 1]
//    // (Using rand() is sufficient for baking)
//    do {
//        v.x = ((rand() % 2000) / 1000.0f) - 1.0f;
//        v.y = ((rand() % 2000) / 1000.0f) - 1.0f;
//        v.z = ((rand() % 2000) / 1000.0f) - 1.0f;
//
//        // Normalize
//        D3DXVec3Normalize(&v, &v);
//    } while (D3DXVec3LengthSq(&v) < 0.001f); // Retry if we got a zero vector
//
//    // 2. If vector points "into" the wall (opposite of normal), flip it
//    if (D3DXVec3Dot(&v, &normal) < 0.0f)
//    {
//        v = -v;
//    }
//
//    return v;
//}

D3DXVECTOR3 CBSPlevel::GetRandomHemisphereVector(const D3DXVECTOR3& normal)
{
    // Thread-Local Index: Each thread keeps its own counter.
    // No locks. No waiting.
    static thread_local int idx = 0;
    // 1. Pick a pseudo-random index using a prime number stride
    // (This acts as a fast random generator)
    idx = (idx + 12345) % 4096; // 4096 must match table size
    // 2. Fetch pre-calculated vector
    D3DXVECTOR3 v = m_randomDirTable[idx];
    // 3. Hemisphere Check
    // If vector points "into" the wall, flip it to point "out".
    // This effectively converts a Uniform Sphere distribution -> Uniform Hemisphere.
    if (D3DXVec3Dot(&v, &normal) < 0.0f)
    {
        // Simple negation is faster than arithmetic
        v.x = -v.x;
        v.y = -v.y;
        v.z = -v.z;
    }
    return v;
}

// Custom Intersection that hits BOTH Front and Back faces
BOOL CBSPlevel::IntersectTriDoubleSided(
    const D3DXVECTOR3& orig, const D3DXVECTOR3& dir,
    const D3DXVECTOR3& v0, const D3DXVECTOR3& v1, const D3DXVECTOR3& v2,
    float& dist, float& u, float& v)
{
    // Möller-Trumbore Algorithm
    const float EPSILON = 0.001f;

    D3DXVECTOR3 edge1 = v1 - v0;
    D3DXVECTOR3 edge2 = v2 - v0;
    D3DXVECTOR3 pvec;
    D3DXVec3Cross(&pvec, &dir, &edge2);
    float det = D3DXVec3Dot(&edge1, &pvec);
    // -------------------------------------------------------------
    // FIX: Standard D3DXIntersectTri culls if det < EPSILON
    // We only check if it is close to 0 (parallel), allowing negative det (backface).
    // -------------------------------------------------------------
    if (det > -EPSILON && det < EPSILON) return FALSE;

    float invDet = 1.0f / det;

    D3DXVECTOR3 tvec = orig - v0;
    u = D3DXVec3Dot(&tvec, &pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return FALSE;

    D3DXVECTOR3 qvec;
    D3DXVec3Cross(&qvec, &tvec, &edge1);
    v = D3DXVec3Dot(&dir, &qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return FALSE;

    // t is the distance
    dist = D3DXVec3Dot(&edge2, &qvec) * invDet;

    // Optional: Filter out hits behind the camera
    if (dist < 0.0f) return FALSE;

    return TRUE;
}

void CBSPlevel::ThreadWorker()
{
    _log(L"Background Thread Started.\n");

    // --- PHASE 1: BUILD BSP ---
    m_eState = BS_BUILDING_BSP;
    m_fProgress = 0.0f;

    // Note: We cannot easily track exact progress of recursion, 
    // so we just pulse it or set it to fixed milestones.
    //ExtractTriangles();
    if (m_bStopRequested) return;
	bPointsDraw = true;
    SubdivideGeometry(m_triangles);
    if (m_bStopRequested) return;
    m_fProgress = 0.1f; // Subdiv done

    // Re-implement BuildBSP logic here slightly to avoid calling the old void function
    // or just call your existing internal helpers.
    nodePool.clear();
    nodePool.reserve(50000);
    int rootIndex = AllocateNode();
    BuildTree(rootIndex, m_subd_triangles);

    m_fProgress = 0.3f; // BSP Tree Built
    if (m_bStopRequested) return;

    // --- PHASE 2: RADIOSITY ---
    m_eState = BS_CALC_RAD;
    _log(L"Preparing Radiosity...\n");
    PrepareRadiosity();
    bPointsDraw = false;

    const int maxIterations = 4096;
    for (int i = 0; i < maxIterations; i++)
    {
        if (m_bStopRequested) return;

        if (!RunRadiosityIteration())
            break; // Convergence reached

        // Update Progress: Map iterations to 0.3 -> 1.0 range
        float radProgress = (float)i / (float)maxIterations;
        m_fProgress = 0.3f + (radProgress * 0.7f);
        ApplyRadiosityToMesh();

    }


    // --- DONE ---
    m_fProgress = 1.0f;
    m_eState = BS_READY;
    _log(L"Background Build Complete.\n");
}

void CBSPlevel::InitPhysics(btDynamicsWorld* dynamicsWorld)
{
	m_pdworld = dynamicsWorld;
    // 1. Create the Triangle Mesh interface
    m_pTriangleMesh = new btTriangleMesh();

    for (const auto& tri : m_triangles)
    {
            m_pTriangleMesh->addTriangle(
                btVector3(tri.v[0].x, tri.v[0].y, tri.v[0].z),
                btVector3(tri.v[1].x, tri.v[1].y, tri.v[1].z),
                btVector3(tri.v[2].x, tri.v[2].y, tri.v[2].z)
			);
    }

    // 3. Create the Shape
    // Use Quantized AABB Compression to save memory
    m_pCollisionShape = new btBvhTriangleMeshShape(m_pTriangleMesh, true);

    // 4. Create the Collision Object
    m_pLevelObject = new btCollisionObject();
    m_pLevelObject->setCollisionShape(m_pCollisionShape);

    // Set friction and restitution for the ground
    m_pLevelObject->setFriction(0.5f);
    m_pLevelObject->setRestitution(0.3f);

    // 5. Add to the World
    // Levels are static, so we don't need a MotionState or Mass
    dynamicsWorld->addCollisionObject(m_pLevelObject);
}

void CBSPlevel::CleanupPhysics()
{
    if (m_pdworld && m_pLevelObject) {
        m_pdworld->removeCollisionObject(m_pLevelObject);
        delete m_pLevelObject;
    }
    SAFE_DELETE( m_pCollisionShape);
    SAFE_DELETE( m_pTriangleMesh);
}