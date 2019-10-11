#pragma once

#include <d3d11.h>
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
	RenderMesh(ID3D11Device* device, RenderMaterial* const material, const PrimitiveType& type, RenderVertex* const vertices, const int& vertexCount, unsigned int* const indices, const int& indexCount);
	~RenderMesh();
	RenderMaterial* GetMaterial() const;
	void SetMaterial(RenderMaterial* const material);
	int GetVertexCount() const;
	RenderVertex* GetVertices() const;
	int GetIndexCount() const;
	unsigned int* GetIndices() const;
	PrimitiveType GetPrimitiveType() const;
	ID3D11Buffer* GetVertexBuffer();
	ID3D11Buffer* GetIndexBuffer();
	//HRESULT UploadBuffers();
	//void Dispose();

private:
	RenderMaterial* Material = nullptr;
	int VertexCount = 0;
	RenderVertex* Vertices = { };
	int IndexCount = 0;
	unsigned int* Indices = { };
	PrimitiveType Type = PrimitiveType::Triangle;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
};