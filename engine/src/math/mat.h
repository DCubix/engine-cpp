#ifndef MAT_H
#define MAT_H

#include "vec.h"
#include "consts.h"

NS_BEGIN

static auto sinCos(float a) {
	return tup(std::sinf(a), std::cosf(a));
}

static float radians(float a) {
	return a * Pi / 180.0f;
}

static float degrees(float a) {
	return a * 180.0f / Pi;
}

struct Mat4 {
	union {
		float val[16];
		Array<Vec4, 4> rows;
	};

	Mat4() : Mat4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) {}

	Mat4(const Array<float, 16>& v);
	Mat4(const Vec4& r0,
		 const Vec4& r1,
		 const Vec4& r2,
		 const Vec4& r3);
	Mat4(float m0, float m1, float m2, float m3,
		 float m4, float m5, float m6, float m7,
		 float m8, float m9, float m10, float m11,
		 float m12, float m13, float m14, float m15);

	Vec4& operator [](u32 row) {
		return rows[row];
	}

	Vec4 operator [](u32 row) const {
		return rows[row];
	}

	static Mat4 ident();
	static Mat4 translation(const Vec3& v);
	static Mat4 uniformScaling(float s);
	static Mat4 scaling(const Vec3& s);
	static Mat4 rotationX(float a);
	static Mat4 rotationY(float a);
	static Mat4 rotationZ(float a);
	static Mat4 axisAngle(const Vec3& axis, float a);
	static Mat4 ortho(float l, float r, float t, float b, float n, float f);
	static Mat4 ortho2D(float width, float height);
	static Mat4 frustum(float l, float r, float t, float b, float n, float f);
	static Mat4 perspective(float fov, float asp, float n, float f);
	static Mat4 lookAt(const Vec3& eye, const Vec3& at, const Vec3& up);

	Mat4 transposed() const;
	Mat4 inverted() const;

	Mat4 operator *(const Mat4& rhs) const {
		Array<float, 16> d = { 0 };
		Mat4 ot = rhs.transposed();
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 4; i++) {
				d[i + j * 4] = rows[j].dot(ot.rows[i]);
			}
		}
		return Mat4(d);
	}

	Vec4 operator *(const Vec4& rhs) const {
		return Vec4(
			rows[0].dot(rhs),
			rows[1].dot(rhs),
			rows[2].dot(rhs),
			rows[3].dot(rhs)
		);
	}

	Mat4 operator *(float o) const {
		return Mat4(
			rows[0] * o,
			rows[1] * o,
			rows[2] * o,
			rows[3] * o
		);
	}

private:
	Mat4 clone() const {
		Array<float, 16> arr;
		std::copy(std::begin(val), std::end(val), arr.begin());
		return Mat4(arr);
	}
};

#endif // MAT_H