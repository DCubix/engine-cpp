#include "ecs.h"

#include <algorithm>

NS_BEGIN

Entity& EntityWorld::create() {
	m_entities.push_back(uptr<Entity>(new Entity()));
	m_entities.back()->m_id = m_entities.size();
	return *m_entities.back().get();
}

void EntityWorld::destroy(const Entity& entity) {
	if (entity.id() == ECS_INVALID_ENTITY || entity.id() > m_entities.size()) {
		return;
	}
	m_entities.erase(m_entities.begin() + (entity.id() - 1));
}

void Entity::removeAll() {
	m_components.clear();
}

void EntityWorld::update(float dt) {
	for (uptr<EntitySystem>& sys : m_systems) {
		sys->update(*this, dt);
	}
}

void EntityWorld::render() {
	for (uptr<EntitySystem>& sys : m_systems) {
		sys->render(*this);
	}
}


NS_END