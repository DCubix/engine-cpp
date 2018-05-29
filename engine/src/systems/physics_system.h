#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "../core/types.h"
#include "../core/ecs.h"
#include "../components/shape.h"

#include "bullet/btBulletCollisionCommon.h"
#include "bullet/btBulletDynamicsCommon.h"

NS_BEGIN

class RigidBody : public Component {
	friend class PhysicsSystem;
public:
	RigidBody() : m_rigidBody(nullptr) {}
	RigidBody(float mass) : m_rigidBody(nullptr), m_mass(mass) {}

	btRigidBody* rigidBody() { return m_rigidBody; }
protected:
	btRigidBody *m_rigidBody;
	float m_mass;
};

class PhysicsSystem : public EntitySystem {
public:
	PhysicsSystem();
	virtual ~PhysicsSystem();

	void update(EntityWorld& world, float dt);
	void entityCreated(EntityWorld& world, Entity& ent) override;
	void entityDestroyed(EntityWorld& world, Entity& ent) override;

private:
	btBroadphaseInterface *m_broadPhase;
	btCollisionConfiguration *m_collisionConfiguration;
	btCollisionDispatcher *m_collisionDispatcher;
	btConstraintSolver *m_solver;
	btDynamicsWorld *m_world;
};

NS_END

#endif // PHYSICS_SYSTEM_H
