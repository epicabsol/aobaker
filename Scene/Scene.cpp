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

void Scene::DrawTreeGUI()
{
	// TODO: Draw scene tree
	size_t selectedObject = -1;
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		bool pressed;
		if (ImGui::TreeNodeEx(this->BakeObjects[i]->GetNameGUI(), &pressed, this->BakeObjects[i]->IsSelected() ? ImGuiTreeNodeFlags_Selected : 0))
		{
			if (pressed)
				selectedObject = i;
			ImGui::TreePop();
		}
	}

	if (selectedObject != -1)
	{
		for (size_t i = 0; i < this->BakeObjects.size(); i++)
		{
			if (i == selectedObject)
				this->BakeObjects[i]->Select();
			else
				this->BakeObjects[i]->Deselect();
		}
	}
}

void Scene::DrawPropertiesGUI()
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
	{
		if (this->BakeObjects[i]->IsSelected())
		{
			if (ImGui::TreeNode(this->BakeObjects[i], nullptr, "%s", this->BakeObjects[i]->GetNameGUI()))
			{
				this->BakeObjects[i]->OnGUI();
				ImGui::TreePop();
			}
		}
	}
}

Scene::~Scene()
{
	for (size_t i = 0; i < this->BakeObjects.size(); i++)
		delete this->BakeObjects[i];
}