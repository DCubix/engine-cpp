#ifndef VEC_H
#define VEC_H

#include "../core/types.h"

#include <cmath>

#define DEF_INDEX() \
float& operator [](u32 i) { \
	return val[i]; \
} \
float operator [](u32 i) const { \
	return val[i]; \
}

#define DEF_NEG(N) \
Vec<N> operator -() const { \
	Vec<N> ret; \
	for (i32 i = 0; i < N; i++) ret.val[i] = -val[i]; \
	return ret; \
}

#define DEF_OP(N, op) \
Vec<N> operator op(const Vec<N>& o) const { \
	Vec<N> ret; \
	for (i32 i = 0; i < N; i++) ret.val[i] = val[i] op o.val[i]; \
	return ret; \
} \
Vec<N> operator op(float o) const { \
	Vec<N> ret; \
	for (i32 i = 0; i < N; i++) ret.val[i] = val[i] op o; \
	return ret; \
}

#define DEF_ASSIGN_OP(N, op) \
Vec<N>& operator op(const Vec<N>& o) { \
	for (i32 i = 0; i < N; i++) val[i] op o.val[i]; \
	return *this; \
} \
Vec<N>& operator op(float o) { \
	for (i32 i = 0; i < N; i++) val[i] op o; \
	return *this; \
}

#define DEF_COMMON(N) \
float dot(const Vec<N>& o) const { \
	float sum = 0.0f; \
	for (i32 i = 0; i < N; i++) sum += val[i] * o.val[i]; \
	return sum; \
} \
float len() const { \
	return sqrtf(dot(*this)); \
} \
Vec<N> normalized() const { \
	float l = len(); \
	Vec<N> ret; \
	for (i32 i = 0; i < N; i++) ret.val[i] = val[i] / l; \
	return ret; \
}

NS_BEGIN

template <u32 N> struct Vec { float val[N]; };

using Vec2 = Vec<2>;
using Vec3 = Vec<3>;
using Vec4 = Vec<4>;

template <>
struct Vec<2> {
	union {
		float val[2];
		struct { float x, y; };
	};

	Vec(float x, float y) : x(x), y(y) {}
	Vec(float v) : Vec(v, v) {}
	Vec() : Vec(0) {}
	
	static Vec2 fromAngle(float a) { return Vec2(cosf(a), sinf(a)); }

	float angle() const { return std::atan2(y, x); }
	float perpDot(const Vec2& o) const { return x * o.y - y * o.x; }
	Vec2 perp() const { return Vec2(-y, x); }

	DEF_COMMON(2);

	DEF_NEG(2);

	DEF_OP(2, +);
	DEF_OP(2, -);
	DEF_OP(2, *);
	DEF_OP(2, /);

	DEF_ASSIGN_OP(2, +=);
	DEF_ASSIGN_OP(2, -=);
	DEF_ASSIGN_OP(2, *=);
	DEF_ASSIGN_OP(2, /=);

	DEF_INDEX();
};

class Quat;
template <>
struct Vec<3> {
	union {
		float val[3];
		struct { float x, y, z; };
		Vec2 xy;
	};

	Vec(float x, float y, float z) : x(x), y(y), z(z) {}
	Vec(const Vec2& v, float z = 0) : Vec(v.x, v.y, z) {}
	Vec(float v) : Vec(v, v, v) {}
	Vec() : Vec(0) {}

	Vec3 cross(const Vec3& o) const {
		return Vec3(
			y * o.z - z * o.y,
			z * o.x - x * o.z,
			x * o.y - y * o.x
		);
	}
	
	DEF_COMMON(3);

	DEF_NEG(3);

	DEF_OP(3, +);
	DEF_OP(3, -);
	DEF_OP(3, *);
	DEF_OP(3, / );

	DEF_ASSIGN_OP(3, +=);
	DEF_ASSIGN_OP(3, -=);
	DEF_ASSIGN_OP(3, *=);
	DEF_ASSIGN_OP(3, /=);

	DEF_INDEX();
};

template <>
struct Vec<4> {
	union {
		float val[4];
		struct { float x, y, z, w; };
		Vec<3> xyz;
		Vec<2> xy;
	};

	Vec(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Vec(const Vec3& v, float w = 1) : Vec(v.x, v.y, v.z, w) {}
	Vec(const Vec2& v, float z = 0, float w = 1) : Vec(v.x, v.y, z, w) {}
	Vec(float v) : Vec(v, v, v, v) {}
	Vec() : Vec(0) {}

	DEF_COMMON(4);

	DEF_NEG(4);

	DEF_OP(4, +);
	DEF_OP(4, -);
	DEF_OP(4, *);
	DEF_OP(4, / );

	DEF_ASSIGN_OP(4, +=);
	DEF_ASSIGN_OP(4, -=);
	DEF_ASSIGN_OP(4, *=);
	DEF_ASSIGN_OP(4, /=);

	DEF_INDEX();
};

NS_END

#endif // VEC_H