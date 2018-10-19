#include "BakeObject.h"
#include "../../ImGui/imgui.h"
#include "../../AOBaker.h"
#include "../../Renderer.h"
// assimp model importer
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
// wstring to string
#include <locale>
#include <codecvt>
static std::wstring_convert<std::codecvt_utf8<wchar_t>> StringConverter;

using namespace DirectX::SimpleMath;

BakeObject::Vertex::Vertex() : Vertex(Vector3::Zero, Vector3::Zero, Vector2::Zero) { }

BakeObject::Vertex::Vertex(const Vector3& position, const Vector3& normal, const Vector2& uv) : Position(position), Normal(normal), UV(uv) { }

BakeObject::Section::Section(const std::wstring& name) : SceneObject(name), VisibleToSelf(L"Visible To Self", true), VisibleToOthers(L"Visible To Others", true), Material(L"Material", nullptr), PreviewMesh(nullptr)
{
	this->AddProperty(&this->VisibleToSelf);
	this->AddProperty(&this->VisibleToOthers);
	this->AddProperty(&this->Material);
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
	// HACK: Constantly set the preview material
	this->PreviewMesh->SetMaterial(this->GetMaterial()->GetPreviewMaterial());
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

void BakeObject::ImportAiNode(const aiScene* scene, aiNode* node, const aiMatrix4x4& parentTransform, Scene* const targetScene)
{
	aiMatrix4x4 transform = parentTransform * node->mTransformation;
	bool isTransformed = transform.IsIdentity();

	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		BakeObject::Vertex* vertices = new BakeObject::Vertex[mesh->mNumVertices];
		std::vector<unsigned int> indices(mesh->mNumFaces * 3); // Will probably have around 3 indices per face
		for (int v = 0; v < mesh->mNumVertices; v++)
		{
			aiVector3D pos = mesh->mVertices[v];
			if (isTransformed)
				pos = transform * pos;
			vertices[v].Position = Vector3(pos.x, pos.y, pos.z);
		}

		if (mesh->HasNormals())
		{
			for (int v = 0; v < mesh->mNumVertices; v++)
			{
				aiVector3D norm = mesh->mNormals[v];
				vertices[v].Normal = Vector3(norm.x, norm.y, norm.z);
			}
		}

		if (mesh->HasTextureCoords(0))
		{
			for (int v = 0; v < mesh->mNumVertices; v++)
			{
				aiVector3D uvw = mesh->mTextureCoords[0][v];
				vertices[v].UV = Vector2(uvw.x, uvw.y);
			}
		}

		for (int f = 0; f < mesh->mNumFaces; f++)
		{
			for (int i = 0; i < mesh->mFaces[f].mNumIndices; i++)
			{
				indices.push_back(mesh->mFaces[f].mIndices[i]);
			}
		}

		MaterialObject* material = nullptr;
		aiString aiMatName;
		scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_NAME, aiMatName);
		//std::wstring materialName = StringConverter.from_bytes(scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
		std::wstring materialName = StringConverter.from_bytes(aiMatName.C_Str());
		OutputDebugStringW(L"Material: '");
		OutputDebugStringW(materialName.c_str());
		OutputDebugStringW(L"'\n");
		int existingIndex = targetScene->FindMaterial(materialName);
		if (existingIndex > -1)
		{
			material = targetScene->GetMaterial(existingIndex);
		}
		else
		{
			material = new MaterialObject(materialName);
			targetScene->AddMaterial(material);
		}

		BakeObject::Section* newSection = new BakeObject::Section(std::wstring(StringConverter.from_bytes(node->mName.data)));
		newSection->SetMeshData(vertices, mesh->mNumVertices, indices.data(), indices.size());
		newSection->SetMaterial(material);
		this->Sections.push_back(newSection);

		delete[] vertices;
		
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		ImportAiNode(scene, node->mChildren[i], transform, targetScene);
	}
}

void BakeObject::LoadFromFile(const std::wstring& filename, Scene* const targetScene)
{
	/*std::ifstream stream = std::ifstream(filename.c_str());

	std::string line;
	std::vector<std::string> parts;
	while (std::getline(stream, line))
	{
		
		OutputDebugStringA(line.c_str());
	}

	stream.close();*/

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile((std::string)StringConverter.to_bytes(filename), aiProcess_ConvertToLeftHanded | aiProcess_FixInfacingNormals | aiProcess_GenNormals | aiProcess_ImproveCacheLocality);
	
	if (scene == nullptr)
	{
		// Error
		return;
	}
	
	// Add all nodes in the scene
	this->ImportAiNode(scene, scene->mRootNode, aiMatrix4x4(), targetScene);
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

BakeObject::~BakeObject() 
{
	for (size_t i = 0; i < this->Sections.size(); i++)
	{
		delete this->Sections[i];
	}
	this->Sections.clear();
}