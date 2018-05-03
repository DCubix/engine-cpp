#include "quat.h"

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
	Mat4 ret = Mat4::ident();

	float sqw = w * w;
	float sqx = x * x;
	float sqy = y * y;
	float sqz = z * z;

	// invs (inverse square length) is only required if quaternion is not already normalised
	float invs = 1.0f / (sqx + sqy + sqz + sqw);
	ret[0][0] = (sqx - sqy - sqz + sqw) * invs; // since sqw + sqx + sqy + sqz =1/invs*invs
	ret[1][1] = (-sqx + sqy - sqz + sqw) * invs;
	ret[2][2] = (-sqx - sqy + sqz + sqw) * invs;

	float tmp1 = x * y;
	float tmp2 = z * w;
	ret[1][0] = 2.0f * (tmp1 + tmp2) * invs;
	ret[0][1] = 2.0f * (tmp1 - tmp2) * invs;

	tmp1 = x * z;
	tmp2 = y * w;
	ret[2][0] = 2.0f * (tmp1 - tmp2) * invs;
	ret[0][2] = 2.0f * (tmp1 + tmp2) * invs;
	tmp1 = y * z;
	tmp2 = x * w;
	ret[2][1] = 2.0f * (tmp1 + tmp2) * invs;
	ret[1][2] = 2.0f * (tmp1 - tmp2) * invs;

	return ret;
}

Quat& Quat::rotate(const Vec3& axis, float a) {
	Quat o(axis, a);
	x = w*o.x - z*o.y + y*o.z + x*o.w;
	y = z*o.x + w*o.y - x*o.z + y*o.w;
	z = -y*o.x + x*o.y + w*o.z + z*o.w;
	w = -x*o.x - y*o.y - z*o.z + w*o.w;
	return *this;
}

NS_END
