#pragma once

#include "SceneObject.h"
#include "Objects/BakeObject.h"

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
	void RemoveObject(BakeObject* const object);
	void Render();
	void DrawObjectList();
	void DrawPropertiesGUI();
	~Scene();

private:
	Vector3Property CameraPosition;
	FloatProperty CameraPitch;
	FloatProperty CameraYaw;
	// All the BakeObjects in the scene. All pointers will be deleted upon Scene destruction.
	std::vector<BakeObject*> BakeObjects;

	static bool EnumerateBakeObjects(void* data, int index, const char** outText);
};