#pragma once

#include "../SceneObject.h"
#include <d3d12.h>
#include "../../SimpleMath.h"

class TransformObject : public SceneObject
{
public:
	inline DirectX::SimpleMath::Vector3 GetPosition() const { return this->Position.GetValue(); }
	inline void SetPosition(const DirectX::SimpleMath::Vector3& position) { this->Position.SetValue(position); }
	inline DirectX::SimpleMath::Vector3 GetRotation() const { return this->Rotation.GetValue(); }
	inline void SetRotation(const DirectX::SimpleMath::Vector3& rotation) { this->Rotation.SetValue(rotation); }
	inline DirectX::SimpleMath::Vector3 GetScale() const { return this->Scale.GetValue(); }
	inline void SetScale(const DirectX::SimpleMath::Vector3& scale) { this->Scale.SetValue(scale); }

	TransformObject(const std::wstring& name);
	TransformObject(const std::wstring& name, const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& rotation, const DirectX::SimpleMath::Vector3& scale);
	DirectX::SimpleMath::Quaternion BuildQuaternion() const;
	DirectX::SimpleMath::Matrix BuildTransform() const;
	virtual void OnGUI() override;

private:
	Vector3Property Position;
	Vector3Property Rotation;
	Vector3Property Scale;
};