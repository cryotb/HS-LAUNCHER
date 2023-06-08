#pragma once

namespace CB
{
	extern LRESULT CALLBACK WindowProcess(_In_ HWND hWindow, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
	extern LRESULT CALLBACK InputProcessKB(int nCode, WPARAM wParam, LPARAM lParam);
	extern LRESULT CALLBACK InputProcessM(int nCode, WPARAM wParam, LPARAM lParam);
}
