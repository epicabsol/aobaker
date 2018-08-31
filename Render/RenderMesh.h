#pragma once

#include <d3d12.h>
#include "../SimpleMath.h"
#include "RenderMaterial.h"

enum class PrimitiveType
{
	Triangle = 0,
	Line = 1,
};

struct RenderVertex
{
public:
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 UV;
	DirectX::SimpleMath::Vector4 Color;

	RenderVertex();
	RenderVertex(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, const DirectX::SimpleMath::Vector2& uv, const DirectX::SimpleMath::Vector4& color);
};

class RenderMesh
{
public:
	RenderMesh(RenderMaterial* const material, const PrimitiveType& type, RenderVertex* const vertices, const int& vertexCount, int* const indices, const int& indexCount);
	RenderMaterial* GetMaterial() const;
	int GetVertexCount() const;
	RenderVertex* GetVertices() const;
	int GetIndexCount() const;
	int* GetIndices() const;
	PrimitiveType GetPrimitiveType() const;
	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView();
	HRESULT UploadBuffers();
	void Dispose();

private:
	RenderMaterial* Material = nullptr;
	int VertexCount = 0;
	RenderVertex* Vertices = { };
	int IndexCount = 0;
	int* Indices = { };
	PrimitiveType Type = PrimitiveType::Triangle;

	ID3D12Resource1* VertexBuffer = nullptr;
	ID3D12Resource1* IndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = { };
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = { };
};