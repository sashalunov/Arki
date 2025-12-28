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


// TODO: reference additional headers your program requires here
// --------------------------------------------------------------------------------
#include <d3d9.h>
#include <d3dx9.h>
//#include <dinput.h>
#include <stdio.h>
#include <math.h> // Required for sqrt
#include <algorithm> // For standard math operations
#include <vector>
#include <string>
#include <list>
#include <fstream> // Required for file I/O

#include "resource.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

void InitLight(D3DLIGHT9& light, D3DLIGHTTYPE ltType, FLOAT x = 0.0f, FLOAT y = 0.0f, FLOAT z = 0.0f, FLOAT r = 1.0f, FLOAT g = 1.0f, FLOAT b = 1.0f);
void InitMaterial(D3DMATERIAL9& mtrl, FLOAT a = 1.0f, FLOAT r = 0.5f, FLOAT g = 0.5f, FLOAT b = 0.5f);
void InitMaterialS(D3DMATERIAL9& mtrl, FLOAT a = 1.0f, FLOAT r = 0.5f, FLOAT g = 0.5f, FLOAT b = 0.5f, FLOAT S = 0.5f);

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
