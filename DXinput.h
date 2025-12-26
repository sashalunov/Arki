#pragma once

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
class CDInput
{
public:
	CDInput(void);
	~CDInput(void);

	BOOL Initialize();
	BOOL Shutdown(void);

	BOOL ProcessInput(void);
	void Acquire(BOOL bAcquire);

	BOOL IsKKeyDown(DWORD key);
	BOOL IsMKeyDown(DWORD key);
	BOOL IsKKeyPress(DWORD key);
	BOOL IsMKeyPress(DWORD key);

	float	GetDeltaX(){ return (float)m_dwMouseX; }
	float	GetDeltaY(){ return (float)m_dwMouseY; }
	float	GetDeltaZ(){ return (float)m_dwMouseZ; }

private:
	BOOL CreateKeyboard(void);
	BOOL CreateMouse(void);

	BOOL UpdateKeyboard(void);
	BOOL UpdateMouse(void);

public:

private:
	HINSTANCE	m_hInst;
	HWND		m_hWnd;

	LPDIRECTINPUT8			m_pDI;
	LPDIRECTINPUTDEVICE8	m_pKeyboard;
	LPDIRECTINPUTDEVICE8	m_pMouse;

	DIMOUSESTATE			m_MouseState;
	DIDATAFORMAT            m_dfMouse;

	BYTE			m_keys[256];
	BYTE			m_oldkeys[256];

	int				m_dwMouseX;
	int				m_dwMouseY;
	int				m_dwMouseZ;

	BOOL		m_bMouseButton[3];
	BOOL		m_bOldMouseButton[3];

};
extern CDInput *dinput;
