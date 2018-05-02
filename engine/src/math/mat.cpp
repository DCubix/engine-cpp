#include "mat.h"

NS_BEGIN

Mat4::Mat4(const Array<float, 16>& v) {
	rows[0] = Vec4(v[0], v[1], v[2], v[3]);
	rows[1] = Vec4(v[4], v[5], v[6], v[7]);
	rows[2] = Vec4(v[8], v[9], v[10], v[11]);
	rows[3] = Vec4(v[12], v[13], v[14], v[15]);
}

Mat4::Mat4(const Vec4& r0, const Vec4& r1, const Vec4& r2, const Vec4& r3) {
	rows[0] = r0;
	rows[1] = r1;
	rows[2] = r2;
	rows[3] = r3;
}

Mat4::Mat4(float m0, float m1, float m2, float m3,
		   float m4, float m5, float m6, float m7,
		   float m8, float m9, float m10, float m11,
		   float m12, float m13, float m14, float m15) {
	rows[0] = Vec4(m0, m1, m2, m3);
	rows[1] = Vec4(m4, m5, m6, m7);
	rows[2] = Vec4(m8, m9, m10, m11);
	rows[3] = Vec4(m12, m13, m14, m15);
}

Mat4 Mat4::ident() {
	return Mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}

Mat4 Mat4::translation(const Vec3& v) {
	return Mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		v.x, v.y, v.z, 1
	);
}

Mat4 Mat4::uniformScaling(float s) {
	return Mat4(
		s, 0, 0, 0,
		0, s, 0, 0,
		0, 0, s, 0,
		0, 0, 0, 1
	);
}

Mat4 Mat4::scaling(const Vec3& s) {
	return Mat4(
		s.x, 0, 0, 0,
		0, s.y, 0, 0,
		0, 0, s.z, 0,
		0, 0, 0, 1
	);
}

Mat4 Mat4::rotationX(float a) {
	auto t = sinCos(a);
	float s = std::get<0>(t);
	float c = std::get<1>(t);
	return Mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, c, -s, 0.0f,
		0.0f, s, c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

Mat4 Mat4::rotationY(float a) {
	auto t = sinCos(a);
	float s = std::get<0>(t);
	float c = std::get<1>(t);
	return Mat4(
		c, 0.0f, -s, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		s, 0.0f, c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

Mat4 Mat4::rotationZ(float a) {
	auto t = sinCos(a);
	float s = std::get<0>(t);
	float c = std::get<1>(t);
	return Mat4(
		c, -s, 0.0f, 0.0f,
		s, c, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

Mat4 Mat4::axisAngle(const Vec3& axis, float a) {
	auto tp = sinCos(a);
	float s = std::get<0>(tp);
	float c = std::get<1>(tp);
	float t = 1.0f - c;
	Vec3 ax = axis.normalized();
	float x = ax.x;
	float y = ax.y;
	float z = ax.z;

	return Mat4(
		t * x * x + c, t * x * y - z * s, t * x * z + y * s, 0.0f,
		t * x * y + z * s, t * y * y + c, t * y * z - x * s, 0.0f,
		t * x * z - y * s, t * y * z + x * s, t * z * z + c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

Mat4 Mat4::ortho(float l, float r, float t, float b, float n, float f) {
	float w = r - l;
	float h = t - b;
	float d = f - n;
	return Mat4(
		2.0f / w, 0.0f, 0.0f, -(r + l) / w,
		0.0f, 2.0f / h, 0.0f, -(t + b) / h,
		0.0f, 0.0f, -2.0f / d, -(f + n) / d,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

Mat4 Mat4::ortho2D(float width, float height) {
	return Mat4::ortho(0, width, height, 0, -1, 1);
}

Mat4 Mat4::frustum(float l, float r, float t, float b, float n, float f) {
	float n2 = 2.0f * n;
	float w = r - l;
	float h = b - t;
	float d = f - n;
	return Mat4(
		n2 / w, 0.0f, 0.0f, 0.0f,
		0.0f, n2 / h, 0.0f, 0.0f,
		(r + l) / w, (t + b) / h, (-f - n) / d, -1.0f,
		0.0f, 0.0f, (-n2 * f) / d, 0.0f
	);
}

Mat4 Mat4::perspective(float fov, float asp, float n, float f) {
	float ymax = n * tanf(fov);
	float xmax = ymax * asp;
	return Mat4::frustum(-xmax, xmax, -ymax, ymax, n, f);
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& at, const Vec3& up) {
	Vec3 z = (eye - at).normalized();
	Vec3 x = up.cross(z).normalized();
	Vec3 y = z.cross(x);

	Mat4 r = Mat4(
		x.x, x.y, -x.z, 0.0f,
		y.x, y.y, -y.z, 0.0f,
		z.x, z.y, -z.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return Mat4::translation(-eye) * r;
}

Mat4 Mat4::transposed() const {
	Vec4 a = rows[0];
	Vec4 b = rows[1];
	Vec4 c = rows[2];
	Vec4 d = rows[3];
	return Mat4(
		a.x, b.x, c.x, d.x,
		a.y, b.y, c.y, d.y,
		a.z, b.z, c.z, d.z,
		a.w, b.w, c.w, d.w
	);
}

Mat4 Mat4::inverted() const {
	//
	// Inversion by Cramer's rule.  Code taken from an Intel publication
	//
	Mat4 mat = clone();
	Array<float, 12> tmp = { 0 };
	Array<float, 16> src = { 0 };

	// Transpose
	for (int i = 0; i < 4; i++) {
		src[i + 0] = (*this)[i][0];
		src[i + 4] = (*this)[i][1];
		src[i + 8] = (*this)[i][2];
		src[i + 12] = (*this)[i][3];
	}

	// Calculate pairs for first 8 elements (cofactors)
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];

	// Calculate first 8 elements (cofactors)
	mat[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
	mat[0][0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
	mat[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
	mat[0][1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
	mat[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
	mat[0][2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
	mat[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
	mat[0][3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
	mat[1][0] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
	mat[1][0] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
	mat[1][1] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
	mat[1][1] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
	mat[1][2] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
	mat[1][2] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
	mat[1][3] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
	mat[1][3] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];

	// Calculate pairs for second 8 elements (cofactors)
	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];
	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];

	// Calculate second 8 elements (cofactors)
	mat[2][0] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
	mat[2][0] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
	mat[2][1] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
	mat[2][1] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
	mat[2][2] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
	mat[2][2] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
	mat[2][3] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
	mat[2][3] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
	mat[3][0] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
	mat[3][0] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
	mat[3][1] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
	mat[3][1] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
	mat[3][2] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
	mat[3][2] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
	mat[3][3] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
	mat[3][3] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];

	// Calculate determinant
	float det = 1.0f / (src[0] * mat[0][0] +
						src[1] * mat[0][1] +
						src[2] * mat[0][2] +
						src[3] * mat[0][3]);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			mat[i][j] = mat[i][j] * det;
		}
	}
	return mat;
}

NS_END
