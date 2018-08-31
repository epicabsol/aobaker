#pragma once

#include <d3d12.h>
#include "InstancePool.h"

struct GPURingAllocation
{
	ID3D12Resource1* Resource = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = NULL;
	void* CPUData = nullptr;
	unsigned long long FrameID = 0;
	int Length = 0;

	GPURingAllocation(ID3D12Resource1* resource, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, void* cpuData, const unsigned long long& frameID, const int& length) : Resource(resource), GPUAddress(gpuAddress), CPUData(cpuData), FrameID(frameID), Length(length) { }
};

class GPURingAllocator
{
	struct Item
	{
		GPURingAllocation Data;
		Item* Previous = nullptr;
		Item* Next = nullptr;
		int StartOffset = 0;

		Item(const GPURingAllocation& data, const int& startOffset, Item* const previous = nullptr) : Data(data), StartOffset(startOffset), Previous(previous) { }
	};
public:

	// Gets the total amount of memory in use by this allocator.
	int GetUsed() const;

	// Gets the size of the buffer this allocator allocates from.
	int GetSize() const;

	// Allocates memory of the given size from the buffer, or returns nullptr.
	GPURingAllocation* Allocate(const int& size, const unsigned long long& frameID);

	// Releases all the allocations with a FrameID less than or equal to the given frameID.
	void ReleaseFrame(const unsigned long long& frameID);

	// Releases all the CPU & GPU resoutces allocated by creating this object.
	void Dispose();

	// Creates a new GPURingAllocator with the given size, which must be deleted by the caller, or returns nullptr if one could not be created.
	static GPURingAllocator* Create(const int& size, const int& itemAlignment = 0, const int& itemSizeAlignment = 0);

private:
	ID3D12Resource1* Resource = nullptr;
	int Used = 0;
	int Size = 0;
	int ItemAlignment = 0;
	int ItemSizeAlignment = 0;
	Item* Front = nullptr; // The last allocated item
	Item* Back = nullptr; // The first allocated item
	void* Data = nullptr;
	InstancePool<Item> ItemPool = InstancePool<Item>();

	GPURingAllocator(const int& size, ID3D12Resource1* const resource, void* data, const int& itemAlignment = 0, const int& itemSizeAlignment = 0) : Size(size), ItemAlignment(itemAlignment), ItemSizeAlignment(itemSizeAlignment), Resource(resource), Data(data) { }
	inline int Align(const int& input) const;
	inline int AlignSize(const int& size) const;
};