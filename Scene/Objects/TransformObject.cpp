#include "TransformObject.h"
#include "../../ImGui/ImGuizmo.h"
#include "../../Renderer.h"

using namespace DirectX::SimpleMath;

TransformObject::TransformObject(const std::wstring& name) : TransformObject(name, Vector3::Zero, Vector3::Zero, Vector3::One) { }

TransformObject::TransformObject(const std::wstring& name, const Vector3& position, const DirectX::SimpleMath::Vector3& rotation, const DirectX::SimpleMath::Vector3& scale) : SceneObject(name), Position(L"Position", position), Rotation(L"Rotation", rotation), Scale(L"Scale", scale)
{
	AddProperty(&this->Position);
	AddProperty(&this->Rotation);
	AddProperty(&this->Scale);
}

Quaternion TransformObject::BuildQuaternion() const
{
	return Quaternion::CreateFromYawPitchRoll(this->GetRotation().y, this->GetRotation().x, this->GetRotation().z);
}

Matrix TransformObject::BuildTransform() const
{
	return Matrix::CreateScale(this->GetScale()) * Matrix::CreateFromQuaternion(this->BuildQuaternion()) * Matrix::CreateTranslation(this->GetPosition());
}

void TransformObject::OnGUI()
{
	SceneObject::OnGUI();

	Matrix view = Renderer::GetView();
	Matrix projection = Renderer::GetProjection();
	Matrix matrix = this->BuildTransform();
	ImGuizmo::Manipulate((float*)view.m, (float*)projection.m, ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, (float*)matrix.m);
	Vector3 translation;
	Vector3 scale;
	Vector3 rotation;
	ImGuizmo::DecomposeMatrixToComponents((float*)matrix.m, (float*)&translation, (float*)&rotation, (float*)&scale);
	this->SetPosition(translation);
	this->SetRotation(rotation * PI / 180.0f);
	this->SetScale(scale);
}