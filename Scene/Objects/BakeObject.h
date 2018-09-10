#pragma once

#include "TransformObject.h"
#include "../../Render/RenderMesh.h"

class BakeObject : public TransformObject
{
public:
	struct Vertex
	{
	public:
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Vector3 Normal;
		DirectX::SimpleMath::Vector2 UV;

		Vertex();
		Vertex(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, const DirectX::SimpleMath::Vector2& uv);
	};

	class Section : public SceneObject
	{
	public:
		inline bool IsVisibleToSelf() const { return this->VisibleToSelf.GetValue(); }
		inline void SetVisibleToSelf(const bool& value) { this->VisibleToSelf.SetValue(value); }
		inline bool IsVisibleToOthers() const { return this->VisibleToOthers.GetValue(); }
		inline void SetVisibleToOthers(const bool& value) { this->VisibleToOthers.SetValue(value); }

		Section(const std::wstring& name);
		void SetMeshData(Vertex* const vertices, const int& vertexCount, unsigned int* const indices, const int& indexCount);
		void Render(const DirectX::SimpleMath::Matrix& transform) const;
		~Section();
		
	private:
		int VertexCount;
		Vertex* Vertices;
		int IndexCount;
		unsigned int* Indices;
		RenderMesh* PreviewMesh;
		
		BooleanProperty VisibleToSelf;
		BooleanProperty VisibleToOthers;
	};

	BakeObject(const std::wstring& name);
	void OnGUI() override;
	void Render() const;
	void LoadFromOBJ(const std::wstring& filename);
	void LoadFromCube();
	void Clear();

private:
	std::vector<Section*> Sections;
};

