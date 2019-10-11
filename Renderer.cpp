#include <dxgidebug.h>
#include <d3dcompiler.h>

#include "Renderer.h"
#include "Window.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/ImGuizmo.h"
#include "Render/ConstantBuffer.h"

//using namespace DirectX::SimpleMath;

const unsigned int BufferCount = 2;
const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
const size_t UploadBufferSize = 1024 * 1024 * 30; // 30MB
const size_t MaxTextureCount = 32;

int BufferWidth = 0;
int BufferHeight = 0;

IDXGISwapChain* Renderer::SwapChain = nullptr;
ID3D11Device* Renderer::Device = nullptr;
ID3D11DeviceContext* Renderer::ImmediateContext = nullptr;

RenderTexture* Backbuffer = nullptr;
RenderTexture* DepthStencilBuffer = nullptr;

ID3D11DepthStencilState* StandardDepthStencilState = nullptr;
ID3D11RasterizerState* StandardRasterizerState = nullptr;
ID3D11SamplerState* StandardSamplerState = nullptr;

DirectX::SimpleMath::Matrix View = DirectX::SimpleMath::Matrix::Identity;
DirectX::SimpleMath::Matrix Projection = DirectX::SimpleMath::Matrix::Identity;

struct WorldConstants
{
	DirectX::SimpleMath::Matrix Model = { };
	DirectX::SimpleMath::Matrix View = { };
	DirectX::SimpleMath::Matrix Projection = { };
};
ConstantBuffer<WorldConstants>* WorldConstantBuffer = nullptr;

HRESULT InitializeBuffers(const int& width, const int& height)
{
	BufferWidth = width;
	BufferHeight = height;

	HRESULT hr = S_OK;

	ID3D11Texture2D* backbuffer = nullptr;
	ID3D11RenderTargetView* rtv = nullptr;
	hr = Renderer::SwapChain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
	hr = Renderer::Device->CreateRenderTargetView(backbuffer, nullptr, &rtv);
	Backbuffer = new RenderTexture(width, height, DXGI_FORMAT_B8G8R8A8_UNORM, backbuffer, nullptr, rtv, nullptr);

	ID3D11Texture2D* depthStencilBuffer = nullptr;
	ID3D11DepthStencilView* dsv = nullptr;
	D3D11_TEXTURE2D_DESC dsvdesc = { };
	dsvdesc.ArraySize = 1;
	dsvdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsvdesc.CPUAccessFlags = 0;
	dsvdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvdesc.Height = height;
	dsvdesc.Width = width;
	dsvdesc.MipLevels = 1;
	dsvdesc.SampleDesc.Count = 1;
	dsvdesc.Usage = D3D11_USAGE_DEFAULT;
	hr = Renderer::Device->CreateTexture2D(&dsvdesc, nullptr, &depthStencilBuffer);
	hr = Renderer::Device->CreateDepthStencilView(depthStencilBuffer, nullptr, &dsv);
	DepthStencilBuffer = new RenderTexture(width, height, dsvdesc.Format, depthStencilBuffer, nullptr, nullptr, dsv);

	D3D11_VIEWPORT viewport = { };
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Renderer::ImmediateContext->RSSetViewports(1, &viewport);

	return hr;
}

void DisposeBuffers()
{
	delete DepthStencilBuffer;
	DepthStencilBuffer = nullptr;
	delete Backbuffer;
	Backbuffer = nullptr;
}

IDXGIAdapter* GetBestAdapter(IDXGIFactory* const factory)
{
	IDXGIAdapter* result = nullptr;

	IDXGIAdapter* adapter = nullptr;
	size_t vram = 0;
	int i = 0;
	while (factory->EnumAdapters(i++, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adapterDesc = {};
		adapter->GetDesc(&adapterDesc);
		if (adapterDesc.DedicatedVideoMemory > vram)
		{
			vram = adapterDesc.DedicatedVideoMemory;
			result = adapter;
		}
	}

	return result;
}

HRESULT Renderer::Initialize(const int& bufferWidth, const int& bufferHeight)
{
	BufferWidth = bufferWidth;
	BufferHeight = bufferHeight;

	HRESULT hr = S_OK;

	IDXGIFactory* factory;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		return hr;
	}

	IDXGIAdapter* adapter = GetBestAdapter(factory);
	if (adapter == nullptr)
	{
		OutputDebugStringW(L"Couldn't find a good adapter.");
		return E_FAIL;
	}

	DXGI_ADAPTER_DESC adapterDesc = { };
	adapter->GetDesc(&adapterDesc);
	OutputDebugStringW(L"Using adapter '");
	OutputDebugStringW(adapterDesc.Description);
	OutputDebugStringW(L"'\n");

	DXGI_SWAP_CHAIN_DESC scdesc = { };
	scdesc.BufferCount = 2;
	scdesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scdesc.BufferDesc.Height = bufferHeight;
	scdesc.BufferDesc.Width = bufferWidth;
	scdesc.BufferDesc.RefreshRate.Numerator = 75;
	scdesc.BufferDesc.RefreshRate.Denominator = 1;
	scdesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scdesc.Flags = 0;
	scdesc.OutputWindow = Window::GetHandle();
	scdesc.SampleDesc.Count = 1;
	scdesc.SampleDesc.Quality = 0;
	scdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scdesc.Windowed = true;
	unsigned int createFlags = 0;
#ifdef _DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, nullptr, 0, D3D11_SDK_VERSION, &scdesc, &SwapChain, &Device, nullptr, &ImmediateContext);
	if (FAILED(hr))
	{
		MessageBoxA(0, "Couldn't create D3D device!", "D3D Failure", 0);
		return hr;
	}

	WorldConstantBuffer = ConstantBuffer<WorldConstants>::Create(Device, WorldConstants(), true);
	if (WorldConstantBuffer == nullptr)
	{
		OutputDebugStringW(L"Failed to create the world constants buffer.");
		return E_FAIL;
	}

	hr = InitializeBuffers(bufferWidth, bufferHeight);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to initialize buffers.");
		return hr;
	}

	D3D11_DEPTH_STENCIL_DESC dsdesc = { };
	dsdesc.StencilEnable = false;
	dsdesc.DepthEnable = true;
	dsdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	hr = Device->CreateDepthStencilState(&dsdesc, &StandardDepthStencilState);

	D3D11_RASTERIZER_DESC rsdesc = { };
	rsdesc.FillMode = D3D11_FILL_SOLID;
	rsdesc.FrontCounterClockwise = true;
	rsdesc.CullMode = D3D11_CULL_BACK;
	hr = Device->CreateRasterizerState(&rsdesc, &StandardRasterizerState);

	D3D11_SAMPLER_DESC sdesc = { };
	sdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hr = Device->CreateSamplerState(&sdesc, &StandardSamplerState);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(Window::GetHandle());
	ImGui_ImplDX11_Init(Device, ImmediateContext);
	ImGui_ImplDX11_CreateDeviceObjects();

	ImGui::StyleColorsDark();

	return hr;
}

HRESULT Renderer::Resize(const int& bufferWidth, const int& bufferHeight)
{
	if (Device != nullptr)
	{
		HRESULT hr = S_OK;

		DisposeBuffers();

		hr = SwapChain->ResizeBuffers(0, bufferWidth, bufferHeight, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to resize the swap chain.");
			return hr;
		}

		hr = InitializeBuffers(bufferWidth, bufferHeight);
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to recreate the swap chain.");
			return hr;
		}

		ImGuizmo::SetRect(0, 0, bufferWidth, bufferHeight);

		return hr;
	}
	else
	{
		return S_FALSE;
	}
}

const float ClearColor[] = { 100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f };

void Renderer::BeginScene()
{
	// Reset ImGui
	//ImGui_ImplDX12_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();


	float color[]{ 0.0f, 1.0f, 0.0f, 1.0f };
	ImmediateContext->ClearRenderTargetView(Backbuffer->GetRTV(), color);
	ImmediateContext->ClearDepthStencilView(DepthStencilBuffer->GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	Projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(3.1415926535f/3.0f, (float)Window::GetClientWidth() / Window::GetClientHeight(), 0.5f, 1000.0f);
	WorldConstantBuffer->Update(ImmediateContext, WorldConstants { DirectX::SimpleMath::Matrix(), View, Projection });

	ID3D11RenderTargetView* rtv = Backbuffer->GetRTV();
	ImmediateContext->OMSetRenderTargets(1, &rtv, DepthStencilBuffer->GetDSV());
	ImmediateContext->OMSetDepthStencilState(StandardDepthStencilState, 0);
	ImmediateContext->RSSetState(StandardRasterizerState);
}

void Renderer::EndScene()
{

}

void Renderer::Present()
{
	// Finish off ImGui
	ImGui::Render();
	//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	SwapChain->Present(1, 0);
}

void Renderer::Dispose()
{
	DisposeBuffers();

	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_Shutdown();

	delete WorldConstantBuffer;

	StandardDepthStencilState->Release();
	StandardDepthStencilState = nullptr;

	StandardRasterizerState->Release();
	StandardRasterizerState = nullptr;

	StandardSamplerState->Release();
	StandardSamplerState = nullptr;

	SwapChain->Release();
	SwapChain = nullptr;

	ImmediateContext->Release();
	ImmediateContext = nullptr;

	Device->Release();
	Device = nullptr;

	// Report the objects that haven't been released, if dxgidebug.dll is present
	HMODULE dxgidebug = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (dxgidebug)
	{
		typedef HRESULT(WINAPI * LPDXGIGETDEBUGINTERFACE)(REFIID, void**);
		LPDXGIGETDEBUGINTERFACE dxgiGetDebugInterface = (LPDXGIGETDEBUGINTERFACE)GetProcAddress(dxgidebug, "DXGIGetDebugInterface");

		IDXGIDebug* dxgiDebug = nullptr;
		HRESULT hr = dxgiGetDebugInterface(IID_PPV_ARGS(&dxgiDebug));
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to get the DXGI debug instance.");
		}
		else
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
	}
}

ID3DBlob* Renderer::ReadBlobFromFile(const std::wstring& filename)
{
	HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		OutputDebugStringW(L"Couldn't open file to read blob!");
		return nullptr;
	}

	unsigned long int fileSize = 0;

	LARGE_INTEGER sizeResult = { };
	if (!GetFileSizeEx(hFile, &sizeResult))
	{
		OutputDebugStringW(L"Couldn't get size of file to read blob!");
		CloseHandle(hFile);
		return nullptr;
	}

	fileSize = (unsigned long int)sizeResult.QuadPart;

	ID3DBlob* result = nullptr;
	HRESULT hr = D3DCreateBlob((size_t)fileSize, &result);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Couldn't create blob for reading file!");
		CloseHandle(hFile);
		return nullptr;
	}

	unsigned long bytesRead = 0;
	bool success = ReadFile(hFile, result->GetBufferPointer(), fileSize, &bytesRead, nullptr);
	if (!success)
	{
		OutputDebugStringW(L"Couldn't read from file into blob!");
		CloseHandle(hFile);
		result->Release();
		return nullptr;
	}

	if (bytesRead != fileSize)
	{
		OutputDebugStringW(L"WARNING: Read a different number of bytes than the file length!");
	}

	return result;
}

void Renderer::SetView(const DirectX::SimpleMath::Matrix& view)
{
	View = view;
}

DirectX::SimpleMath::Matrix Renderer::GetView()
{
	return View;
}

DirectX::SimpleMath::Matrix Renderer::GetProjection()
{
	return Projection;
}

void Renderer::Render(RenderMesh* const mesh, const DirectX::SimpleMath::Matrix& transform)
{
	WorldConstants worldConstants = { };
	worldConstants.Projection = Projection;
	worldConstants.Model = transform;
	worldConstants.View = View;

	WorldConstantBuffer->Update(ImmediateContext, worldConstants);

	/*CommandList->SetGraphicsRootSignature(mesh->GetMaterial()->GetShader()->GetSignature());
	WorldConstantBuffer->Apply(CommandList, worldConstants, 0, CurrentFrameID);
	CommandList->IASetPrimitiveTopology(mesh->GetPrimitiveType() == PrimitiveType::Triangle ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST : D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	CommandList->IASetIndexBuffer(mesh->GetIndexBufferView());
	CommandList->IASetVertexBuffers(0, 1, mesh->GetVertexBufferView());
	CommandList->SetPipelineState(mesh->GetMaterial()->GetShader()->GetPipelineState());

	for (int i = 0; i < mesh->GetMaterial()->GetTextureCount(); i++)
	{
		//CommandList->SetGraphicsRootShaderResourceView(i + 1, mesh->GetMaterial()->GetTextures()[i]->GetGPUVirtualAddress());
		D3D12_GPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = Renderer::SRVHeap->GetGPUDescriptorHandleForHeapStart().ptr + mesh->GetMaterial()->GetTextures()[i]->GetIndex() * RTVIncrementSize;
		CommandList->SetGraphicsRootDescriptorTable(1, handle);

	}

	CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);*/

	ImmediateContext->IASetIndexBuffer(mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer();
	UINT stride = sizeof(RenderVertex);
	UINT offset = 0;
	ImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	ImmediateContext->IASetInputLayout(mesh->GetMaterial()->GetShader()->GetInputLayout());
	ImmediateContext->IASetPrimitiveTopology(mesh->GetPrimitiveType() == PrimitiveType::Triangle ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST : D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	
	ImmediateContext->VSSetShader(mesh->GetMaterial()->GetShader()->GetVertexShader(), nullptr, 0);
	ID3D11Buffer* constantBuffer = WorldConstantBuffer->GetBuffer();
	ImmediateContext->VSSetConstantBuffers(0, 1, &constantBuffer);

	ImmediateContext->PSSetShader(mesh->GetMaterial()->GetShader()->GetPixelShader(), nullptr, 0);
	ID3D11ShaderResourceView* texture = nullptr;
	for (int i = 0; i < mesh->GetMaterial()->GetTextureCount(); i++)
	{
		texture = mesh->GetMaterial()->GetTextures()[i]->GetSRV();
		ImmediateContext->PSSetShaderResources(i, 1, &texture);
		ImmediateContext->PSSetSamplers(i, 1, &StandardSamplerState);
	}

	ImmediateContext->DrawIndexed(mesh->GetIndexCount(), 0, 0);
}