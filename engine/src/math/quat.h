#ifndef QUAT_H
#define QUAT_H

#include "vec.h"
#include "mat.h"

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
		return Quat(
			w*o.x - z*o.y + y*o.z + x*o.w,
			z*o.x + w*o.y - x*o.z + y*o.w,
			-y*o.x + x*o.y + w*o.z + z*o.w,
			-x*o.x - y*o.y - z*o.z + w*o.w
		);
	}

	Quat operator *(float o) const {
		return Quat(x * o, y * o, z * o, w * o);
	}

	Vec3 operator *(const Vec3& o) const {
		Quat q = Quat(o.x, o.y, o.z, 0.0f);
		return ((*this) * q * (*this).conjugated()).imaginary;
	}

	Vec3 forward() const { return (*this) * Vec3(0, 0, -1); }
	Vec3 right() const { return (*this) * Vec3(1, 0, 0); }
	Vec3 up() const { return (*this) * Vec3(0, 1, 0); }

};

NS_END

#endif // QUAT_H