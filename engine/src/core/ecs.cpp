#include "ecs.h"

#include <algorithm>

NS_BEGIN

Entity& EntityWorld::create() {
	m_entities.push_back(uptr<Entity>(new Entity()));
	m_entities.back()->m_id = m_entities.size();
	m_recentlyCreated.push_back(m_entities.back().get());
	return *m_entities.back().get();
}

void EntityWorld::destroy(Entity& entity) {
	if (entity.id() == ECS_INVALID_ENTITY || entity.id() > m_entities.size()) {
		return;
	}
	m_recentlyDestroyed.push_back(&entity);
	m_entities.erase(m_entities.begin() + (entity.id() - 1));
}

void EntityWorld::update(float dt) {
	for (Entity* ent : m_recentlyCreated) {
		for (uptr<EntitySystem>& sys : m_systems) {
			sys->entityCreated(*this, *ent);
		}
	}
	m_recentlyCreated.clear();

	for (Entity* ent : m_recentlyDestroyed) {
		for (uptr<EntitySystem>& sys : m_systems) {
			sys->entityDestroyed(*this, *ent);
		}
	}
	m_recentlyDestroyed.clear();

	for (uptr<EntitySystem>& sys : m_systems) {
		sys->update(*this, dt);
	}
}

void EntityWorld::render() {
	for (uptr<EntitySystem>& sys : m_systems) {
		sys->render(*this);
	}
}

void Entity::removeAll() {
	m_components.clear();
}

NS_END
