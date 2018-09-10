#include "SceneObject.h"

void SceneObject::OnGUI()
{
	for (size_t i = 0; i < this->Properties.size(); i++)
	{
		this->Properties[i]->OnGUI();
	}
}

SceneProperty* SceneObject::GetProperty(const std::wstring& name) const
{
	for (size_t i = 0; i < this->Properties.size(); i++)
		if (this->Properties[i]->GetName() == name)
			return this->Properties[i];
	return nullptr;
}
SceneObject::SceneObject(const std::wstring& name) : Name(L"Name", name)
{
	this->AddProperty(&this->Name);
}
