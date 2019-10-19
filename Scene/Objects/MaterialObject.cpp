#include "MaterialObject.h"
#include "../../AOBaker.h"

MaterialObject::MaterialObject(const std::wstring& name) : SceneObject(name), AOMaxRange(L"AO Max Range", 1.0f)
{
	this->AddProperty(&this->AOMaxRange);
	this->AOTexture = new BakeTexture(512, 512, 1, 1);
	RenderTexture* PreviewTextures[] = { this->AOTexture->GetGPUPreview() };
	this->PreviewMaterial = new RenderMaterial(GetBakeObjectShader(), PreviewTextures, _ARRAYSIZE(PreviewTextures));
}

MaterialObject::~MaterialObject()
{
	delete this->PreviewMaterial;
	this->PreviewMaterial = nullptr;
	delete this->AOTexture;
	this->AOTexture = nullptr;
}