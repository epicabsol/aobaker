#include <windowsx.h> // For GET_X_LPARAM and GET_Y_LPARAM
#include "AOBaker.h"
#include "Window.h"
#include "Renderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"

const wchar_t* ClassName = L"AOBakerWindow";

HWND Handle = NULL;
HINSTANCE HInstance = NULL;
bool Exit = false;

extern IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}

	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	else if (msg == WM_SIZE)
	{
		if (wParam != SIZE_MINIMIZED)
		{
			Renderer::Resize(Window::GetClientWidth(), Window::GetClientHeight());
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	else if (msg == WM_LBUTTONDOWN)
	{
		MouseDown(0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_MBUTTONDOWN)
	{
		MouseDown(1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_RBUTTONDOWN)
	{
		MouseDown(2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_LBUTTONUP)
	{
		MouseUp(0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_MBUTTONUP)
	{
		MouseUp(1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_RBUTTONUP)
	{
		MouseUp(2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_MOUSEMOVE)
	{
		MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	else if (msg == WM_KEYDOWN)
	{
		KeyDown(wParam);
	}
	else if (msg == WM_KEYUP)
	{
		KeyUp(wParam);
	}
	else 
	{
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

HWND Window::GetHandle()
{
	return Handle;
}

int Window::GetClientWidth()
{
	RECT r = { };
	GetClientRect(GetHandle(), &r);
	return r.right - r.left;
}

int Window::GetClientHeight()
{
	RECT r = {};
	GetClientRect(GetHandle(), &r);
	return r.bottom - r.top;
}

int Window::GetWidth()
{
	RECT r = { };
	GetWindowRect(GetHandle(), &r);
	return r.right - r.left;
}

int Window::GetHeight()
{
	RECT r = { };
	GetWindowRect(GetHandle(), &r);
	return r.bottom - r.top;
}

HRESULT Window::Register(const wchar_t* const titleText, const HINSTANCE& hInstance)
{
	HRESULT hr = S_OK;

	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = ClassName;

	RegisterClass(&wc);

	Handle = CreateWindow(ClassName, titleText, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, nullptr);
	if (GetHandle() == NULL)
	{
		return E_FAIL;
	}

	HInstance = hInstance;

	return hr;
}

void Window::Show()
{
	ShowWindow(GetHandle(), SW_SHOW);
}

LARGE_INTEGER LastCounterValue = { };
LARGE_INTEGER CounterFrequency = { };
void Window::MessageLoop()
{
	QueryPerformanceFrequency(&CounterFrequency);

	MSG msg = {};
	float dt = 0.0f;
	LARGE_INTEGER counterValue = { };
	while (!Exit)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				Exit = true;
				break;
			}
		}

		QueryPerformanceCounter(&counterValue);
		if (LastCounterValue.QuadPart > 0)
		{
			dt = (counterValue.QuadPart - LastCounterValue.QuadPart) / (float)CounterFrequency.QuadPart;
		}
		LastCounterValue = counterValue;
		Update(dt);
		Render();
	}
}

void Window::Dispose()
{
	UnregisterClass(ClassName, HInstance);
}