#include "RenderTexture.h"
#include "../Renderer.h"

RenderTexture::RenderTexture(const int& width, const int& height, const DXGI_FORMAT& format)
{
	if (width <= 0 || height <= 0)
		throw L"Invalid size given to RenderTexture::RenderTexture.";

	this->Index = Renderer::AllocateTextureIndex();
	this->Width = width;
	this->Height = height;
	this->Format = format;

	D3D12_RESOURCE_DESC resdesc = { };
	resdesc.Alignment = 0;
	resdesc.DepthOrArraySize = 1;
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Format = this->Format;
	resdesc.Width = this->Width;
	resdesc.Height = this->Height;
	resdesc.MipLevels = 1;
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	D3D12_HEAP_PROPERTIES heapdesc = { };
	heapdesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapdesc.CreationNodeMask = 0;
	heapdesc.VisibleNodeMask = 0;
	heapdesc.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapdesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	D3D12_CLEAR_VALUE cleardesc = { };
	cleardesc.Format = this->Format;
	
	HRESULT hr;
	hr = Renderer::Device->CreateCommittedResource(&heapdesc, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->GPUResource));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create resource for texture!");
		DebugBreak();
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = { };
	srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Format = this->Format;
	srvdesc.Texture2D.MipLevels = 1;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.PlaneSlice = 0;
	srvdesc.Texture2D.ResourceMinLODClamp = 0;
	srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = Renderer::SRVHeap->GetCPUDescriptorHandleForHeapStart().ptr + this->Index * Renderer::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	Renderer::Device->CreateShaderResourceView(this->GPUResource, &srvdesc, handle);
}

RenderTexture::~RenderTexture()
{
	this->GPUResource->Release();
	this->GPUResource = nullptr;
}

size_t RenderTexture::GetIndex() const
{
	return this->Index;
}

void RenderTexture::UploadData(char* const data, const size_t& length) const
{
	Renderer::UploadTexture(this->GPUResource, (void*)data, length, this->Width, this->Height, this->Format);
}

D3D12_GPU_VIRTUAL_ADDRESS RenderTexture::GetGPUVirtualAddress() const
{
	return this->GPUResource->GetGPUVirtualAddress();
}

void RenderTexture::UploadCheckerboard(char r, char g, char b) const
{
	char data[64 * 64 * 4];
	for (int y = 0; y < 64; y++)
	{
		for (int x = 0; x < 64; x++)
		{
			data[y * 64 * 4 + x * 4 + 0] = (x / 16 + y / 16) % 2 == 0 ? b : 0; // B
			data[y * 64 * 4 + x * 4 + 1] = (x / 16 + y / 16) % 2 == 0 ? g : 0; // G
			data[y * 64 * 4 + x * 4 + 2] = (x / 16 + y / 16) % 2 == 0 ? r : 0; // R
			data[y * 64 * 4 + x * 4 + 3] = 255; // A
		}
	}
	this->UploadData(data, (size_t)(64 * 64 * 4));
}