#include "transform.h"

NS_BEGIN

Transform::Transform()
	:	position(Vec3()), scale(Vec3(1.0f)), rotation(Quat(0, 0, 0, 1.0f)),
		m_prevPosition(Vec3()), m_prevScale(Vec3(1.0f)), m_prevRotation(Quat(0, 0, 0, 1.0f)),
		m_parentMatrix(Mat4(1.0f)), m_dirty(false), m_parent(nullptr)
{}

bool Transform::changed() {
	if (m_parent && m_parent->changed()) {
		return true;
	}

	if (position.x != m_prevPosition.x ||
		position.y != m_prevPosition.y ||
		position.z != m_prevPosition.z) {
		m_prevPosition = position;
		return true;
	}

	if (scale.x != m_prevScale.x ||
		scale.y != m_prevScale.y ||
		scale.z != m_prevScale.z) {
		m_prevScale = scale;
		return true;
	}

	if (rotation.x != m_prevRotation.x ||
		rotation.y != m_prevRotation.y ||
		rotation.z != m_prevRotation.z ||
		rotation.w != m_prevRotation.w)
	{
		m_prevRotation = rotation;
		return true;
	}

	return false;
}

void Transform::update() {
	if(m_dirty) {
		m_prevPosition = position;
		m_prevRotation = rotation;
		m_prevScale = scale;
	} else {
		m_prevPosition = position + Vec3(1.0f);
		m_prevRotation = rotation * 0.5f;
		m_prevScale = scale + Vec3(1.0f);
		m_dirty = true;
	}
}

void Transform::setParent(Transform* parent) {
	m_parent = parent;
}

Quat Transform::worldRotation() {
	Quat parentRotation(1, 0, 0, 0);

	if (m_parent) {
		parentRotation = m_parent->worldRotation();
	}

	return parentRotation * rotation;
}

Mat4 Transform::getTransformation() {
	update();
	return getParentTransform() *
			glm::translate(Mat4(1.0f), position) *
			glm::mat4_cast(rotation) *
			glm::scale(Mat4(1.0f), scale);
}

Mat4 Transform::getTransformationLocal() {
	update();
	return glm::translate(Mat4(1.0f), position) *
			glm::mat4_cast(rotation) *
			glm::scale(Mat4(1.0f), scale);
}

Mat4 Transform::getParentTransform() {
	if (m_parent && m_parent->changed()) {
		m_parentMatrix = m_parent->getTransformation();
	}
	return m_parentMatrix;
}

void Transform::setFromMatrix(const Mat4& mat) {
	Mat4 m = mat;
	position = Vec3(m[3]);
	m[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

	scale = Vec3(
			glm::length(Vec3(m[0])),
			glm::length(Vec3(m[1])),
			glm::length(Vec3(m[2]))
	);

	m[0] /= scale.x;
	m[1] /= scale.y;
	m[2] /= scale.z;

	rotation = glm::quat_cast(m);
}

void Transform::rotate(const Quat& rot) {
	rotation *= rot;
}

void Transform::rotate(const Vec3& axis, float angle) {
	rotate(glm::angleAxis(angle, axis));
}

void Transform::lookAt(const Vec3& eye, const Vec3& at, const Vec3& up) {
	rotation = glm::lookAt(eye, at, up);
}

NS_END
