#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "../core/ecs.h"

#include "../math/vec.h"
#include "../math/quat.h"
#include "../math/mat.h"

NS_BEGIN

class Transform : public Component {
public:
	Transform();

	Vec3 position, scale;
	Quat rotation;

	Transform* parent() { return m_parent; }
	void setParent(Transform* parent);

	void rotate(const Quat& rot);
	void rotate(const Vec3& axis, float angle);
	void lookAt(const Vec3& eye, const Vec3& at, const Vec3& up);

	bool changed();

	Vec3 worldPosition() { return Vec3((getParentTransform() * Vec4(position, 1.0f))); }
	Quat worldRotation();

	Vec3 forward() { return Vec3(worldRotation() * Vec4(0, 0, -1, 0)); }
	Vec3 right() { return Vec3(worldRotation() * Vec4(1, 0, 0, 0)); }
	Vec3 up() { return Vec3(worldRotation() * Vec4(0, 1, 0, 0)); }

	Mat4 getTransformation();
	Mat4 getTransformationLocal();
	Mat4 getParentTransform();

	// Utilities
	bool moveTowards(const Vec3& target, float mdd);

	void setFromMatrix(const Mat4& mat);

private:
	Transform* m_parent;

	Vec3 m_prevPosition, m_prevScale;
	Quat m_prevRotation;

	Mat4 m_parentMatrix;

	bool m_dirty;

	void update();
};

NS_END

#endif /* TRANSFORM_H */
