#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "../core/ecs.h"

#include "../math/vec.h"
#include "../math/quat.h"
#include "../math/mat.h"

NS_BEGIN

class Transform : public Component{
public:
	Transform();
	
	Vec3 position, scale;
	Mat4 rotation;
	
	Mat4 localToWorldMatrix();
	Mat4 worldToLocalMatrix();
	
	Transform* parent() { return m_parent; }
	void setParent(Transform* parent);
	
	void rotate(const Vec3& axis, float angle);
	void lookAt(const Vec3& eye, const Vec3& at, const Vec3& up);
	
	bool changed();
	void setDirty();

	Vec3 worldPosition() { return Vec3((localToWorldMatrix() * Vec4(position, 1.0f))); }
	Mat4 worldRotation();

	Vec3 forward() { return Vec3(worldRotation() * Vec4(0, 0, -1, 0)); }
	Vec3 back() { return Vec3(worldRotation() * Vec4(0, 0, 1, 0)); }
	Vec3 right() { return Vec3(worldRotation() * Vec4(1, 0, 0, 0)); }
	Vec3 left() { return Vec3(worldRotation() * Vec4(-1, 0, 0, 0)); }
	Vec3 up() { return Vec3(worldRotation() * Vec4(0, 1, 0, 0)); }
	Vec3 down() { return Vec3(worldRotation() * Vec4(0, -1, 0, 0)); }
	
private:
	Transform* m_parent;
	Vector<Transform*> m_children;
	
	Vec3 m_prevPosition, m_prevScale;
	Mat4 m_prevRotation;
	
	bool m_dirty, m_inverseDirty;
	Mat4 m_worldToLocal, m_localToWorld;
	
	Mat4 localToParentMatrix();

};

NS_END

#endif /* TRANSFORM_H */
