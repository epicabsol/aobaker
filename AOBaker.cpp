#include "AOBaker.h"
#include "Renderer.h"
#include "Render/RenderShader.h"
#include "Render/RenderMesh.h"
#include "ImGui/imgui.h"
#include "Window.h"
#include <string>
#include <vector>

using namespace DirectX::SimpleMath;

const std::wstring WorldVertexShaderFilename = L"data/WorldVertexShader.cso";
const std::wstring ScreenVertexShaderFilename = L"data/ScreenVertexShader.cso";
const std::wstring ColorPixelShaderFilename = L"data/ColorPixelShader.cso";

bool IsLeftMouseDown = false;
bool IsMiddleMouseDown = false;
bool IsRightMouseDown = false;

bool IsKeyDown[256];

float CameraSpeed = 10.0f;

RenderShader* WorldShader = nullptr;

RenderMaterial* TestMaterial = nullptr;

RenderMesh* TestMesh = nullptr;

Scene* CurrentScene = nullptr;

const D3D12_INPUT_ELEMENT_DESC WorldInputElements[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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

RenderMaterial* GetTestMaterial()
{
	return TestMaterial;
}

RenderMesh* GetTestMesh()
{
	return TestMesh;
}

void Initialize()
{
	// World shader
	D3D12_ROOT_PARAMETER1 worldRPs[1];

	// Direct CBV
	worldRPs[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	worldRPs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	worldRPs[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	worldRPs[0].Descriptor.RegisterSpace = 0;
	worldRPs[0].Descriptor.ShaderRegister = 0;

	// CBV table
	/*D3D12_DESCRIPTOR_RANGE1 worldConstantsRange = { };
	worldConstantsRange.BaseShaderRegister = 0;
	worldConstantsRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	worldConstantsRange.NumDescriptors = 1;
	worldConstantsRange.OffsetInDescriptorsFromTableStart = 0;
	worldConstantsRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	worldConstantsRange.RegisterSpace = 0;

	worldRPs[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	worldRPs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	worldRPs[0].DescriptorTable.NumDescriptorRanges = 1;
	worldRPs[0].DescriptorTable.pDescriptorRanges = &worldConstantsRange;*/

	// Direct constants
	/*worldRPs[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	worldRPs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	worldRPs[0].Constants.Num32BitValues = 16 * 3; // 3 float4x4 matrices
	worldRPs[0].Constants.RegisterSpace = 0;
	worldRPs[0].Constants.ShaderRegister = 0;*/

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC worldDesc = { };
	worldDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	worldDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	worldDesc.Desc_1_1.NumParameters = 1;
	worldDesc.Desc_1_1.pParameters = worldRPs;
	worldDesc.Desc_1_1.NumStaticSamplers = 0;
	worldDesc.Desc_1_1.pStaticSamplers = nullptr;

	WorldShader = RenderShader::Create(worldDesc, WorldVertexShaderFilename, ColorPixelShaderFilename, WorldInputElements, _ARRAYSIZE(WorldInputElements));
	if (WorldShader == nullptr)
	{
		OutputDebugStringW(L"Failed to create WorldShader.");
		DebugBreak();
	}

	// Test material
	TestMaterial = new RenderMaterial(WorldShader, nullptr, 0);

	// Test rendermesh
	TestMesh = new RenderMesh(TestMaterial, PrimitiveType::Triangle, TestMeshVertices, 3, TestMeshIndices, 6);
	HRESULT hr = TestMesh->UploadBuffers();
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to upload test mesh buffer.");
	}

	CurrentScene = new Scene();
	//CurrentScene->AddObject(new BakeObject(L"Object 1"));
	//CurrentScene->AddObject(new BakeObject(L"Object 2"));
	//BakeObject* cube = new BakeObject(L"Cube");
	//cube->LoadFromCube();
	//CurrentScene->AddObject(cube);
	BakeObject* nomad = new BakeObject(L"Nomad");
	nomad->LoadFromFile(L"D:\\codemastrben\\Documents\\Projects\\Modding\\Mass Effect Modding\\Andromeda\\model dumps\\frosty testing\\mako\\mako_static_mesh_LOD0.obj");
	CurrentScene->AddObject(nomad);
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

	// Menu bar
	if (ImGui::BeginMainMenuBar())
	{
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
		if (ImGui::BeginMenu("View"))
		{
			// TODO: First / Third person, Focus selection, reset, movement speed
			if (ImGui::MenuItem("Style Editor"))
			{
				ImGui::ShowStyleEditor();
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

	// Models window
	//ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f, Window::GetClientHeight()), ImVec2(1000.0f, Window::GetClientHeight()));
	if (ImGui::Begin("Models"))
	{
		CurrentScene->DrawObjectList();
	}
	ImGui::End();

	// Properties window
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
		TestMesh->Dispose();
		delete TestMesh;
		TestMesh = nullptr;
	}

	if (TestMaterial != nullptr)
	{
		delete TestMaterial;
		TestMaterial = nullptr;
	}

	if (WorldShader != nullptr)
	{
		WorldShader->Dispose();
		delete WorldShader;
		WorldShader = nullptr;
	}
}