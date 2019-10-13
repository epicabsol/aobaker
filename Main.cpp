#include <Windows.h>

#include "Window.h"
#include "Renderer.h"
#include "AOBaker.h"
#include "BakeEngine.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* cmdLine, int nCmdShow)
{
	Window::Register(L"benji AOBaker", hInstance);

	Renderer::Initialize(Window::GetClientWidth(), Window::GetClientHeight());

	Initialize();

	BakeEngine::Init();

	Window::Show();
	Window::MessageLoop();

	BakeEngine::Dispose();

	Dispose();

	Renderer::Dispose();

	Window::Dispose();
}