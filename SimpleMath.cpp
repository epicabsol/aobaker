#include <d3d12.h>
#include "SimpleMath.h"

using namespace DirectX::SimpleMath;

const Vector2 Vector2::Zero = Vector2();
const Vector3 Vector3::Zero = Vector3();
const Vector3 Vector3::One = Vector3(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::UnitX = Vector3(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::UnitY = Vector3(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::UnitZ = Vector3(0.0f, 0.0f, 1.0f);
const Vector4 Vector4::Zero = Vector4();
const Vector4 Vector4::One = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
const Matrix Matrix::Identity = Matrix();