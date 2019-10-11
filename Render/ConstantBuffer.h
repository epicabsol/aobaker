#pragma once

#include <d3d11.h>
//#include <d3d12.h>
//#include "GPURingAllocator.h"
//#include <vector>

template<typename T>
class ConstantBuffer
{
private:
	ID3D11Buffer* Buffer;

public:
	ConstantBuffer(ID3D11Buffer* buffer) : Buffer(buffer)
	{

	}

	~ConstantBuffer()
	{
		if (this->Buffer != nullptr)
		{
			this->Buffer->Release();
			this->Buffer = nullptr;
		}
	}

	ID3D11Buffer* GetBuffer() const
	{
		return this->Buffer;
	}

	void Update(ID3D11DeviceContext* context, const T& value)
	{
		D3D11_MAPPED_SUBRESOURCE data = { };
		HRESULT hr = context->Map(this->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
		if (SUCCEEDED(hr))
		{
			*(T*)(data.pData) = value;
			context->Unmap(this->Buffer, 0);
		}
		else
		{
			throw "o no";
		}
	}

	static ConstantBuffer<T>* Create(ID3D11Device* device, const T& value, bool isDynamic)
	{
		D3D11_BUFFER_DESC desc = { };
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(T);
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.Usage = isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		D3D11_SUBRESOURCE_DATA initialData = { };
		initialData.pSysMem = &value;
		ID3D11Buffer* buffer = nullptr;
		HRESULT hr = device->CreateBuffer(&desc, &initialData, &buffer);
		ConstantBuffer<T>* result = nullptr;
		if (SUCCEEDED(hr))
		{
			result = new ConstantBuffer<T>(buffer);
		}
		return result;
	}
};
