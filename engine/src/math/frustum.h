#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "vec.h"
#include "mat.h"
#include "../core/types.h"

#include <cmath>

NS_BEGIN

class Plane {
public:
	Plane() = default;
	Plane(const Vec4& abcd);

	void normalize();

	Vec3 normal;
	float d; // Distance from origin;
};

class Frustum {
public:
	Frustum() = default;
	Frustum(const Mat4& mat, bool normalizePlanes = true);

	Plane planes[6];
};

NS_END

#endif // FRUSTUM_H
