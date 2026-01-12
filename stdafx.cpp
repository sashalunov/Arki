// stdafx.cpp : source file that includes just the standard includes

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void InitLight(D3DLIGHT9& light, D3DLIGHTTYPE ltType, float x, float y, float z, float r, float g, float b)
{
    D3DXVECTOR3 vecLightDirUnnormalized(x, y, z);

    ZeroMemory(&light, sizeof(D3DLIGHT9));
    light.Type = ltType;
    light.Diffuse.r = r;
    light.Diffuse.g = g;
    light.Diffuse.b = b;
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecLightDirUnnormalized);
    light.Position.x = x;
    light.Position.y = y;
    light.Position.z = z;
    light.Range = 1000.0f;
	light.Specular.r = light.Specular.g = light.Specular.b = 1.0f;

}

void InitMaterial(D3DMATERIAL9& mtrl, float a, float r, float g, float b)
{
    ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
    mtrl.Diffuse.a = mtrl.Ambient.a = a;
    mtrl.Diffuse.r = mtrl.Ambient.r = r;
    mtrl.Diffuse.g = mtrl.Ambient.g = g;
    mtrl.Diffuse.b = mtrl.Ambient.b = b;
}
void InitMaterialS(D3DMATERIAL9& mtrl, float a, float r, float g, float b, float s)
{
    ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
    mtrl.Diffuse.a = mtrl.Ambient.a = mtrl.Specular.a = mtrl.Emissive.a = a ;
    mtrl.Diffuse.r = mtrl.Ambient.r = r;
    mtrl.Diffuse.g = mtrl.Ambient.g = g;
    mtrl.Diffuse.b = mtrl.Ambient.b = b;
	mtrl.Specular.r = mtrl.Specular.g = mtrl.Specular.b = s;
	mtrl.Power = 20.0f;
}


// 1. Pseudo-Random Hash (Returns 0.0 to 1.0)
// Used for generating stars
float Hash(float n)
{
    return fmod((sin(n) * 43758.5453123f), 1.0f);
}

float Hash3D(D3DXVECTOR3 p)
{
    D3DXVECTOR3 v = D3DXVECTOR3(12.9898f, 78.233f, 45.164f);
    float dotP = D3DXVec3Dot(&p, &v);
    return Hash(dotP);
}

// 2. Simple "Cloudy" Noise
// Combines sine waves to create a nebulous look
float NebulaNoise(D3DXVECTOR3 p)
{
    const float baseFreq = 4.0f;
    float total = 0.0f;

    // Layer 1
    total += sin(p.x * baseFreq + 1.3f) * cos(p.y * baseFreq + 2.4f) * sin(p.z * baseFreq + 0.5f);
    // Layer 2 (Higher frequency, lower amplitude)
    total += sin(p.x * baseFreq * 2 + 0.2f) * cos(p.y * baseFreq * 2 + 3.1f) * sin(p.z * baseFreq * 2 + 1.5f) * 0.5f;

    // Normalize roughly to 0..1 range
    return (total / 3.0f) + 0.5f;
}



// Helper: Opens a Windows file picker (Wide String Version)
std::wstring OpenFileDialog(HWND owner = NULL, LPCWSTR filter)
{
    OPENFILENAMEW ofn;       // Wide structure
    wchar_t szFile[260];     // Wide buffer

    // Initialize buffer
    ZeroMemory(szFile, sizeof(szFile));

    // Initialize OPENFILENAMEW
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = L'\0';
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t); // Size in WCHARs
    ofn.lpstrFilter = filter; // Wide string literals
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        return std::wstring(ofn.lpstrFile);
    }
    return L""; // User cancelled
}

// Helper: Opens a Windows "Save As" file picker (Wide String Version)
std::wstring SaveFileDialog(HWND owner = NULL, LPCWSTR filter)
{
    OPENFILENAMEW ofn;
    wchar_t szFile[260];

    ZeroMemory(szFile, sizeof(szFile));

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = L'\0';
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"txt";

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn) == TRUE)
    {
        return std::wstring(ofn.lpstrFile);
    }
    return L"";
}

// Helper: Converts Wide String to UTF-8 (ImGui format)
// We use a static buffer so the pointer remains valid long enough for ImGui to draw it.
const char* WToUTF8(const std::wstring& wide) 
{
    static char buffer[256]; // Static buffer reuse
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, buffer, 256, NULL, NULL);
    return buffer;
}

// Creates a 3D Rounded Box Mesh
// width, height, depth: Total dimensions of the box
// radius: Radius of the rounded corners
// slices: Smoothness of the curves (higher = smoother, e.g., 16 or 24)
ID3DXMesh* CreateRoundedBox(IDirect3DDevice9* device, float width, float height, float depth, float radius, int slices)
{
    // 1. Create a high-res Sphere to start. 
    // We use a sphere because it already has the correct topology (smooth corners).
    ID3DXMesh* pSphere = NULL;
    D3DXCreateSphere(device, 1.0f, slices, slices, &pSphere, NULL);

    // 2. Clone the mesh to get a vertex buffer we can modify (SYSTEMMEM allows locking)
    ID3DXMesh* pRoundedBox = NULL;
    pSphere->CloneMeshFVF(D3DXMESH_SYSTEMMEM, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, device, &pRoundedBox);
    pSphere->Release(); // Done with the template sphere

    // 3. Lock the vertex buffer
    struct Vertex { float x, y, z; float nx, ny, nz; float u, v; };
    Vertex* v = NULL;
    pRoundedBox->LockVertexBuffer(0, (void**)&v);

    int numVerts = pRoundedBox->GetNumVertices();

    // 4. Calculate the "Inner Box" limits.
    // The rounded box is essentially a smaller flat box + a radius extension.
    float innerW = (width * 0.5f) - radius;
    float innerH = (height * 0.5f) - radius;
    float innerD = (depth * 0.5f) - radius;

    // Safety check: if radius is too big, clamp it
    if (innerW < 0) innerW = 0;
    if (innerH < 0) innerH = 0;
    if (innerD < 0) innerD = 0;

    for (int i = 0; i < numVerts; i++)
    {
        // Get the unit vector from the center (which is just the sphere vertex pos)
        D3DXVECTOR3 normal(v[i].x, v[i].y, v[i].z);
        D3DXVec3Normalize(&normal, &normal);

        // --- The Morphing Logic ---
        // We push the vertex to the "corner" of the inner flat box...
        float sx = (normal.x > 0) ? innerW : -innerW;
        float sy = (normal.y > 0) ? innerH : -innerH;
        float sz = (normal.z > 0) ? innerD : -innerD;

        // ...and then add the radius in the direction of the normal.
        // This preserves the spherical curve at the corners but flattens the faces.
        v[i].x = sx + (normal.x * radius);
        v[i].y = sy + (normal.y * radius);
        v[i].z = sz + (normal.z * radius);

        // Update the normal (it remains the same as the sphere normal!)
        v[i].nx = normal.x;
        v[i].ny = normal.y;
        v[i].nz = normal.z;
    }

    pRoundedBox->UnlockVertexBuffer();

    // 5. Recalculate Bounding Box (Optional, D3DX updates this automatically usually)
    // D3DXComputeBoundingBox(...) 

    return pRoundedBox;
}