#pragma once

#include "SceneObject.h"
#include "Objects/BakeObject.h"
#include "Objects/MaterialObject.h"

#undef GetObject

// Scenes hold all the information for a usage session, including the models, materials, and settings.
class Scene : public SceneObject
{
public:
	inline DirectX::SimpleMath::Vector3 GetCameraPosition() const { return this->CameraPosition.GetValue(); }
	inline void SetCameraPosition(const DirectX::SimpleMath::Vector3& position) { this->CameraPosition.SetValue(position); }
	inline float GetCameraPitch() const { return this->CameraPitch.GetValue(); }
	inline void SetCameraPitch(const float& pitch) { this->CameraPitch.SetValue(pitch); }
	inline float GetCameraYaw() const { return this->CameraYaw.GetValue(); }
	inline void SetCameraYaw(const float& yaw) { this->CameraYaw.SetValue(yaw); }

	Scene();
	void AddObject(BakeObject* const object);
	void RemoveObject(const int& index);
	int FindObject(const std::wstring& name) const;
	int FindObject(BakeObject* const object) const;
	BakeObject* GetObject(const int& index) const;
	int GetObjectCount() const;
	void AddMaterial(MaterialObject* const material);
	void RemoveMaterial(const int& index);
	int FindMaterial(const std::wstring& name) const;
	int FindMaterial(MaterialObject* const material) const;
	MaterialObject* GetMaterial(const int& index) const;
	int GetMaterialCount() const;
	void Render();
	void DrawObjectList();
	void DrawMaterialList();
	void DrawPropertiesGUI();
	~Scene();

private:
	Vector3Property CameraPosition;
	FloatProperty CameraPitch;
	FloatProperty CameraYaw;
	// All the BakeObjects in the scene. All pointers will be deleted upon Scene destruction.
	std::vector<BakeObject*> BakeObjects;
	std::vector<MaterialObject*> Materials;

	static bool EnumerateBakeObjects(void* data, int index, const char** outText);
	static bool EnumerateMaterials(void* data, int index, const char** outText);
};