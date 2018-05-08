#include "quat.h"

#include "vec.h"

NS_BEGIN

Quat::Quat(const Vec3& axis, float a) {
	float angle = a / 2.0f;
	float s = sinf(angle);
	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = cosf(a);
}

float Quat::magnitude() const {
	return sqrtf(x * x + y * y + z * z + w * w);
}

Quat Quat::normalized() const {
	float m = magnitude();
	return Quat(x / m, y / m, z / m, w / m);
}

Quat Quat::conjugated() const {
	return Quat(-x, -y, -z, w);
}

Mat4 Quat::toMat4() const {
	Vec3 forward(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y));
	Vec3 up(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x));
	Vec3 right(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y));

	return Mat4::rotation(forward, up, right);
}

Quat& Quat::rotate(const Vec3& axis, float a) {
	Quat o(axis, a);
	x = w*o.x - z*o.y + y*o.z + x*o.w;
	y = z*o.x + w*o.y - x*o.z + y*o.w;
	z = -y*o.x + x*o.y + w*o.z + z*o.w;
	w = -x*o.x - y*o.y - z*o.z + w*o.w;
	return *this;
}

Quat& Quat::lookAt(const Vec3& eye, const Vec3& dest) {
	const Vec3 forward = Vec3(0, 0, 1);
	Vec3 fwd = (dest - eye).normalized();

    Vec3 rotAxis = forward.cross(fwd);
    float dot = forward.dot(fwd);

    x = rotAxis.x;
    y = rotAxis.y;
    z = rotAxis.z;
    w = dot + 1;

	float m = magnitude();
	x /= m;
	y /= m;
	z /= m;
	w /= m;
	
    return *this;
}

Vec3 Quat::operator*(const Vec3& o) const {
	Quat q = Quat(o.x, o.y, o.z, 0.0);
	return ((*this) * q * conjugated()).imaginary;
}

NS_END
