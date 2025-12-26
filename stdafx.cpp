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
    mtrl.Diffuse.a = mtrl.Ambient.a = a;
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
    float dotP = D3DXVec3Dot(&p, &D3DXVECTOR3(12.9898f, 78.233f, 45.164f));
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

