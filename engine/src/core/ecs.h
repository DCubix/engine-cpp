#ifndef ECS_H
#define ECS_H

#include "../core/types.h"

#include <memory>
#include <typeindex>
#include <typeinfo>
#include <type_traits>

#define ECS_INVALID_ENTITY 0

NS_BEGIN

typedef std::type_index TypeIndex;
template<typename T>
static TypeIndex getTypeIndex() {
	return std::type_index(typeid(T));
}

struct Component {  virtual ~Component() = default; }; // Base for components

using ComponentMap = UMap<TypeIndex, uptr<Component>>;

class Entity {
	friend class EntityWorld;
public:
	Entity() = default;
	virtual ~Entity() = default;
	
	template <class C, typename... Args>
	C& assign(Args&&... args) {
		static_assert(
				std::is_base_of<Component, C>::value,
				"Component must be derived from 'Component'."
		);
		m_components.insert(std::make_pair(getTypeIndex<C>(), uptr<C>(new C(args...))));
		return *((C*) m_components.at(getTypeIndex<C>()).get());
	}
	
	template <class C>
	bool remove() {
		static_assert(
				std::is_base_of<Component, C>::value,
				"Component must be derived from 'Component'."
		);
		auto found = m_components.find(getTypeIndex<C>());
		if (found != m_components.end()) {
			m_components.erase(found);
			return true;
		}
		return false;
	}
	
	void removeAll();
	
	template <class C>
	bool has() const {
		static_assert(
				std::is_base_of<Component, C>::value,
				"Component must be derived from 'Component'."
		);
		return m_components.find(getTypeIndex<C>()) != m_components.end();
	}
	
	template<typename T, typename V, typename... Types>
	bool has() const {
		return has<T>() && has<V, Types...>();
	}
	
	template <class C>
	C* get() {
		static_assert(
				std::is_base_of<Component, C>::value,
				"Component must be derived from 'Component'."
		);
		auto found = m_components.find(getTypeIndex<C>());
		if (found == m_components.end()) {
			return nullptr;
		}
		return ((C*) found->second.get());
	}
	
	u64 id() const { return m_id; }
	
protected:
	ComponentMap m_components;
	u64 m_id;
};

class EntityWorld;
class EntitySystem {
public:
	virtual void update(EntityWorld& world, float dt) {}
	virtual void render(EntityWorld& world) {}
};

using EntityList = Vector<uptr<Entity>>;
using SystemList = Vector<uptr<EntitySystem>>;

class EntityWorld {
public:
	EntityWorld() = default;
	virtual ~EntityWorld() = default;
	
	Entity& create();
	void destroy(const Entity& entity);

	template<class... Cs> 
	void each(void(*f)(Entity&, Cs...)) {
		each_internal<Cs...>(f);
	}

	template<class F>
	void each(F&& func) {
		lambda_each_internal(&F::operator(), func);
	}
	
	template <class S>
	S& registerSystem() {
		static_assert(
				std::is_base_of<EntitySystem, S>::value,
				"System must be derived from 'EntitySystem'."
		);
		m_systems.push_back(uptr<S>(new S()));
		return *((S*) m_systems.back().get());
	}
	
	template <class C>
	Entity* find() {
		for (uptr<Entity>& ent : m_entities) {
			if (ent->has<C>()) return ent.get();
		}
		return nullptr;
	}
	
	void update(float dt);
	void render();
	
private:
	EntityList m_entities;
	SystemList m_systems;
	
	template<class... Cs, class F>
	void each_internal(F&& func) {
		for(uptr<Entity>& ent : m_entities) {
			if(ent->has<Cs...>()) {
				func(*ent, *ent->get<Cs>()...);
			}
		}
	}
	
	template<class G, class... Cs, class F>
	void lambda_each_internal(void (G::*)(Entity&, Cs&...), F&& f) {
		each_internal<Cs...>(std::forward<F>(f));
	}
	
	template<class G, class... Cs, class F>
	void lambda_each_internal(void (G::*)(Entity&, Cs&...) const, F&& f) {
		each_internal<Cs...>(std::forward<F>(f));
	}
};

NS_END

#endif /* ECS_H */

