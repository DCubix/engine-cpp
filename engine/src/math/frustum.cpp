#include "frustum.h"

NS_BEGIN

Plane::Plane(const Vec4 &abcd)
		: normal(abcd.x, abcd.y, abcd.z), d(abcd.w)
{
}

void Plane::normalize() {
	float mag = glm::length(normal);
	normal /= mag;
	d /= mag;
}

NS_END
Frustum::Frustum(const Mat4& mat, bool normalizePlanes) {
	planes[0] = Plane(mat[3]+mat[0]);       // left
	planes[1] = Plane(mat[3]-mat[0]);       // right
	planes[2] = Plane(mat[3]-mat[1]);       // top
	planes[3] = Plane(mat[3]+mat[1]);       // bottom
	planes[4] = Plane(mat[3]+mat[2]);       // near
	planes[5] = Plane(mat[3]-mat[2]);       // far

	if (normalizePlanes) {
		planes[0].normalize();
		planes[1].normalize();
		planes[2].normalize();
		planes[3].normalize();
		planes[4].normalize();
		planes[5].normalize();
	}
}
