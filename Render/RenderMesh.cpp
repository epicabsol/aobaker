#include "RenderMesh.h"
#include "../Renderer.h"

//#include "Windows.h"

using namespace DirectX::SimpleMath;

RenderVertex::RenderVertex()
{
	this->Position = Vector3::Zero;
	this->Normal = Vector3::Zero;
	this->UV = Vector2::Zero;
	this->Color = Vector4::One;
}

RenderVertex::RenderVertex(const Vector3& position, const Vector3& normal, const Vector2& uv, const Vector4& color)
{
	this->Position = position;
	this->Normal = normal;
	this->UV = uv;
	this->Color = color;
}

RenderMesh::RenderMesh(RenderMaterial* const material, const PrimitiveType& type, RenderVertex* const vertices, const int& vertexCount, unsigned int* const indices, const int& indexCount)
{
	this->Material = material;
	this->Type = type;
	this->VertexCount = vertexCount;
	if (this->VertexCount > 0)
	{
		this->Vertices = new RenderVertex[this->VertexCount];
		for (int i = 0; i < this->VertexCount; i++)
			this->Vertices[i] = vertices[i];

		OutputDebugStringW(L"Creating RenderMesh with vertex count = ");
		OutputDebugStringW(std::to_wstring(this->VertexCount).c_str());
		OutputDebugStringW(L", start is = ");
		OutputDebugStringW(std::to_wstring((size_t)this->Vertices).c_str());
		OutputDebugStringW(L", end is = ");
		OutputDebugStringW(std::to_wstring((size_t)(this->Vertices) + this->VertexCount * sizeof(RenderVertex)).c_str());
		OutputDebugStringW(L"\n");
	}
	this->IndexCount = indexCount;
	if (this->IndexCount > 0)
	{
		this->Indices = new unsigned int[this->IndexCount];
		for (int i = 0; i < this->IndexCount; i++)
			this->Indices[i] = indices[i];
	}
}

RenderMaterial* RenderMesh::GetMaterial() const
{
	return this->Material;
}

void RenderMesh::SetMaterial(RenderMaterial* const material)
{
	this->Material = material;
}

int RenderMesh::GetVertexCount() const
{
	return this->VertexCount;
}

RenderVertex* RenderMesh::GetVertices() const
{
	return this->Vertices;
}

int RenderMesh::GetIndexCount() const
{
	return this->IndexCount;
}

unsigned int* RenderMesh::GetIndices() const
{
	return this->Indices;
}

PrimitiveType RenderMesh::GetPrimitiveType() const
{
	return this->Type;
}

D3D12_VERTEX_BUFFER_VIEW* RenderMesh::GetVertexBufferView()
{
	return &this->VertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW* RenderMesh::GetIndexBufferView()
{
	return &this->IndexBufferView;
}

HRESULT RenderMesh::UploadBuffers()
{
	D3D12_HEAP_PROPERTIES heapProps = { };
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC resdesc = { };
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Alignment = 0;
	resdesc.Width = this->GetVertexCount() * sizeof(RenderVertex);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	HRESULT hr = Renderer::Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->VertexBuffer));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create vertex buffer resource\n");
		return hr;
	}

	resdesc.Width = this->GetIndexCount() * sizeof(int);
	hr = Renderer::Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->IndexBuffer));
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Failed to create index buffer resource\n");
	}

	Renderer::UploadData(this->VertexBuffer, this->GetVertices(), this->GetVertexCount() * sizeof(RenderVertex));
	Renderer::UploadData(this->IndexBuffer, this->GetIndices(), this->GetIndexCount() * sizeof(int));
	this->VertexBufferView.SizeInBytes = sizeof(RenderVertex) * this->GetVertexCount();
	this->VertexBufferView.StrideInBytes = sizeof(RenderVertex);
	this->VertexBufferView.BufferLocation = this->VertexBuffer->GetGPUVirtualAddress();
	this->IndexBufferView.SizeInBytes = sizeof(int) * this->GetIndexCount();
	this->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	this->IndexBufferView.BufferLocation = this->IndexBuffer->GetGPUVirtualAddress();
	return S_OK;
}

void RenderMesh::Dispose()
{
	if (this->VertexBuffer != nullptr)
	{
		this->VertexBuffer->Release();
		this->VertexBuffer = nullptr;
	}
	if (this->IndexBuffer != nullptr)
	{
		this->IndexBuffer->Release();
		this->IndexBuffer = nullptr;
	}
	if (this->IndexCount > 0)
	{
		delete[] this->Indices;
		this->Indices = nullptr;
	}
	if (this->VertexCount > 0)
	{
		delete[] this->Vertices;
		this->Vertices = nullptr;
	}
}