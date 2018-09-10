#include "BakeObject.h"
#include "../../ImGui/imgui.h"
#include "../../AOBaker.h"
#include "../../Renderer.h"

using namespace DirectX::SimpleMath;

BakeObject::Vertex::Vertex() : Vertex(Vector3::Zero, Vector3::Zero, Vector2::Zero) { }

BakeObject::Vertex::Vertex(const Vector3& position, const Vector3& normal, const Vector2& uv) : Position(position), Normal(normal), UV(uv) { }

BakeObject::Section::Section(const std::wstring& name) : SceneObject(name), VisibleToSelf(L"Visible To Self", true), VisibleToOthers(L"Visible To Others", true), PreviewMesh(nullptr)
{
	this->AddProperty(&this->VisibleToSelf);
	this->AddProperty(&this->VisibleToOthers);
}

void BakeObject::Section::SetMeshData(Vertex* const vertices, const int& vertexCount, unsigned int* const indices, const int& indexCount)
{
	// TODO: Assign the RenderMaterial from the BakeMaterial.
	if (this->PreviewMesh != nullptr)
	{
		this->PreviewMesh->Dispose();
	}
	RenderVertex* renderVertices = new RenderVertex[vertexCount];
	for (int i = 0; i < vertexCount; i++)
	{
		renderVertices[i] = RenderVertex(vertices[i].Position, vertices[i].Normal, vertices[i].UV, Vector4::One);
	}
	this->PreviewMesh = new RenderMesh(GetTestMaterial(), PrimitiveType::Triangle, renderVertices, vertexCount, indices, indexCount);
	this->PreviewMesh->UploadBuffers();
	delete[] renderVertices;
}

void BakeObject::Section::Render(const DirectX::SimpleMath::Matrix& transform) const
{
	Renderer::Render(this->PreviewMesh, transform);
	//Renderer::Render(GetTestMesh(), transform);
}

BakeObject::Section::~Section()
{
	if (this->PreviewMesh != nullptr)
	{
		this->PreviewMesh->Dispose();
		this->PreviewMesh = nullptr;
	}
}

BakeObject::BakeObject(const std::wstring& name) : TransformObject(name) { }

void BakeObject::OnGUI() 
{
	for (size_t i = 0; i < this->Sections.size(); i++)
	{
		if (ImGui::TreeNode((const void*)this->Sections[i], nullptr, "Section '%s'", this->Sections[i]->GetNameGUI()))
		{
			this->Sections[i]->OnGUI();
			ImGui::TreePop();
		}
	}
	TransformObject::OnGUI();
}

void BakeObject::Render() const
{
	DirectX::SimpleMath::Matrix transform = this->BuildTransform();
	for (size_t i = 0; i < this->Sections.size(); i++)
	{
		this->Sections[i]->Render(transform);
	}
}

void BakeObject::LoadFromOBJ(const std::wstring& filename)
{
	// TODO: Implement OBJ loading
}

BakeObject::Vertex CubeVertices[] = 
{
	{ Vector3(0.5f, 0.5f, 0.5f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
	{ Vector3(-0.5f, 0.5f, 0.5f), Vector3::Zero, Vector2::Zero },
	{ Vector3(0.5f, -0.5f, 0.5f), Vector3::Zero, Vector2::Zero },
	{ Vector3(-0.5f, -0.5f, 0.5f), Vector3::Zero, Vector2::Zero },
	{ Vector3(0.5f, 0.5f, -0.5f), Vector3::Zero, Vector2::Zero },
	{ Vector3(-0.5f, 0.5f, -0.5f), Vector3::Zero, Vector2::Zero },
	{ Vector3(0.5f, -0.5f, -0.5f), Vector3::Zero, Vector2::Zero },
	{ Vector3(-0.5f, -0.5f, -0.5f), Vector3::Zero, Vector2::Zero }
};

unsigned int CubeIndices[] = 
{
	0, 2, 1,
	1, 2, 3,
	4, 5, 6,
	5, 7, 6,

	0, 4, 2,
	2, 4, 6,
	1, 3, 5,
	3, 7, 5,

	0, 1, 4,
	1, 5, 4,
	2, 6, 3,
	3, 6, 7,
};

void BakeObject::LoadFromCube()
{
	this->Clear();
	Section* s = new Section(L"Geometry");
	s->SetMeshData(CubeVertices, 8, CubeIndices, 36);
	this->Sections.push_back(s);
}

void BakeObject::Clear()
{
	for (size_t i = 0; i < this->Sections.size(); i++)
	{
		delete this->Sections[i];
	}
	this->Sections.clear();
}