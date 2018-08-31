#pragma once

#include <d3d12.h>
#include "GPURingAllocator.h"
#include <vector>

template<typename T>
class ConstantBuffer
{
public:
	void Apply(ID3D12GraphicsCommandList* const commandList, const T& value, const unsigned int& rootParameterIndex, const unsigned long long& frameID);
	void ReleaseFrame(const unsigned long long& frameID);
	void Dispose();

	static ConstantBuffer<T>* Create();

private:
	constexpr int floor(int input) { return 0; }

	std::vector<GPURingAllocator*> Allocators;
	const int InitialCount = 0x10;
	//const int InstanceSizeAlign = 256;
	const int InstanceSize = sizeof(T); // sizeof(T) snapped to 256

	ConstantBuffer();
};

template<typename T>
void ConstantBuffer<T>::Apply(ID3D12GraphicsCommandList* const commandList, const T& value, const unsigned int& rootParameterIndex, const unsigned long long& frameID)
{
	GPURingAllocator* allocator = nullptr;
	if (this->Allocators.size() == 0)
	{
		allocator = GPURingAllocator::Create(sizeof(T) * ConstantBuffer<T>::InitialCount, 256, 256);
		if (allocator == nullptr)
		{
			OutputDebugStringW(L"[ERROR]: ConstantBuffer<T>::Apply: Failed to create first GPURingAllocator!");
			return;
		}
		else
		{
			this->Allocators.push_back(allocator);
		}
	}
	else
	{
		allocator = this->Allocators.at(this->Allocators.size() - 1);
	}

	GPURingAllocation* allocation = allocator->Allocate(InstanceSize, frameID);
	if (allocation == nullptr)
	{
		allocator = GPURingAllocator::Create(allocator->GetSize() * 2);
		if (allocator != nullptr)
		{
			this->Allocators.push_back(allocator);
			allocation = allocator->Allocate(InstanceSize, frameID);
			if (allocation == nullptr)
			{
				OutputDebugStringW(L"[ERROR]: ConstantBuffer<T>::Apply: Increased GPURingAllocator still failed to allocate!");
				return;
			}
		}
		else
		{
			OutputDebugStringW(L"[ERROR]: ConstantBuffer<T>::Apply: Failed to create increased GPURingAllocator!");
			return;
		}
	}

	*((T*)allocation->CPUData) = value;
	commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, allocation->GPUAddress);
}

template<typename T>
void ConstantBuffer<T>::ReleaseFrame(const unsigned long long& frameID)
{
	for (int i = 0; i < this->Allocators.size(); i++)
	{
		this->Allocators.at(i)->ReleaseFrame(frameID);

		// Dispose all allocators that are empty except the most recent
		if (this->Allocators.at(i)->GetUsed() == 0 && i < this->Allocators.size() - 1)
		{
			this->Allocators.at(i)->Dispose();
		}
	}
}

template<typename T>
void ConstantBuffer<T>::Dispose()
{
	for (int i = 0; i < this->Allocators.size(); i++)
	{
		this->Allocators.at(i)->Dispose();
	}
}

template<typename T>
ConstantBuffer<T>* ConstantBuffer<T>::Create()
{
	return new ConstantBuffer<T>();
}

template<typename T>
ConstantBuffer<T>::ConstantBuffer()
{

}
