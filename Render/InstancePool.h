#pragma once

#include <vector>

// A pool of reusable storage for instances of type T.
// Usage: Call InstancePool<T>::Allocate() to use an instance, and call InstancePool<T>::Release(T*) to free an instance.
template<typename T>
class InstancePool
{
	struct Item
	{
	public:
		T Data = { };
		Item* Next = nullptr;
		bool InUse = false;

		Item() : Data() { }
		Item(T data) { Data = data; }
	};

public:
	InstancePool(const int& count = 0x10, const int& increment = 0x10);
	T* Allocate();
	void Release(T* const item);
	~InstancePool();

private:
	Item* FirstItem = nullptr;
	Item* LastItem = nullptr;
	Item* NextFreeItem = nullptr;
	int SizeIncrement = 1;
	std::vector<void*> DataToDelete;

	void Extend(const int& count);
};

template<typename T>
InstancePool<T>::InstancePool(const int& count, const int& increment) 
{
	this->SizeIncrement = increment;
	this->Extend(count);
}

template<typename T>
T* InstancePool<T>::Allocate()
{
	// Check if we need to extend
	if (this->NextFreeItem == nullptr)
	{
		this->Extend(SizeIncrement); // Will also set this->NextFreeItem
	}

	// Claim the item
	Item* result = this->NextFreeItem;
	result->InUse = true;

	// Update this->NextFreeItem
	// Keep looping until this->NextFreeItem != nullptr && !this->NextFreeItem->InUse
	this->NextFreeItem = result->Next;
	while (this->NextFreeItem == nullptr || this->NextFreeItem->InUse)
	{
		if (this->NextFreeItem == nullptr)
		{
			this->NextFreeItem = this->FirstItem;
		}
		else
		{
			this->NextFreeItem = this->NextFreeItem->Next;
		}
	}

	//result->Data = { }; // Clear out the data
	ZeroMemory(&result->Data, sizeof(T));
	return &result->Data;
}

template<typename T>
void InstancePool<T>::Release(T* const item)
{
	_ASSERT(offsetof(struct InstancePool<T>::Item, Data) == 0); // Ensure that the address of the data is the same as the address of the containing Item

	Item* itemContainer = (Item*)(item);
	itemContainer->InUse = false;
	if (this->NextFreeItem == nullptr)
	{
		this->NextFreeItem = itemContainer;
	}
}

template<typename T>
InstancePool<T>::~InstancePool()
{
	for (int i = 0; i < this->DataToDelete.size(); i++)
	{
		delete[] this->DataToDelete.at(i);
	}
}

template<typename T>
void InstancePool<T>::Extend(const int& count)
{
	_ASSERT(count > 0);

	Item* newData = (Item*) ::operator new(sizeof(Item) * count);
	//Item* newData = new Item[count]();
	ZeroMemory(newData, sizeof(Item) * count);

	if (this->FirstItem == nullptr)
	{
		this->FirstItem = &newData[0];
	}
	if (this->LastItem != nullptr)
	{
		this->LastItem->Next = &newData[0];
	}
	this->LastItem = &newData[count - 1];
	
	for (int i = 0; i < count - 1; i++)
	{
		newData[i].Next = &newData[i + 1];
	}

	if (this->NextFreeItem == nullptr)
	{
		this->NextFreeItem = &newData[0];
	}

	this->DataToDelete.push_back(newData);
}