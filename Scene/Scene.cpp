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

void Scene::RemoveObject(BakeObject* const object)
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		if (this->BakeObjects[i] == object)
		{
			this->BakeObjects.erase(this->BakeObjects.begin() + i);
			break;
		}
	}
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
	*outText = scene->BakeObjects.at(index)->GetNameGUI();
	return true;
}

void Scene::DrawObjectList()
{
	int selectedItem = -1;
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
		if (this->BakeObjects[i]->IsSelected())
			selectedItem = i;

	ImGui::PushItemWidth(-1);
	if (ImGui::ListBox("Bake Objects", &selectedItem, &EnumerateBakeObjects, this, this->BakeObjects.size(), 10))
	{
		for (size_t i = 0; i < this->BakeObjects.size(); i++)
		{
			if (i == selectedItem)
				this->BakeObjects[i]->Select();
			else
				this->BakeObjects[i]->Deselect();
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
			//if (ImGui::TreeNode(this->BakeObjects[i], nullptr, "%s", this->BakeObjects[i]->GetNameGUI()))
			//{
			this->BakeObjects[i]->OnGUI();
			//ImGui::TreePop();
			//}
		}
	}
}

Scene::~Scene()
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
		delete this->BakeObjects[i];
}