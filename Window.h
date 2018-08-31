#pragma once
#include <Windows.h>

// Handles the Win32 side of the AOBaker window.

namespace Window
{
	HWND GetHandle();

	int GetClientWidth();
	int GetClientHeight();
	int GetWidth();
	int GetHeight();

	HRESULT Register(const wchar_t* const titleText, const HINSTANCE& hInstance);

	void Show();

	void MessageLoop();

	void Dispose();
}

