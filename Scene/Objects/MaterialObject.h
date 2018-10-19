#pragma once

#include "../SceneObject.h"
#include "../BakeTexture.h"
#include "../../Render/RenderMaterial.h"

class MaterialObject : public SceneObject 
{
public:
	inline float GetAOMaxRange() const { return this->AOMaxRange.GetValue(); }
	inline void SetAOMaxRange(const float& aoMaxRange) { this->AOMaxRange.SetValue(aoMaxRange); }
	inline RenderMaterial* GetPreviewMaterial() const { return this->PreviewMaterial; }
	MaterialObject(const std::wstring& name);
	~MaterialObject();

private:
	FloatProperty AOMaxRange;
	BakeTexture* AOTexture = nullptr;
	RenderMaterial* PreviewMaterial = nullptr;
};