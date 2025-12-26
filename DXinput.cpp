#include "StdAfx.h"
#include "Logger.h"
#include "D3DRender.h"
#include "DXinput.h"


static CDInput g_dinput;
CDInput *dinput = (CDInput*)&g_dinput;

// --------------------------------------------------------------------------------
CDInput::CDInput(void)
{
	m_pDI = NULL;
	m_pKeyboard = NULL;
	m_pMouse = NULL;

	memset(m_keys,0, sizeof(m_keys));

	m_dwMouseX = 0;
	m_dwMouseY = 0;
	m_dwMouseZ = 0;

	m_bMouseButton[0] = FALSE;
	m_bMouseButton[1] = FALSE;
	m_bMouseButton[2] = FALSE;

	m_dfMouse.dwSize = sizeof( DIDATAFORMAT );
	m_dfMouse.dwObjSize = sizeof( DIOBJECTDATAFORMAT );
	m_dfMouse.dwFlags = DIDF_RELAXIS;
	m_dfMouse.dwDataSize = sizeof( m_MouseState );

}

// --------------------------------------------------------------------------------
CDInput::~CDInput(void)
{
//	Shutdown();
}

// --------------------------------------------------------------------------------
BOOL CDInput::Initialize()
{
	HRESULT hr;
	m_hInst = GetModuleHandle(NULL);
	m_hWnd = d3d9->GetHWND();


	_log(L"Initializing DirectInput8\n");

	hr = DirectInput8Create(m_hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDI, NULL);
	if(FAILED(hr))
	{
		_log(L"Failed to initialise DirectInput \n");
		return FALSE;
	}

	if(!CreateKeyboard())
	{
		_log(L"Failed to create Keyboard device\n");
		return FALSE;
	}

	if(!CreateMouse())
	{
		_log(L"Failed to create Mouse device\n");
		return FALSE;
	}

	Acquire(TRUE);

	return TRUE;
}

// --------------------------------------------------------------------------------
BOOL CDInput::Shutdown(void)
{
	_log(L"Shutting down DirectInput8\n");


	SAFE_RELEASE(m_pMouse);
	SAFE_RELEASE(m_pKeyboard);
	SAFE_RELEASE(m_pDI);

	return TRUE;
}

// --------------------------------------------------------------------------------
BOOL CDInput::CreateKeyboard(void)
{
	HRESULT hr;

	hr = m_pDI->CreateDevice(GUID_SysKeyboard, &m_pKeyboard, NULL);
	if(FAILED(hr))
		return FALSE;

	hr = m_pKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if(FAILED(hr))
		return FALSE;

	hr = m_pKeyboard->SetCooperativeLevel(m_hWnd, DISCL_NONEXCLUSIVE  | DISCL_FOREGROUND);
	if(FAILED(hr))
		return FALSE;

	return TRUE;
}

// --------------------------------------------------------------------------------
BOOL CDInput::CreateMouse(void)
{
	HRESULT hr;
	DIPROPDWORD dipdw;

	hr = m_pDI->CreateDevice(GUID_SysMouse, &m_pMouse, NULL);
	if(FAILED(hr))
		return FALSE;

	hr = m_pMouse->SetDataFormat(&c_dfDIMouse);
	if(FAILED(hr))
		return FALSE;

	hr = m_pMouse->SetCooperativeLevel(m_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if(FAILED(hr))
		return FALSE;

	dipdw.diph.dwSize			= sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize= sizeof(DIPROPHEADER);
	dipdw.diph.dwObj			= 0;
	dipdw.diph.dwHow			= DIPH_DEVICE;
	dipdw.dwData				= 32;

	//hr = m_pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
	//if(FAILED(hr))
	//	return FALSE;

	return TRUE;
}

// --------------------------------------------------------------------------------
void CDInput::Acquire(BOOL bAcquire)
{
	if(bAcquire)
	{
		if(m_pKeyboard)
			m_pKeyboard->Acquire();

		if(m_pMouse)
			m_pMouse->Acquire();
	}
	else
	{
		if(m_pKeyboard)
			m_pKeyboard->Unacquire();

		if(m_pMouse)
			m_pMouse->Unacquire();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::UpdateKeyboard(void)
{
	HRESULT hr;

	memcpy(m_oldkeys, m_keys, sizeof(m_keys));

	hr = m_pKeyboard->GetDeviceState(sizeof(m_keys), m_keys);
	if(hr == DIERR_INPUTLOST)
	{
		m_pKeyboard->Acquire();
		hr = m_pKeyboard->GetDeviceState(sizeof(m_keys), m_keys);
	}

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::UpdateMouse(void)
{
	HRESULT hr;

	//m_dwMouseX = 0;
	//m_dwMouseY = 0;
	//m_dwMouseZ = 0;

	////m_bMouseButton[0] = FALSE;
	////m_bMouseButton[1] = FALSE;
	////m_bMouseButton[2] = FALSE;

	//while(TRUE)
	//{
	//	DIDEVICEOBJECTDATA dod;
	//	DWORD dwElements = 1;

	//	hr = m_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &dod, &dwElements, 0);
	//	if(hr == DIERR_INPUTLOST)
	//	{
	//		m_pMouse->Acquire();
	//		hr = m_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &dod, &dwElements, 0);
	//	}

	//	if(FAILED(hr) || dwElements == 0)
	//		break;

	//	switch(dod.dwOfs)
	//	{
	//	case DIMOFS_X:
	//		m_dwMouseX = dod.dwData;
	//		break;

	//	case DIMOFS_Y:
	//		m_dwMouseY = dod.dwData;
	//		break;

	//	case DIMOFS_Z:
	//		m_dwMouseZ = dod.dwData;
	//		break;

	//	case DIMOFS_BUTTON0:
	//		if(dod.dwData != 0)
	//			m_bMouseButton[0] = TRUE;
	//		else
	//			m_bMouseButton[0] = FALSE;
	//		break;

	//	case DIMOFS_BUTTON1:
	//		if(dod.dwData != 0)
	//			m_bMouseButton[1] = TRUE;
	//		else
	//			m_bMouseButton[1] = FALSE;
	//		break;

	//	case DIMOFS_BUTTON2:
	//		if(dod.dwData != 0)
	//			m_bMouseButton[2] = TRUE;
	//		else
	//			m_bMouseButton[2] = FALSE;
	//		break;


	//	}
	//}

	hr = m_pMouse->Poll();
    if( FAILED( hr ) )
    {
        hr = m_pMouse->Acquire();
        while( hr == DIERR_INPUTLOST )
            hr = m_pMouse->Acquire();

		return TRUE;
	}

    if( FAILED( hr = m_pMouse->GetDeviceState( sizeof( DIMOUSESTATE ), &m_MouseState ) ) )
        return FALSE; // The device should have been acquired during the Poll()

	m_dwMouseX = m_MouseState.lX;
	m_dwMouseY = m_MouseState.lY;
	m_dwMouseZ = m_MouseState.lZ;

    for( int i = 0; i < 3; i++ )
    {
		m_bMouseButton[i] = m_MouseState.rgbButtons[i] & 0x80;
    }

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::ProcessInput(void)
{
	if(!UpdateKeyboard())
		return FALSE;

	if(!UpdateMouse())
		return FALSE;

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::IsKKeyDown(DWORD key)
{
	if(m_keys[key] & 0x80)return TRUE;

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::IsMKeyDown(DWORD key)
{
	return m_bMouseButton[key];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::IsKKeyPress(DWORD key)
{
	return ((m_keys[key] & 0x80) && !(m_oldkeys[key] & 0x80)) ? TRUE : FALSE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDInput::IsMKeyPress(DWORD key)
{
	return m_bMouseButton[key];
}
