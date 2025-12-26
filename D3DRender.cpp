// --------------------------------------------------------------------------------
//	file: D3DRender.cpp
//---------------------------------------------------------------------------------
//  created: 10.08.2010 1:28 by JIyHb
// --------------------------------------------------------------------------------
#include "StdAfx.h"
#include "Logger.h"
#include "D3DRender.h"

static CD3DRender g_render;
CD3DRender *d3d9 = (CD3DRender*)&g_render;

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CD3DRender::CD3DRender(void)
{
	m_hInst = GetModuleHandle(NULL);
	m_hWnd	= NULL;

	m_dwWndWidth = 1280;
	m_dwWndHeight = 720;
	m_dwWndStyle = WS_OVERLAPPEDWINDOW;
	m_dwWndExStyle = WS_EX_CLIENTEDGE;
	m_bFullscreen = 0;
	m_bDeviceLost = FALSE;
	m_bActive = FALSE;
    m_bDeviceObjectsInited = FALSE;
    m_bDeviceObjectsRestored = FALSE;

	LoadString(m_hInst, IDS_APP_TITLE, m_szWndTitle, 100);
	LoadString(m_hInst, IDC_PHYSTANK, m_szWndClass, 100);

	m_pD3D	= NULL;
	m_pD3DDev	= NULL;
	memset(&m_d3dpp, 0, sizeof(D3DPRESENT_PARAMETERS));
	m_fFps				= 0;
	m_d3dpp.BackBufferWidth = m_dwWndWidth;
	m_d3dpp.BackBufferHeight = m_dwWndHeight;


}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CD3DRender::~CD3DRender(void)
{
	//Shutdown();
}

// --------------------------------------------------------------------------------
BOOL CD3DRender::Initialize()
{
	WNDCLASSEX wcex;
	RECT rcWnd;
	HRESULT hr;

	_log(L"Initializing Direct3D9\n");

	// D3d9 stuff
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if(!m_pD3D)
	{
		_log(L"Failed to initialize Direct3D\n");
		return FALSE;
	}

	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT ,&m_desktopmode);


	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= m_hInst;
	wcex.hIcon			= LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_WIN));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PHYSTANK);
	wcex.lpszClassName	= m_szWndClass;
	wcex.hIconSm		= LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_SMALL));
	if(!RegisterClassEx(&wcex))
		return FALSE;

	// Window style
	if(m_bFullscreen)
	{
		m_dwWndStyle	= WS_POPUP|WS_SYSMENU|WS_VISIBLE;
		m_dwWndExStyle	= 0;
		m_dwWndWidth = m_desktopmode.Width;
		m_dwWndHeight = m_desktopmode.Height;

	}

	// Window size
	SetRect(&rcWnd, 0, 0, m_dwWndWidth, m_dwWndHeight);
	AdjustWindowRectEx(&rcWnd, m_dwWndStyle, TRUE, m_dwWndExStyle);

	// Create the main window
	m_hWnd = CreateWindowEx(m_dwWndExStyle, m_szWndClass, m_szWndTitle, m_dwWndStyle, 
		CW_USEDEFAULT, CW_USEDEFAULT, (rcWnd.right-rcWnd.left), (rcWnd.bottom-rcWnd.top), 0, 0, m_hInst, 0);
	if(!m_hWnd)
		return FALSE;

	ShowWindow(m_hWnd, SW_SHOWDEFAULT);
	UpdateWindow(m_hWnd);


	m_d3dpp.BackBufferWidth = m_dwWndWidth;
	m_d3dpp.BackBufferHeight = m_dwWndHeight;
	m_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	m_d3dpp.BackBufferCount = 1;
	m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_d3dpp.MultiSampleQuality = 0;
	m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_d3dpp.hDeviceWindow = m_hWnd;
	m_d3dpp.Windowed = !m_bFullscreen;
	m_d3dpp.EnableAutoDepthStencil = TRUE;
	m_d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
	m_d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	m_d3dpp.FullScreen_RefreshRateInHz = 0;
	m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if(m_bFullscreen)
	{
		m_d3dpp.BackBufferFormat = m_desktopmode.Format;
		m_d3dpp.BackBufferWidth = m_desktopmode.Width;
		m_d3dpp.BackBufferHeight = m_desktopmode.Height;
		m_d3dpp.FullScreen_RefreshRateInHz = m_desktopmode.RefreshRate;
	}

	Log->Log(L"Display mode: %d x %d\n", m_dwWndWidth, m_dwWndHeight );

	hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, m_hWnd, 
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp, &m_pD3DDev);
	if(FAILED(hr))
	{
		_log(L"Failed to create Direct3D9 Device\n");
		return FALSE;
	}

	// Find out which Adapter (Graphics Card) this device was created on
	D3DDEVICE_CREATION_PARAMETERS creationParams;
	if (FAILED(m_pD3DDev->GetCreationParameters(&creationParams))) {
		m_pD3DDev->Release();
		return FALSE;
	}

	//  Get the Adapter Identifier
	D3DADAPTER_IDENTIFIER9 adapterId;
	// Flags: 0 is standard. Use D3DENUM_WHQL_LEVEL to check driver signing (slow).
	if (SUCCEEDED(m_pD3D->GetAdapterIdentifier(creationParams.AdapterOrdinal, 0, &adapterId))) {

		// --- Access the Data ---
		_log(_T("Card Name: %hs\n"), adapterId.Description);
		_log(_T("Driver:    %hs\n"), adapterId.Driver);

		// Use %X for Hexadecimal printing of IDs
		_log(_T("Vendor ID: 0x%04X\n"), adapterId.VendorId);
		_log(_T("Device ID: 0x%04X\n"), adapterId.DeviceId);
	}

	return TRUE;
}

// --------------------------------------------------------------------------------
void CD3DRender::Shutdown()
{
	_log(L"Shutting down Direct3D9\n");

	SAFE_RELEASE(m_pD3DDev);
	SAFE_RELEASE(m_pD3D);

	DestroyWindow(m_hWnd);
	UnregisterClass(m_szWndClass, m_hInst);

	m_hWnd = NULL;
	m_hInst = NULL;
}

// --------------------------------------------------------------------------------
HRESULT CD3DRender::Resize(UINT width, UINT height)
{
	m_d3dpp.BackBufferWidth = m_dwWndWidth = width;
	m_d3dpp.BackBufferHeight = m_dwWndHeight = height;

	
	return S_OK;	
}

// --------------------------------------------------------------------------------
HRESULT CD3DRender::Reset()
{
	HRESULT hr = E_FAIL;
	
	if(m_pD3DDev)hr = m_pD3DDev->Reset(&m_d3dpp);

	return hr;
}