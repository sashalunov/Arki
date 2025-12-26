// --------------------------------------------------------------------------------
// main.cpp : Defines the entry point for the application.
// --------------------------------------------------------------------------------
#include "stdafx.h"
#include "ArkiGame.h"
#include "imgui.h"
#include "main.h"

// Global Game Pointer
ArkiGame* g_pGame = NULL;

// --------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	// Main message loop
	MSG msg = {};

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PHYSTANK));

	g_pGame = new ArkiGame();

	if (g_pGame->Init())
	{
		PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				g_pGame->Run();
			}
		}
	}
	g_pGame->Shutdown();
	delete g_pGame;
	return static_cast<int>(msg.wParam);
}

// --------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT clientrect;

	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
	// (Your code process Win32 messages)
	// (You should discard mouse/keyboard messages in your game/engine when io.WantCaptureMouse/io.WantCaptureKeyboard are set.)
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(0, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_ENTERSIZEMOVE:
		//d3d9->Activate(FALSE);
		break;
	case WM_EXITSIZEMOVE:
		//d3d9->Activate(TRUE);
		break;
	case WM_SIZE:
		//GetClientRect(hWnd, &clientrect);
		if (d3d9->GetDevice() != NULL && wParam != SIZE_MINIMIZED)
		{
			d3d9->Resize(LOWORD(lParam), HIWORD(lParam));
			g_pGame->ResetDevice();
		}
		if( SIZE_MINIMIZED == wParam )
        {
			//d3d9->Activate(FALSE);		
		}
		if( SIZE_MAXIMIZED == wParam || SIZE_RESTORED == wParam)
        {

		}		
		break;
	case WM_ACTIVATEAPP:
		//d3d9->Activate(BOOL(wParam));		
		break;
	case WM_ACTIVATE:
		// LOWORD(wParam) contains the activation state
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			// Window lost focus -> PAUSE GAME
			if (g_pGame) g_pGame->m_isPaused = true;
		}
		else
		{
			// Window gained focus (WA_ACTIVE or WA_CLICKACTIVE) -> RESUME GAME
			if (g_pGame) g_pGame->m_isPaused = false;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		//if(d3d9->GetDevice())d3d9->GetDevice()->Present(0,0,0,0);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_INPUT:
	{
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
		RAWINPUT* raw = (RAWINPUT*)lpb;

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			// ACCUMULATE! Do not just set =.
			// If the user whips the mouse fast, you might get 5 messages in one frame.
			//mouseDeltaX += raw->data.mouse.lLastX;
			//mouseDeltaY += raw->data.mouse.lLastY;
			g_pGame->SetMouseDeltas(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
		}
		break;
	}
	case WM_MOUSEWHEEL:
		if (g_pGame) g_pGame->scrollAmount += (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);

}
// --------------------------------------------------------------------------------
// Message handler for about box.
// --------------------------------------------------------------------------------
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
