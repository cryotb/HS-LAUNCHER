#include <Include.hpp>

LRESULT CB::WindowProcess(_In_ HWND hWindow, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if(G::P::Input && G::P::Input->is_ready())
		G::P::Input->wnd_proc(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		return FALSE;
	default:
		break;
	}
	
	switch(uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		G::P::Core->OnExitRequest();
		break;
	default:
		return DefWindowProc(hWindow, uMsg, wParam, lParam);
	}
	
	return NULL;
}

LRESULT CB::InputProcessKB(int nCode, WPARAM wParam, LPARAM lParam)
{
	//Log::Debug(_XS("low level keyboard hook called."));

	if (nCode == HC_ACTION)
		G::P::Input->on_keyboard_msg(nCode, wParam, lParam);
	
	return CallNextHookEx(G::H::Keyboard, nCode, wParam, lParam);
}

LRESULT CB::InputProcessM(int nCode, WPARAM wParam, LPARAM lParam)
{
	//Log::Debug(_XS("low level mouse hook called."));

	if (nCode == HC_ACTION)
		G::P::Input->on_mouse_msg(nCode, wParam, lParam);

	return CallNextHookEx(G::H::Keyboard, nCode, wParam, lParam);
}
