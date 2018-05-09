#include "transform.h"

NS_BEGIN

Transform::Transform()
	:	position(Vec3()), scale(Vec3(1.0f)), rotation(Mat4(1.0f)),
		m_prevPosition(Vec3()), m_prevScale(Vec3(1.0f)), m_prevRotation(Mat4(1.0f)),
		m_dirty(true), m_inverseDirty(true), m_parent(nullptr),
		m_localToWorld(Mat4(1.0f)), m_worldToLocal(Mat4(1.0f))
{}

Mat4 Transform::localToParentMatrix() {
	return glm::translate(Mat4(1.0f), position) * rotation * glm::scale(Mat4(1.0f), scale);
}

Mat4 Transform::localToWorldMatrix() {
	if (m_dirty) {
		if (m_parent == nullptr) {
			m_localToWorld = localToParentMatrix();
		} else {
			m_localToWorld = localToParentMatrix() * m_parent->localToWorldMatrix();
		}
		m_dirty = false;
	}
	return m_localToWorld;
}

Mat4 Transform::worldToLocalMatrix() {
	if (m_inverseDirty) {
		m_worldToLocal = glm::inverse(localToWorldMatrix());
		m_inverseDirty = false;
	}
	return m_worldToLocal;
}

void Transform::setDirty() {
	if (!m_dirty) {
		m_dirty = true;
		m_inverseDirty = true;
		for (Transform* child : m_children) {
			child->setDirty();
		}
	}
}

bool Transform::changed() {
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

	if (rotation != m_prevRotation) {
		m_prevRotation = rotation;
		return true;
	}

	return false;
}

void Transform::setParent(Transform* parent) {
	if (m_parent) {
		auto pos = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), parent);
		m_parent->m_children.erase(pos);
	}

	m_parent = parent;

	if (parent) {
		parent->m_children.push_back(this);
	}

	setDirty();
}

Mat4 Transform::worldRotation() {
	Mat4 parentRotation(1.0f);

	if (m_parent) {
		parentRotation = m_parent->worldRotation();
	}

	return parentRotation * rotation;
}

void Transform::rotate(const Vec3& axis, float angle) {
	rotation = glm::rotate(rotation, angle, axis);
}

void Transform::lookAt(const Vec3& eye, const Vec3& at, const Vec3& up) {
	rotation = glm::lookAt(eye, at, up);
}

NS_END
