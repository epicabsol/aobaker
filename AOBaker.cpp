#include "AOBaker.h"
#include "Renderer.h"
#include "BakeEngine.h"
#include "Render/RenderShader.h"
#include "Render/RenderMesh.h"
#include "Render/RenderTexture.h"
#include "ImGui/imgui.h"
#include "Window.h"
#include <string>
#include <vector>

using namespace DirectX::SimpleMath;

const std::wstring WorldVertexShaderFilename = L"data/WorldVertexShader.cso";
//const std::wstring ScreenVertexShaderFilename = L"data/ScreenVertexShader.cso";
const std::wstring ColorPixelShaderFilename = L"data/ColorPixelShader.cso";
const std::wstring BakeObjectPixelShaderFilename = L"data/BakeObjectPixelShader.cso";

bool IsLeftMouseDown = false;
bool IsMiddleMouseDown = false;
bool IsRightMouseDown = false;

bool IsKeyDown[256];

float CameraSpeed = 10.0f;

RenderShader* WorldShader = nullptr;
RenderShader* BakeObjectShader = nullptr;

RenderMaterial* TestMaterial = nullptr;

RenderMesh* TestMesh = nullptr;

RenderTexture* TestTexture = nullptr;

Scene* CurrentScene = nullptr;

const D3D11_INPUT_ELEMENT_DESC WorldInputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

RenderVertex TestMeshVertices[] = {
	{ Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 1), Vector4(1, 1, 1, 1) },
	{ Vector3(0, 1, 0), Vector3(0, 0, 1), Vector2(0, 0), Vector4(1, 1, 1, 1) },
	{ Vector3(1, 0, 0), Vector3(0, 0, 1), Vector2(1, 1), Vector4(1, 1, 1, 1) }
};

unsigned int TestMeshIndices[] = {
	0, 1, 2,
	0, 2, 1
};

Scene* GetCurrentScene()
{
	return CurrentScene;
}

RenderShader* GetWorldShader()
{
	return WorldShader;
}

RenderShader* GetBakeObjectShader()
{
	return BakeObjectShader;
}

RenderMaterial* GetTestMaterial()
{
	return TestMaterial;
}

RenderMesh* GetTestMesh()
{
	return TestMesh;
}

RenderTexture* GetTestTexture()
{
	return TestTexture;
}

void ShowImportModel()
{
	OPENFILENAME ofn = { };
	wchar_t path[MAX_PATH];
	wchar_t currentDirectory[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDirectory);
	
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = Window::GetHandle();
	ofn.lpstrFile = path;
	path[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"Wavefront OBJ Files (*.obj)\0*.obj\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrFileTitle = 0;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		BakeObject* newObject = new BakeObject(L"Imported Model");
		newObject->LoadFromFile(ofn.lpstrFile, CurrentScene);
		CurrentScene->AddObject(newObject);
	}

	SetCurrentDirectory(currentDirectory);
}

void Initialize()
{
	// BakeObject shader
	BakeObjectShader = RenderShader::Create(WorldInputElements, ARRAYSIZE(WorldInputElements), WorldVertexShaderFilename, BakeObjectPixelShaderFilename);
	if (BakeObjectShader == nullptr)
	{
		OutputDebugStringW(L"Failed to create BakeObjectShader.");
		DebugBreak();
	}

	TestTexture = RenderTexture::Create(64, 64, DXGI_FORMAT_B8G8R8A8_UNORM, nullptr, 0, true);
	TestTexture->UploadCheckerboard(Renderer::ImmediateContext, 255, 0, 255);

	// Test shader and material
	WorldShader = RenderShader::Create(WorldInputElements, _ARRAYSIZE(WorldInputElements), WorldVertexShaderFilename, ColorPixelShaderFilename);
	if (WorldShader == nullptr)
	{
		OutputDebugStringW(L"Failed to create WorldShader.");
		DebugBreak();
	}
	TestMaterial = new RenderMaterial(BakeObjectShader, &TestTexture, 1);

	// Test rendermesh
	TestMesh = new RenderMesh(Renderer::Device, TestMaterial, PrimitiveType::Triangle, TestMeshVertices, 3, TestMeshIndices, 6);
	if (TestMesh == nullptr)
	{
		OutputDebugStringW(L"Failed to upload test mesh buffer.");
	}

	CurrentScene = new Scene();
	//CurrentScene->AddObject(new BakeObject(L"Object 1"));
	//CurrentScene->AddObject(new BakeObject(L"Object 2"));
	//BakeObject* cube = new BakeObject(L"Cube");
	//cube->LoadFromCube();
	//CurrentScene->AddObject(cube);


	//BakeObject* nomad = new BakeObject(L"Nomad");
	// WIP: Load a 3d model file here
	//nomad->LoadFromFile(L"E:\\Projects\\Modding\\Mass Effect Modding\\Andromeda\\model dumps\\frosty testing\\mako\\mako_static_mesh_LOD0_split.obj", CurrentScene);
	//nomad->LoadFromFile(L"C:\\Users\\codemastrben\\Documents\\LEGO Technic\\Parts\\part6536.obj", CurrentScene);
	//CurrentScene->AddObject(nomad);

	//ImGui::GetStateStorage()->SetInt(ImGui::GetID("Models"), 1);
}

void Update(const float& dT)
{
	Matrix cameraRotation = Matrix::CreateRotationX(-CurrentScene->GetCameraPitch()) * Matrix::CreateRotationY(-CurrentScene->GetCameraYaw());
	Vector3 cameraPosition = CurrentScene->GetCameraPosition();
	
	Vector3 forward = Vector3::TransformNormal(Vector3(0, 0, -1), cameraRotation);
	Vector3 right = Vector3::TransformNormal(Vector3(1, 0, 0), cameraRotation);
	if (IsKeyDown['W'])
	{
		cameraPosition += forward * dT * CameraSpeed;
	}
	if (IsKeyDown['S'])
	{
		cameraPosition -= forward * dT * CameraSpeed;
	}
	if (IsKeyDown['D'])
	{
		cameraPosition += right * dT * CameraSpeed;
	}
	if (IsKeyDown['A'])
	{
		cameraPosition -= right * dT * CameraSpeed;
	}
	if (IsKeyDown[VK_LSHIFT])
	{
		cameraPosition -= Vector3(0, dT * CameraSpeed, 0);
	}
	if (IsKeyDown[VK_SPACE])
	{
		cameraPosition += Vector3(0, dT * CameraSpeed, 0);
	}

	CurrentScene->SetCameraPosition(cameraPosition);
}

float elapsed = 0.0f;
void Render()
{
	elapsed += ImGui::GetIO().DeltaTime;

	Renderer::BeginScene();

	//CurrentScene->SetCameraYaw(elapsed);
	Matrix view = Matrix::CreateTranslation(-CurrentScene->GetCameraPosition()) * Matrix::CreateRotationY(CurrentScene->GetCameraYaw()) * Matrix::CreateRotationX(CurrentScene->GetCameraPitch());
	Renderer::SetView(view);

	//Renderer::Render(TestMesh, DirectX::SimpleMath::Matrix::CreateRotationY(3.1415926535f / 4.0f * elapsed));
	//Renderer::Render(TestMesh, DirectX::SimpleMath::Matrix::Identity);

	CurrentScene->Render();

	ImGui::ShowDemoWindow();

	int menuBarHeight = 0;
	// Menu bar
	if (ImGui::BeginMainMenuBar())
	{
		menuBarHeight = ImGui::GetWindowSize().y;
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "CTRL+N"))
			{

			}
			if (ImGui::MenuItem("Open", "CTRL+O"))
			{

			}
			if (ImGui::MenuItem("Save", "CTRL+S"))
			{

			}
			if (ImGui::MenuItem("Save As...", "CTRL+SHIFT+S"))
			{

			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::MenuItem("Bake"))
			{
				BakeEngine::Bake(CurrentScene);
			}
			if (ImGui::MenuItem("Import Object..."))
			{
				ShowImportModel();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			// TODO: First / Third person, Focus selection, reset, movement speed
			if (ImGui::MenuItem("Style Editor"))
			{
				//ImGui::ShowStyleEditor();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("Overview"))
			{

			}
			if (ImGui::MenuItem("GitLab Repository"))
			{

			}
			if (ImGui::MenuItem("About"))
			{

			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Left Dockpane
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(300, Renderer::GetBufferHeight() - menuBarHeight));
	ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
	if (ImGui::Begin("Left Dockpane", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration))
	{
		ImGui::PopStyleVar(3);

		// Models window: v1
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		//ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Models", ImGuiTreeNodeFlags_SpanFullWidth))
		{
			//ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
			//ImGui::PopStyleVar(2);
			//ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1);
			int objectListHeight = ImGui::CalcTextSize(" ").y;
			if (CurrentScene->GetObjectCount() > 0)
			{
				objectListHeight *= CurrentScene->GetObjectCount();
			}
			//objectListHeight += ImGui::GetFrameHeightWithSpacing();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			if (ImGui::BeginChild("Models List", ImVec2(0, objectListHeight), true))
			{
				CurrentScene->DrawObjectList();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
		}
		else
		{
			//ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
			//ImGui::PopStyleVar(2);
		}
		ImGui::PopStyleVar(2);

		// Models window: v2
		//ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorPos(), ImVec2(ImGui::GetCursorPosX() + 10, 10), )
	}
	else
	{
		ImGui::PopStyleVar(3);
	}
	ImGui::End();


	// Materials window
	ImGui::SetNextWindowSize(ImVec2(300, 0));
	if (ImGui::Begin("Materials"))
	{
		CurrentScene->DrawMaterialList();
	}
	ImGui::End();

	// Properties window
	ImGui::SetNextWindowSize(ImVec2(300, 0));
	if (ImGui::Begin("Properties"))
	{
		CurrentScene->DrawPropertiesGUI();
	}
	ImGui::End();
	//ImGui::Button("Do Thing", ImVec2(300, 60));

	Renderer::EndScene();

	Renderer::Present();
}

void MouseDown(const int& button, const int& x, const int& y)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	if (button == 0)
		IsLeftMouseDown = true;
	else if (button == 1)
		IsMiddleMouseDown = true;
	else if (button == 2)
		IsRightMouseDown = true;
}

int LastMouseX = 0;
int LastMouseY = 0;
void MouseMove(const int& x, const int& y)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	if (IsLeftMouseDown)
	{
		float yaw = CurrentScene->GetCameraYaw();
		yaw += (x - LastMouseX) * 0.007f;
		CurrentScene->SetCameraYaw(yaw);

		float pitch = CurrentScene->GetCameraPitch();
		pitch += (y - LastMouseY) * 0.007f;
		CurrentScene->SetCameraPitch(pitch);
	}


	LastMouseX = x;
	LastMouseY = y;
}

void MouseUp(const int& button, const int& x, const int& y)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
	
	if (button == 0)
		IsLeftMouseDown = false;
	else if (button == 1)
		IsMiddleMouseDown = false;
	else if (button == 2)
		IsRightMouseDown = false;
}

void KeyDown(const int& vkey)
{
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	IsKeyDown[vkey] = true;
}

void KeyUp(const int& vkey)
{
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	IsKeyDown[vkey] = false;
}

void Dispose()
{
	if (CurrentScene != nullptr)
	{
		delete CurrentScene;
		CurrentScene = nullptr;
	}

	if (TestMesh != nullptr)
	{
		delete TestMesh;
		TestMesh = nullptr;
	}

	if (TestMaterial != nullptr)
	{
		delete TestMaterial;
		TestMaterial = nullptr;
	}

	if (TestTexture != nullptr)
	{
		delete TestTexture;
		TestTexture = nullptr;
	}

	if (BakeObjectShader != nullptr)
	{
		delete BakeObjectShader;
		BakeObjectShader = nullptr;
	}

	if (WorldShader != nullptr)
	{
		delete WorldShader;
		WorldShader = nullptr;
	}
}