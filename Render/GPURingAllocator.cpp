#include "GPURingAllocator.h"
#include "../Renderer.h"

int GPURingAllocator::GetUsed() const
{
	return this->Used;
}

int GPURingAllocator::GetSize() const
{
	return this->Size;
}

GPURingAllocation* GPURingAllocator::Allocate(const int& size, const unsigned long long& frameID)
{
	const int alignedSize = this->AlignSize(size);
	// Find the starting offset
	int offset = 0;
	if (this->Front != nullptr)
	{
		offset = this->Front->StartOffset + this->Front->Data.Length;
		if (offset > this->Size - alignedSize)
		{
			offset = 0;
		}
		if (frameID < this->Front->Data.FrameID)
		{
			OutputDebugStringW(L"[WARNING]: GPURingAllocator: You're trying to go back in time with these frame IDs. Don't.");
			return nullptr;
		}
	}
	offset = this->Align(offset);

	// Find the max offset
	int maxOffset = this->Size;
	if (this->Back != nullptr)
	{
		maxOffset = this->Back->StartOffset;
	}

	// Check size
	if (maxOffset - offset < alignedSize)
	{
		OutputDebugStringW(L"[WARNING]: GPURingBuffer: Couldn't allocate enough memory!");
	}

	// Construct item (don't assume any values in the ItemPool's allocated data)
	//OutputDebugStringW(L"[INFO]: GPURingAllocator: Allocating item.\n");
	Item* newItem = ItemPool.Allocate();
	newItem->Next = nullptr;
	newItem->Previous = this->Front;
	newItem->StartOffset = offset;
	newItem->Data = GPURingAllocation(this->Resource, this->Resource->GetGPUVirtualAddress() + offset, (void*)((char*)(this->Data) + offset), frameID, alignedSize);

	// Update references
	if (this->Back == nullptr)
	{
		this->Back = newItem;
	}
	if (this->Front != nullptr)
	{
		this->Front->Next = newItem;
	}
	this->Front = newItem;
	this->Used += alignedSize;

	return &newItem->Data;
}

void GPURingAllocator::ReleaseFrame(const unsigned long long& frameID)
{
	while (this->Back != nullptr && this->Back->Data.FrameID <= frameID)
	{
		//OutputDebugStringW(L"[INFO]: GPURingAllocator: Disposing item.\n");
		this->Used -= this->Back->Data.Length;

		// Release the item info back to the pool
		this->ItemPool.Release(this->Back);

		// Remove from buffer
		this->Back = this->Back->Next;
	}
}

void GPURingAllocator::Dispose()
{
	if (this->Resource != nullptr)
	{
		this->Resource->Unmap(0, nullptr);
		this->Data = nullptr;
		this->Resource->Release();
		this->Resource = nullptr;
	}
	this->Used = 0;
}

GPURingAllocator* GPURingAllocator::Create(const int& size, const int& itemAlignment, const int& itemSizeAlignment)
{
	// Create the GPU resource
	ID3D12Resource1* resource = nullptr;
	void* data = nullptr;
	D3D12_HEAP_PROPERTIES heapProps = { };
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	D3D12_RESOURCE_DESC resDesc = { };
	resDesc.Alignment = 0;
	resDesc.Width = size;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	HRESULT hr = Renderer::Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"[ERROR]: GPURingAllocator: Failed to create Resource.");
		return nullptr;
	}
	resource->SetName(L"GPURingAllocator GPU Buffer");
	resource->Map(0, nullptr, &data);

	return new GPURingAllocator(size, resource, data, itemAlignment, itemSizeAlignment);
}

int GPURingAllocator::Align(const int& input) const
{
	return input + ((this->ItemAlignment - (input % this->ItemAlignment)) % this->ItemAlignment);
}

int GPURingAllocator::AlignSize(const int& size) const
{
	return size + ((this->ItemSizeAlignment - (size % this->ItemSizeAlignment)) % this->ItemSizeAlignment);
}