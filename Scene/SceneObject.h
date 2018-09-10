#pragma once

#include "SceneProperty.h"

#include <vector>

class SceneObject
{
public:
	inline std::wstring GetName() const { return this->Name.GetValue(); }
	inline void SetName(const std::wstring& name) { this->Name.SetValue(name); }
	inline bool IsSelected() const { return this->Selected; }
	inline void Select() { this->Selected = true; }
	inline void Deselect() { this->Selected = false; }
	virtual void OnGUI();
	inline char* GetNameGUI() const { return this->Name.GetValueGUI(); }
	SceneProperty* GetProperty(const std::wstring& name) const;

	SceneObject(const std::wstring& name);

protected:
	inline void AddProperty(SceneProperty* const property) { this->Properties.push_back(property); }

private:
	std::vector<SceneProperty*> Properties = std::vector<SceneProperty*>();
	StringProperty Name;
	bool Selected = false;
};