#include "quat.h"

NS_BEGIN

Quat::Quat(const Vec3& axis, float a) {
	float angle = a / 2.0f;
	float s = std::sinf(angle);
	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = std::cosf(a);
}

float Quat::magnitude() const {
	return std::sqrtf(x * x + y * y + z * z + w * w);
}

Quat Quat::normalized() const {
	float m = magnitude();
	return Quat(x / m, y / m, z / m, w / m);
}

Quat Quat::conjugated() const {
	return Quat(-x, -y, -z, w);
}

Mat4 Quat::toMat4() const {
	return Mat4(
		Vec4((*this) * Vec3(1.0, 0.0, 0.0), 0.0f),
		Vec4((*this) * Vec3(0.0, 1.0, 0.0), 0.0f),
		Vec4((*this) * Vec3(0.0, 0.0, 1.0), 0.0f),
		Vec4(0.0, 0.0, 0.0, 1.0)
	);
}

NS_END
