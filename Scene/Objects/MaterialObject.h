#pragma once

#include "../SceneObject.h"

class MaterialObject : public SceneObject 
{
public:
	inline float GetAOMaxRange() const { return this->AOMaxRange.GetValue(); }
	inline void SetAOMaxRange(const float& aoMaxRange) { this->AOMaxRange.SetValue(aoMaxRange); }
	MaterialObject(const std::wstring& name);

private:
	FloatProperty AOMaxRange;
};