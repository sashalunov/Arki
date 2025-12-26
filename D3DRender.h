// --------------------------------------------------------------------------------
//	file: D3DRender.h
//---------------------------------------------------------------------------------
//  created: 10.08.2010 1:28 by JIyHb
//  updated: 22.12.2025 04:37 by JIyHb
// --------------------------------------------------------------------------------
#pragma once


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// --------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
class CTimer
{
public:
	CTimer()
	{
		SetThreadAffinityMask(GetCurrentThread(), 1);
		QueryPerformanceFrequency(&m_liClockFrequency);
		QueryPerformanceCounter(&m_liPreviousTime);
		m_dwStartTick = GetTickCount();
	}

	~CTimer()
	{
	}

	// Returns the time in ms since the last call
	double GetElapsedTime()
	{
		LARGE_INTEGER qwTime;
		QueryPerformanceCounter(&qwTime);

		double elapsedTime = (double)(qwTime.QuadPart - m_liPreviousTime.QuadPart);
		double msecTicks = (double)(1000 * elapsedTime / m_liClockFrequency.QuadPart);

		m_liPreviousTime = qwTime;

		return msecTicks;
	}

private:
	LARGE_INTEGER m_liClockFrequency;
	LARGE_INTEGER m_liPreviousTime;
	DWORD m_dwStartTick;

};
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
class CD3DRender
{
public:
	CD3DRender(void);
	~CD3DRender(void);

	BOOL Initialize();
	void Shutdown();

	UINT GetWidth(void) {return m_d3dpp.BackBufferWidth;}
	UINT GetHeight(void) {return m_d3dpp.BackBufferHeight;}
	D3DDISPLAYMODE GetDesktopMode(void) {return m_desktopmode; }
	HWND GetHWND(void) {return m_hWnd;}

	void Activate(BOOL bA){m_bActive = bA;}
	BOOL IsActive(void) {return m_bActive;}

	LPDIRECT3DDEVICE9 GetDevice(void){return m_pD3DDev;}
	HRESULT	Resize(UINT width, UINT height);
	HRESULT	Reset(void);
public:
	D3DPRESENT_PARAMETERS	m_d3dpp;

protected:
	HINSTANCE	m_hInst;
	HWND		m_hWnd;

	BOOL		m_bActive;
	BOOL		m_bDeviceLost;
    BOOL		m_bDeviceObjectsInited;
    BOOL		m_bDeviceObjectsRestored;

	DWORD		m_dwWndStyle;
	DWORD		m_dwWndExStyle;
	TCHAR		m_szWndClass[100];
	TCHAR		m_szWndTitle[100];

	BOOL		m_bFullscreen;
	DWORD		m_dwWndWidth;
	DWORD		m_dwWndHeight;
	DWORD		m_dwRefreshRate;

	D3DFORMAT	m_fmtFullscreen;

	LPDIRECT3D9				m_pD3D;
	LPDIRECT3DDEVICE9		m_pD3DDev;
	D3DDISPLAYMODE			m_desktopmode;

	// Settings
	//CConfigFile	m_pCfg;

	// Statistics
	FLOAT		m_fFps;
	char		m_szFps[512];

};
extern CD3DRender *d3d9;