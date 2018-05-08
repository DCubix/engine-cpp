#ifndef QUAT_H
#define QUAT_H

#include "mat.h"
#include "vec.h"

NS_BEGIN

class Quat {
public:
	union {
		float val[4];
		struct { float x, y, z, w; };
		Vec3 imaginary;
	};

	Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Quat(const Vec3& axis, float a);
	
	float magnitude() const;
	Quat normalized() const;
	Quat conjugated() const;
	Mat4 toMat4() const;
	
	Quat& rotate(const Vec3& axis, float a);
	Quat& lookAt(const Vec3& eye, const Vec3& dest);

	Quat operator +(const Quat& o) const {
		return Quat(x + o.x, y + o.y, z + o.z, w + o.w);
	}

	Quat operator *(const Quat& o) const {
		float w_ = w * o.w - x * o.x - y * o.y - z * o.z;
		float x_ = x * o.w + w * o.x + y * o.z - z * o.y;
		float y_ = y * o.w + w * o.y + z * o.x - x * o.z;
		float z_ = z * o.w + w * o.z + x * o.y - y * o.x;
		return Quat(x_, y_, z_, w_);
	}

	Quat operator *(float o) const {
		return Quat(x * o, y * o, z * o, w * o);
	}

	Vec3 operator *(const Vec3& o) const;
	
};

NS_END

#endif // QUAT_H