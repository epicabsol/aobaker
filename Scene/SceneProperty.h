#pragma once

#include <d3d12.h>
#include "../SimpleMath.h"

#include <string>

class SceneProperty
{
public:
	std::wstring GetName() const;

	SceneProperty(const std::wstring& name);
	virtual void OnGUI() = 0;
	// TODO: virtual void Write(BinaryWriter writer)
	// TODO: virtual void Read(BinaryReader reader)

protected:
	// The name is stored as a char array so dear imgui will play nice with it.
	static const size_t NameDataLength = 255;
	char NameData[NameDataLength];
};

class StringProperty : public SceneProperty
{
public:
	std::wstring GetValue() const;
	inline char* GetValueGUI() const { return (char*)this->ValueData; }
	void SetValue(const std::wstring& value);

	StringProperty(const std::wstring& name);
	StringProperty(const std::wstring& name, const std::wstring& value);
	void OnGUI() override;

private:
	static const size_t ValueDataLength = 0x0FFF;
	char ValueData[ValueDataLength];
};

class Vector3Property : public SceneProperty
{
public:
	DirectX::SimpleMath::Vector3 GetValue() const;
	void SetValue(const DirectX::SimpleMath::Vector3& value);

	Vector3Property(const std::wstring& name);
	Vector3Property(const std::wstring& name, const DirectX::SimpleMath::Vector3& value);
	void OnGUI() override;

private:
	DirectX::SimpleMath::Vector3 Value;
};

class BooleanProperty : public SceneProperty
{
public:
	bool GetValue() const;
	void SetValue(const bool& value);

	BooleanProperty(const std::wstring& name);
	BooleanProperty(const std::wstring& name, const bool& value);
	void OnGUI() override;

private:
	bool Value;
};

class FloatProperty : public SceneProperty
{
public:
	float GetValue() const;
	void SetValue(const float& value);

	FloatProperty(const std::wstring& name);
	FloatProperty(const std::wstring& name, const float& value);
	void OnGUI() override;

private:
	float Value;
};

// TODO: Add concrete subclasses here (IntProperty, RGBProperty, RGBAProperty, MaterialProperty...)