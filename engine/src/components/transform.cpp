#include "transform.h"

NS_BEGIN

Transform::Transform()
	:	position(Vec3()), scale(Vec3(1.0f)), rotation(Quat(0, 0, 0, 1)),
		m_prevPosition(Vec3()), m_prevScale(Vec3(1.0f)), m_prevRotation(Quat(0, 0, 0, 1)),
		m_dirty(true), m_inverseDirty(true), m_parent(nullptr),
		m_localToWorld(Mat4::ident()), m_worldToLocal(Mat4::ident())
{}

Mat4 Transform::localToParentMatrix() {
	return Mat4::translation(position) *
			rotation.toMat4() *
			Mat4::scaling(scale);
}

Mat4 Transform::localToWorldMatrix() {
	if (m_dirty) {
		if (m_parent == nullptr) {
			m_localToWorld = localToParentMatrix();
		} else {
			m_localToWorld = m_parent->localToWorldMatrix() * localToParentMatrix();
		}
		m_dirty = false;
	}
	return m_localToWorld;
}

Mat4 Transform::worldToLocalMatrix() {
	if (m_inverseDirty) {
		m_worldToLocal = localToWorldMatrix().inverted();
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

	if (rotation.x != m_prevRotation.x ||
		rotation.y != m_prevRotation.y ||
		rotation.z != m_prevRotation.z ||
		rotation.w != m_prevRotation.w) {
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

NS_END
