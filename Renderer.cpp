#include <dxgidebug.h>
#include <d3dcompiler.h>

#include "Renderer.h"
#include "Window.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx12.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/ImGuizmo.h"
#include "Render/ConstantBuffer.h"

//using namespace DirectX::SimpleMath;

const unsigned int BufferCount = 2;
const bool UseSoftwareRenderer = false;
const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
const size_t UploadBufferSize = 1024 * 1024 * 30; // 30MB
const size_t MaxTextureCount = 32;

int BufferWidth = 0;
int BufferHeight = 0;

D3D12_VIEWPORT Viewport = { };
D3D12_RECT ScissorRect = { };

IDXGISwapChain3* Renderer::SwapChain = nullptr;
ID3D12Device* Renderer::Device = nullptr;
ID3D12Resource* Backbuffers[BufferCount] = { };
ID3D12Resource* DepthStencilBuffer = nullptr;
ID3D12CommandAllocator* Renderer::CommandAllocator = nullptr;
ID3D12CommandQueue* Renderer::CommandQueue = nullptr;
ID3D12DescriptorHeap* Renderer::RTVHeap = nullptr;
ID3D12DescriptorHeap* Renderer::DSVHeap = nullptr;
ID3D12DescriptorHeap* Renderer::SRVHeap = nullptr;
ID3D12GraphicsCommandList* CommandList = nullptr;
ID3D12Resource* UploadBuffer = nullptr;

unsigned int BufferIndex = 0;
unsigned int RTVIncrementSize = 0;
unsigned int SRVIncrementSize = 0;
size_t NextTextureIndex = 0;
size_t ImGuiFontTextureIndex = 0;

ID3D12Fence* FrameCompleteFence = nullptr;
unsigned long long CurrentFrameID = 1;

HANDLE FenceEvent = NULL;
ID3D12Fence* Fence = nullptr;
unsigned long long int FenceValue = 0;

HANDLE UploadFenceEvent = NULL;
ID3D12Fence* UploadFence = nullptr;
unsigned long long int UploadFenceValue = 0;

DirectX::SimpleMath::Matrix View = DirectX::SimpleMath::Matrix::Identity;
DirectX::SimpleMath::Matrix Projection = DirectX::SimpleMath::Matrix::Identity;

struct WorldConstants
{
	DirectX::SimpleMath::Matrix Model = { };
	DirectX::SimpleMath::Matrix View = { };
	DirectX::SimpleMath::Matrix Projection = { };
};
ConstantBuffer<WorldConstants>* WorldConstantBuffer = nullptr;

void DumbWait()
{
	const unsigned long long int fence = FenceValue;
	HRESULT hr = Renderer::CommandQueue->Signal(Fence, fence);
	FenceValue++;

	if (Fence->GetCompletedValue() < fence)
	{
		Fence->SetEventOnCompletion(fence, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

HRESULT InitializeBuffers(const int& width, const int& height)
{
	BufferWidth = width;
	BufferHeight = height;

	HRESULT hr = S_OK;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(Renderer::RTVHeap->GetCPUDescriptorHandleForHeapStart());
	for (unsigned int i = 0; i < BufferCount; i++)
	{
		hr = Renderer::SwapChain->GetBuffer(i, IID_PPV_ARGS(&Backbuffers[i]));
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to get buffer from swap chain.");
			return hr;
		}
		Renderer::Device->CreateRenderTargetView(Backbuffers[i], nullptr, rtvHandle);
		rtvHandle.ptr += UINT64(RTVIncrementSize);
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc = { };
	dsvdesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	D3D12_CLEAR_VALUE dsvclear = { };
	dsvclear.Format = DXGI_FORMAT_D32_FLOAT;
	dsvclear.DepthStencil.Depth = 1.0f;
	dsvclear.DepthStencil.Stencil = 0;
	D3D12_HEAP_PROPERTIES dsvheap = { };
	dsvheap.Type = D3D12_HEAP_TYPE_DEFAULT;
	dsvheap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	dsvheap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	dsvheap.CreationNodeMask = 0;
	dsvheap.VisibleNodeMask = 0;
	D3D12_RESOURCE_DESC resdesc = { };
	resdesc.Alignment = 0;
	resdesc.DepthOrArraySize = 1;
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resdesc.Format = DXGI_FORMAT_D32_FLOAT;
	resdesc.Height = BufferHeight;
	resdesc.Width = BufferWidth;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resdesc.MipLevels = 1;
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	hr = Renderer::Device->CreateCommittedResource(&dsvheap, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &dsvclear, IID_PPV_ARGS(&DepthStencilBuffer));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create the depth-stencil buffer.");
		return hr;
	}
	Renderer::Device->CreateDepthStencilView(DepthStencilBuffer, &dsvdesc, Renderer::DSVHeap->GetCPUDescriptorHandleForHeapStart());

	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = (float)width;
	Viewport.Height = (float)height;
	Viewport.MinDepth = 0;
	Viewport.MaxDepth = 1.0f;

	ScissorRect.left = 0;
	ScissorRect.right = width;
	ScissorRect.top = 0;
	ScissorRect.bottom = height;

	return hr;
}

void DisposeBuffers()
{
	for (unsigned int i = 0; i < BufferCount; i++)
	{
		Backbuffers[i]->Release();
		Backbuffers[i] = nullptr;
	}

	DepthStencilBuffer->Release();
}

IDXGIAdapter1* GetBestAdapter(IDXGIFactory4* const factory)
{
	IDXGIAdapter1* result = nullptr;

	IDXGIAdapter1* adapter = nullptr;
	size_t vram = 0;
	int i = 0;
	while (factory->EnumAdapters1(i++, &adapter) != DXGI_ERROR_NOT_FOUND)
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

#ifdef _DEBUG
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif

	IDXGIFactory4* factory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		return hr;
	}

	if (UseSoftwareRenderer)
	{
		IDXGIAdapter* warpAdapter = nullptr;
		hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
		if (FAILED(hr))
		{
			return hr;
		}

		hr = D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to create WARP device.");
			return hr;
		}
	}
	else
	{
		IDXGIAdapter1* adapter = GetBestAdapter(factory);
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

		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Failed to create hardware device.");
			return hr;
		}
	}

	// Command queue
	D3D12_COMMAND_QUEUE_DESC cmddesc = { };
	cmddesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmddesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = Device->CreateCommandQueue(&cmddesc, IID_PPV_ARGS(&CommandQueue));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create the command queue.");
		return hr;
	}

	// Swap chain
	DXGI_SWAP_CHAIN_DESC scdesc = { };
	scdesc.BufferCount = BufferCount;
	scdesc.BufferDesc.Width = BufferWidth;
	scdesc.BufferDesc.Height = BufferHeight;
	scdesc.BufferDesc.Format = BufferFormat;
	scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scdesc.OutputWindow = Window::GetHandle();
	scdesc.SampleDesc.Count = 1;
	scdesc.Windowed = true;

	IDXGISwapChain* legacySwapChain = nullptr;
	hr = factory->CreateSwapChain(CommandQueue, &scdesc, &legacySwapChain);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create swap chain.");
		return hr;
	}
	hr = legacySwapChain->QueryInterface(&SwapChain);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to obtain SwapChain3.");
		return hr;
	}
	legacySwapChain->Release();

	BufferIndex = SwapChain->GetCurrentBackBufferIndex();

	// Descriptor heaps
	D3D12_DESCRIPTOR_HEAP_DESC heapdesc = { };
	heapdesc.NumDescriptors = BufferCount;
	heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = Device->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&RTVHeap));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create the RTV descriptor heap.");
		return hr;
	}
	RTVHeap->SetName(L"Render Target View Heap");
	RTVIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	heapdesc = { };
	heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapdesc.NumDescriptors = 1;
	heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = Device->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&DSVHeap));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create the DSV descriptor heap.");
		return hr;
	}
	DSVHeap->SetName(L"Depth Stencil View Heap");

	heapdesc = { };
	heapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapdesc.NumDescriptors = MaxTextureCount;
	heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = Device->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&SRVHeap));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create the SRV descriptor heap.");
		return hr;
	}
	SRVHeap->SetName(L"Shader Resource View Heap");

	// Frame resources (RTVs)
	hr = InitializeBuffers(bufferWidth, bufferHeight);
	if (FAILED(hr))
	{
		return hr;
	}

	// Command allocator
	hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create command allocator.");
		return hr;
	}

	// Command list
	hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create the command list.");
		return hr;
	}
	CommandList->Close();

	WorldConstantBuffer = ConstantBuffer<WorldConstants>::Create();
	if (WorldConstantBuffer == nullptr)
	{
		OutputDebugStringW(L"Failed to create the world constants buffer.");
		return E_FAIL;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(Window::GetHandle());
	ImGui_ImplDX12_Init(Device, BufferCount, BufferFormat, SRVHeap->GetCPUDescriptorHandleForHeapStart(), SRVHeap->GetGPUDescriptorHandleForHeapStart());
	ImGuiFontTextureIndex = AllocateTextureIndex();

	ImGui::StyleColorsDark();

	// Fencing
	hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create fence.");
		return hr;
	}
	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (FenceEvent == nullptr)
	{
		OutputDebugStringW(L"Failed to create fence event.");
		return E_FAIL;
	}
	hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FrameCompleteFence));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create frame complete fence.");
		return hr;
	}

	// Upload system
	hr = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&UploadFence));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create upload fence.");
		return hr;
	}
	UploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (UploadFenceEvent == nullptr)
	{
		OutputDebugStringW(L"Failed to create upload fence event.");
		return hr;
	}
	D3D12_HEAP_PROPERTIES uploadProps = { };
	uploadProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadProps.CreationNodeMask = 0;
	uploadProps.VisibleNodeMask = 0;
	uploadProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC uploadDesc = { };
	uploadDesc.Alignment = 0;
	uploadDesc.DepthOrArraySize = 1;
	uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadDesc.Height = 1;
	uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadDesc.MipLevels = 1;
	uploadDesc.SampleDesc.Count = 1;
	uploadDesc.SampleDesc.Quality = 0;
	uploadDesc.Width = UploadBufferSize;
	hr = Device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadBuffer));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create upload buffer.");
		return hr;
	}

	DumbWait();

	return hr;
}

HRESULT Renderer::Resize(const int& bufferWidth, const int& bufferHeight)
{
	if (Device != nullptr)
	{
		HRESULT hr = S_OK;

		ImGui_ImplDX12_InvalidateDeviceObjects();
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

		ImGui_ImplDX12_CreateDeviceObjects();

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
	unsigned long long lastCompletedFrameID = FrameCompleteFence->GetCompletedValue();
	WorldConstantBuffer->ReleaseFrame(lastCompletedFrameID);

	// Reset ImGui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();

	BufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CommandAllocator->Reset();
	D3D12_RESOURCE_BARRIER bardesc = {};
	bardesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bardesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	bardesc.Transition.pResource = Backbuffers[BufferIndex];
	bardesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	bardesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	bardesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	CommandList->Reset(CommandAllocator, nullptr);
	//CommandList->SetGraphicsRootSignature(RootSignature);
	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);
	CommandList->ResourceBarrier(1, &bardesc);

	D3D12_CPU_DESCRIPTOR_HANDLE backbuffer = { };
	backbuffer.ptr = RTVHeap->GetCPUDescriptorHandleForHeapStart().ptr + BufferIndex * RTVIncrementSize;

	D3D12_CPU_DESCRIPTOR_HANDLE depthstencilview = { };
	depthstencilview.ptr = DSVHeap->GetCPUDescriptorHandleForHeapStart().ptr;

	CommandList->OMSetRenderTargets(1, &backbuffer, false, &depthstencilview);
	CommandList->ClearRenderTargetView(backbuffer, ClearColor, 0, nullptr);
	CommandList->ClearDepthStencilView(depthstencilview, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	CommandList->SetDescriptorHeaps(1, &SRVHeap);

	Projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(3.1415926535f/3.0f, (float)Window::GetClientWidth() / Window::GetClientHeight(), 0.5f, 1000.0f);
}

void Renderer::EndScene()
{

}

void Renderer::Present()
{
	// Finish off ImGui
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);

	D3D12_RESOURCE_BARRIER bardesc = {};
	bardesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bardesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	bardesc.Transition.pResource = Backbuffers[BufferIndex];
	bardesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	bardesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	bardesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	CommandList->ResourceBarrier(1, &bardesc);
	CommandList->Close();

	ID3D12CommandList* commandLists[] = { CommandList };

	CommandQueue->ExecuteCommandLists(1, commandLists);
	CommandQueue->Signal(FrameCompleteFence, CurrentFrameID);

	SwapChain->Present(1, 0);

	DumbWait();

	CurrentFrameID++;
}

void Renderer::Dispose()
{
	DumbWait();

	UploadBuffer->Release();
	UploadBuffer = nullptr;

	CloseHandle(UploadFenceEvent);

	FrameCompleteFence->Release();
	FrameCompleteFence = nullptr;

	UploadFence->Release();
	UploadFence = nullptr;

	CloseHandle(FenceEvent);

	Fence->Release();
	Fence = nullptr;

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (WorldConstantBuffer != nullptr)
	{
		WorldConstantBuffer->Dispose();
		delete WorldConstantBuffer;
		WorldConstantBuffer = nullptr;
	}

	CommandAllocator->Release();
	CommandAllocator = nullptr;

	DisposeBuffers();

	SRVHeap->Release();
	SRVHeap = nullptr;

	DSVHeap->Release();
	DSVHeap = nullptr;

	RTVHeap->Release();
	RTVHeap = nullptr;

	SwapChain->Release();
	SwapChain = nullptr;

	CommandList->Release();
	CommandList = nullptr;

	CommandQueue->Release();
	CommandQueue = nullptr;

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
	hr = ReadFile(hFile, result->GetBufferPointer(), fileSize, &bytesRead, nullptr);
	if (FAILED(hr))
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

HRESULT Renderer::UploadData(ID3D12Resource* const destination, void* const data, const size_t& length)
{
	HRESULT hr = S_OK;
	// Copy to upload buffer
	// ASSUMPTION: The upload buffer is not currently in use by the GPU.
	if (length > UploadBufferSize)
	{
		OutputDebugStringW(L"Failed to upload data: Data length was larger than the upload buffer!");
		return E_OUTOFMEMORY;
	}

	void* UploadBufferData = nullptr;
	D3D12_RANGE uploadBufferReadRange = { };
	uploadBufferReadRange.Begin = 0;
	uploadBufferReadRange.End = 0;
	hr = UploadBuffer->Map(0, &uploadBufferReadRange, &UploadBufferData);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to map upload buffer.");
		return hr;
	}
	memcpy(UploadBufferData, data, length);
	UploadBuffer->Unmap(0, nullptr);
	UploadBufferData = nullptr;

	// Queue the GPU copy
	CommandList->Reset(CommandAllocator, nullptr);
	//CommandList->CopyResource(destination, UploadBuffer);
	CommandList->CopyBufferRegion(destination, 0, UploadBuffer, 0, length);
	CommandList->Close();
	ID3D12CommandList* commandLists[] = { CommandList };
	CommandQueue->ExecuteCommandLists(1, commandLists);

	hr = Renderer::CommandQueue->Signal(UploadFence, UploadFenceValue);

	if (UploadFence->GetCompletedValue() < UploadFenceValue)
	{
		UploadFence->SetEventOnCompletion(UploadFenceValue, UploadFenceEvent);
		WaitForSingleObject(UploadFenceEvent, INFINITE);
	}
	UploadFenceValue++;


	return hr;
}

HRESULT Renderer::UploadTexture(ID3D12Resource* const destination, void* const data, const size_t& length, const int& width, const int& height, const DXGI_FORMAT& format)
{
	HRESULT hr = S_OK;
	// Copy to upload buffer
	// ASSUMPTION: The upload buffer is not currently in use by the GPU.
	if (length > UploadBufferSize)
	{
		OutputDebugStringW(L"Failed to upload data: Data length was larger than the upload buffer!");
		return E_OUTOFMEMORY;
	}

	void* UploadBufferData = nullptr;
	D3D12_RANGE uploadBufferReadRange = { };
	uploadBufferReadRange.Begin = 0;
	uploadBufferReadRange.End = 0;
	hr = UploadBuffer->Map(0, &uploadBufferReadRange, &UploadBufferData);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to map upload buffer.");
		return hr;
	}
	memcpy(UploadBufferData, data, length);
	UploadBuffer->Unmap(0, nullptr);
	UploadBufferData = nullptr;

	// Queue the GPU copy
	CommandList->Reset(CommandAllocator, nullptr);
	//CommandList->CopyResource(destination, UploadBuffer);

	D3D12_RESOURCE_DESC destDesc = destination->GetDesc();

	D3D12_TEXTURE_COPY_LOCATION srcLocation = { };
	srcLocation.PlacedFootprint.Footprint.Depth = 1;
	srcLocation.PlacedFootprint.Footprint.Format = format;
	srcLocation.PlacedFootprint.Footprint.Height = height;
	srcLocation.PlacedFootprint.Footprint.RowPitch = length / height;
	srcLocation.PlacedFootprint.Footprint.Width = width;
	srcLocation.PlacedFootprint.Offset = 0;
	srcLocation.pResource = UploadBuffer;
	//srcLocation.SubresourceIndex = 0;
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	D3D12_TEXTURE_COPY_LOCATION dstLocation = { };
	dstLocation.pResource = destination;
	dstLocation.SubresourceIndex = 0;
	/*dstLocation.PlacedFootprint.Footprint.Depth = 1;
	dstLocation.PlacedFootprint.Footprint.Format = format;
	dstLocation.PlacedFootprint.Footprint.Height = height;
	dstLocation.PlacedFootprint.Footprint.RowPitch = length / height;
	dstLocation.PlacedFootprint.Footprint.Width = width;
	dstLocation.PlacedFootprint.Offset = 0;*/
	//dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);




	CommandList->Close();
	ID3D12CommandList* commandLists[] = { CommandList };
	CommandQueue->ExecuteCommandLists(1, commandLists);

	hr = Renderer::CommandQueue->Signal(UploadFence, UploadFenceValue);

	if (UploadFence->GetCompletedValue() < UploadFenceValue)
	{
		UploadFence->SetEventOnCompletion(UploadFenceValue, UploadFenceEvent);
		WaitForSingleObject(UploadFenceEvent, INFINITE);
	}
	UploadFenceValue++;


	return hr;
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

	CommandList->SetGraphicsRootSignature(mesh->GetMaterial()->GetShader()->GetSignature());
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

	CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
}

size_t Renderer::AllocateTextureIndex()
{
	if (NextTextureIndex == MaxTextureCount)
	{
		DebugBreak();
		return 0;
	}
	return NextTextureIndex++;
}