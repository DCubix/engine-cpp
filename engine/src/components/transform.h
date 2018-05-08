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
	Quat rotation;
	
	Mat4 localToWorldMatrix();
	Mat4 worldToLocalMatrix();
	
	Transform* parent() { return m_parent; }
	void setParent(Transform* parent);
	
	void rotate(const Vec3& axis, float angle);
	
	bool changed();
	void setDirty();

	Vec3 worldPosition() { return (localToWorldMatrix() * Vec4(position, 1.0f)).xyz; }
	Quat worldRotation();

	Vec3 forward() { return worldRotation() * Vec3(0, 0, -1); }
	Vec3 back() { return worldRotation() * Vec3(0, 0, 1); }
	Vec3 right() { return worldRotation() * Vec3(1, 0, 0); }
	Vec3 left() { return worldRotation() * Vec3(-1, 0, 0); }
	Vec3 up() { return worldRotation() * Vec3(0, 1, 0); }
	Vec3 down() { return worldRotation() * Vec3(0, -1, 0); }
	
private:
	Transform* m_parent;
	Vector<Transform*> m_children;
	
	Vec3 m_prevPosition, m_prevScale;
	Quat m_prevRotation;
	
	bool m_dirty, m_inverseDirty;
	Mat4 m_worldToLocal, m_localToWorld;
	
	Mat4 localToParentMatrix();

};

NS_END

#endif /* TRANSFORM_H */
