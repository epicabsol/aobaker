#include "SceneProperty.h"
#include "../ImGui/imgui.h"

using namespace DirectX::SimpleMath;

// wstring to string
#include <locale>
#include <codecvt>
std::wstring_convert<std::codecvt_utf8<wchar_t>> StringConverter;

std::wstring SceneProperty::GetName() const
{
	return std::wstring(StringConverter.from_bytes(this->NameData));
}

SceneProperty::SceneProperty(const std::wstring& name) 
{ 
	std::string value = StringConverter.to_bytes(name);
	size_t i = 0;
	for (i = 0; i < value.size(); i++)
	{
		this->NameData[i] = value.c_str()[i];
	}
	for (i; i < NameDataLength; i++)
	{
		this->NameData[i] = 0;
	}
}

std::wstring StringProperty::GetValue() const
{
	return std::wstring(StringConverter.from_bytes(this->ValueData));
}

void StringProperty::SetValue(const std::wstring& value)
{
	std::string text = StringConverter.to_bytes(value);
	size_t i = 0;
	for (i = 0; i < text.size(); i++)
	{
		this->ValueData[i] = text.c_str()[i];
	}
	for (i; i < ValueDataLength; i++)
	{
		this->ValueData[i] = 0;
	}
}

StringProperty::StringProperty(const std::wstring& name) : StringProperty(name, L"") { }

StringProperty::StringProperty(const std::wstring& name, const std::wstring& value) : SceneProperty(name)
{
	this->SetValue(value);
}

void StringProperty::OnGUI()
{
	ImGui::InputText(this->NameData, this->ValueData, this->ValueDataLength);
}

Vector3 Vector3Property::GetValue() const
{
	return this->Value;
}

void Vector3Property::SetValue(const Vector3& value)
{
	this->Value = value;
}

Vector3Property::Vector3Property(const std::wstring& name) : Vector3Property(name, Vector3::Zero) { }

Vector3Property::Vector3Property(const std::wstring& name, const Vector3& value) : SceneProperty(name), Value(value) { }

void Vector3Property::OnGUI()
{
	ImGui::InputFloat3(this->NameData, (float*)&this->Value, 2);
}

bool BooleanProperty::GetValue() const
{
	return this->Value;
}

void BooleanProperty::SetValue(const bool& value)
{
	this->Value = value;
}

BooleanProperty::BooleanProperty(const std::wstring& name) : BooleanProperty(name, false) { }

BooleanProperty::BooleanProperty(const std::wstring& name, const bool& value) : SceneProperty(name), Value(value) { }

void BooleanProperty::OnGUI()
{
	ImGui::Checkbox(this->NameData, &this->Value);
}

float FloatProperty::GetValue() const
{
	return this->Value;
}

void FloatProperty::SetValue(const float& value)
{
	this->Value = value;
}

FloatProperty::FloatProperty(const std::wstring& name) : FloatProperty(name, 0.0f) { }

FloatProperty::FloatProperty(const std::wstring& name, const float& value) : SceneProperty(name), Value(value) { }

void FloatProperty::OnGUI()
{
	ImGui::InputFloat(this->NameData, &this->Value, 0.1, 1.0f, 2);
}