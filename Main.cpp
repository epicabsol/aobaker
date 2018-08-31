#include <Windows.h>

#include "Window.h"
#include "Renderer.h"
#include "AOBaker.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* cmdLine, int nCmdShow)
{
	Window::Register(L"benji AOBaker", hInstance);

	Renderer::Initialize(Window::GetClientWidth(), Window::GetClientHeight());

	Initialize();

	Window::Show();
	Window::MessageLoop();

	Dispose();

	Renderer::Dispose();

	Window::Dispose();
}