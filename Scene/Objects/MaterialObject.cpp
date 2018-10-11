#include "MaterialObject.h"

MaterialObject::MaterialObject(const std::wstring& name) : SceneObject(name), AOMaxRange(L"AO Max Range", 1.0f)
{
	this->AddProperty(&this->AOMaxRange);
}