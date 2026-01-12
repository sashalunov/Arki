// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#pragma warning(disable: 4996)			// deprecated stuff

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
// Windows Header Files:
#include <windows.h>
#include <hidusage.h> // Required for usage codes
#include <mmsystem.h>
// C RunTime Header Files
// --------------------------------------------------------------------------------
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <deque>
#include <commdlg.h> // Required for GetOpenFileName
#include <map>
#include <string>
#include <random>    // The modern random library
// TODO: reference additional headers your program requires here
// --------------------------------------------------------------------------------
#include <d3d9.h>
#include <d3dx9.h>
//#include <dinput.h>
#include <stdio.h>
#include <math.h> // Required for sqrt
#include <cmath> // Required for sqrt (distance calculation)
#include <algorithm> // For standard math operations
#include <vector>
#include <list>
#include <fstream> // Required for file I/O
#include <xaudio2.h>
#include <x3daudio.h>

#include <btBulletDynamicsCommon.h>

#include "resource.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
static FLOAT Epsilon = 0.001f;// Helper: Check if float is approximately zero
inline bool IsZero(float val, float epsilon = 0.001f) {
	return std::abs(val) <= epsilon;
}
// Helper: Opens a Windows file picker (Wide String Version)
std::wstring OpenFileDialog(HWND owner, LPCWSTR filter = L"Arki Level\0*.txt;*.lvl\0All\0*.*\0");
// Helper: Opens a Windows "Save As" file picker (Wide String Version)
std::wstring SaveFileDialog(HWND owner, LPCWSTR filter = L"Arki Level\0*.txt;*.lvl\0All\0*.*\0");

// Helper: Converts Wide String to UTF-8 (ImGui format)
// We use a static buffer so the pointer remains valid long enough for ImGui to draw it.
const char* WToUTF8(const std::wstring& wide);

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

void InitLight(D3DLIGHT9& light, D3DLIGHTTYPE ltType, FLOAT x = 0.0f, FLOAT y = 0.0f, FLOAT z = 0.0f, FLOAT r = 1.0f, FLOAT g = 1.0f, FLOAT b = 1.0f);
void InitMaterial(D3DMATERIAL9& mtrl, FLOAT a = 1.0f, FLOAT r = 0.5f, FLOAT g = 0.5f, FLOAT b = 0.5f);
void InitMaterialS(D3DMATERIAL9& mtrl, FLOAT a = 1.0f, FLOAT r = 0.5f, FLOAT g = 0.5f, FLOAT b = 0.5f, FLOAT S = 0.5f);
ID3DXMesh* CreateRoundedBox(IDirect3DDevice9* device, float width, float height, float depth, float radius, int slices);


float Hash(float n);
float Hash3D(D3DXVECTOR3 p);
float NebulaNoise(D3DXVECTOR3 p);


// TGA specific constants and structs
#pragma pack (push, 1)
struct  TGAHEADER
{
	BYTE	idLength;
	BYTE	colormapType;
	BYTE	imageType;
	WORD	colormapIndex;
	WORD	colormapLength;
	BYTE	colormapEntrySize;
	WORD	xOrigin;
	WORD	yOrigin;
	WORD	width;
	WORD	height;
	BYTE	pixelSize;
	BYTE	imageDesc;
};
#pragma	pack (pop)
// Random float in range [min, max]
inline float RandomFloatInRange(float min, float max)
{
	return min + (max - min) * ((float)rand() / RAND_MAX);
}

// Helper: Convert Bullet Physics Transform to DirectX 9 Matrix
inline D3DXMATRIXA16 ConvertBulletTransform(const btTransform& trans)
{
	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity(&matWorld);

	// 1. Extract Rotation Basis and Origin (Position)
	btMatrix3x3 rot = trans.getBasis();
	btVector3 pos = trans.getOrigin();

	// 2. Copy Rotation (Row-Major mapping)
	matWorld._11 = (float)rot[0][0]; matWorld._12 = (float)rot[0][1]; matWorld._13 = (float)rot[0][2];
	matWorld._21 = (float)rot[1][0]; matWorld._22 = (float)rot[1][1]; matWorld._23 = (float)rot[1][2];
	matWorld._31 = (float)rot[2][0]; matWorld._32 = (float)rot[2][1]; matWorld._33 = (float)rot[2][2];

	// 3. Copy Translation (Bottom row in DX9)
	matWorld._41 = (float)pos.getX();
	matWorld._42 = (float)pos.getY();
	matWorld._43 = (float)pos.getZ();

	return matWorld;
}


