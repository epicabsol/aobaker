#include "Scene.h"
#include "../Renderer.h"
#include "../ImGui/imgui.h"

using namespace DirectX::SimpleMath;

Scene::Scene() : SceneObject(L"Scene"), CameraPosition(L"Camera Position", Vector3(0.0f, 10.0f, 10.0f)), CameraPitch(L"Camera Pitch", 0.5f), CameraYaw(L"Camera Yaw")
{
	this->AddProperty(&this->CameraPosition);
	this->AddProperty(&this->CameraPitch);
	this->AddProperty(&this->CameraYaw);
}

void Scene::AddObject(BakeObject* const object)
{
	this->BakeObjects.push_back(object);
}

void Scene::RemoveObject(const int& index)
{
	this->BakeObjects.erase(this->BakeObjects.begin() + index);
}

int Scene::FindObject(const std::wstring& name) const
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		if (this->BakeObjects[i]->GetName() == name)
		{
			return i;
		}
	}
	return -1;
}

int Scene::FindObject(BakeObject* const object) const
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		if (this->BakeObjects[i] == object)
		{
			return i;
		}
	}
	return -1;
}

BakeObject* Scene::GetObject(const int& index) const
{
	return this->BakeObjects[index];
}

int Scene::GetObjectCount() const
{
	return this->BakeObjects.size();
}

void Scene::AddMaterial(MaterialObject* const material) 
{
	this->Materials.push_back(material);
}

void Scene::RemoveMaterial(const int& index)
{
	this->Materials.erase(this->Materials.begin() + index);
}

int Scene::FindMaterial(const std::wstring& name) const
{
	for (size_t i = 0; i < this->Materials.size(); i++)
	{
		if (this->Materials[i]->GetName() == name)
		{
			return i;
		}
	}
	return -1;
}

int Scene::FindMaterial(MaterialObject* const material) const
{
	for (size_t i = 0; i < this->Materials.size(); i++)
	{
		if (this->Materials[i] == material)
		{
			return i;
		}
	}
	return -1;
}

MaterialObject* Scene::GetMaterial(const int& index) const
{
	return this->Materials[index];
}

int Scene::GetMaterialCount() const
{
	return this->Materials.size();
}

void Scene::Render()
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		this->BakeObjects[i]->Render();
	}
}

bool Scene::EnumerateBakeObjects(void* data, int index, const char** outText)
{
	Scene* scene = (Scene*)data;
	*outText = scene->BakeObjects[index]->GetNameGUI();
	return true;
}

bool Scene::EnumerateMaterials(void* data, int index, const char** outText)
{
	Scene* scene = (Scene*)data;
	*outText = scene->Materials[index]->GetNameGUI();
	return true;
}

void Scene::DrawObjectList()
{
	int selectedItem = -1;
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
		if (this->BakeObjects[i]->IsSelected())
			selectedItem = i;

	ImGui::PushItemWidth(-1.0f);
	if (ImGui::ListBox("", &selectedItem, &EnumerateBakeObjects, this, this->BakeObjects.size(), 10))
	{
		for (size_t i = 0; i < this->BakeObjects.size(); i++)
		{
			if (i == selectedItem) 
			{
				this->BakeObjects[i]->Select();
				for (size_t m = 0; m < this->Materials.size(); m++)
					this->Materials[m]->Deselect();
			}
			else
				this->BakeObjects[i]->Deselect();
		}
	}
	ImGui::PopItemWidth();
}

void Scene::DrawMaterialList()
{
	int selectedItem = -1;
	for (size_t i = 0; i < this->Materials.size(); i++)
		if (this->Materials[i]->IsSelected())
			selectedItem = i;

	ImGui::PushItemWidth(-1.0f);
	if (ImGui::ListBox("", &selectedItem, &EnumerateMaterials, this, this->Materials.size(), 10))
	{
		for (size_t i = 0; i < this->Materials.size(); i++)
		{
			if (i == selectedItem) 
			{
				this->Materials[i]->Select();
				for (size_t b = 0; b < this->BakeObjects.size(); b++)
					this->BakeObjects[b]->Deselect();
			}
			else
				this->Materials[i]->Deselect();
		}
	}
	ImGui::PopItemWidth();
}

void Scene::DrawPropertiesGUI()
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		if (this->BakeObjects[i]->IsSelected())
		{
			this->BakeObjects[i]->OnGUI();
		}
	}
	for (size_t i = 0; i < this->Materials.size(); i++)
	{
		if (this->Materials[i]->IsSelected())
		{
			this->Materials[i]->OnGUI();
		}
	}
}

Scene::~Scene()
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
		delete this->BakeObjects[i];
	for (size_t i = 0; i < this->Materials.size(); i++)
		delete this->Materials[i];
}