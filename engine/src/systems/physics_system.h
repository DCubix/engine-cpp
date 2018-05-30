#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "../core/types.h"
#include "../core/ecs.h"
#include "../components/shape.h"

#include "../gfx/imm.h"

#include "bullet/btBulletCollisionCommon.h"
#include "bullet/btBulletDynamicsCommon.h"

NS_BEGIN

class EngDebugDraw : public btIDebugDraw {
public:
	EngDebugDraw() = default;

	void reportErrorWarning(const char* warningString) {}

	void draw3dText(const btVector3& location,const char* textString) {}

	void setDebugMode(int debugMode) {}

	int getDebugMode() const {}

	void drawContactPoint(
			const btVector3& PointOnB,
			const btVector3& normalOnB,
			btScalar distance,
			int lifeTime,
			const btVector3& color)
	{

	}

	void clearLines() {
		Imm::begin(PrimitiveType::Lines);
	}

	void flushLines() {
		Imm::end();
	}

	void drawLine(
			const btVector3& from,
			const btVector3& to,
			const btVector3& color)
	{
		Vec4 col(color.x(), color.y(), color.z(), 1.0f);

		Imm::vertex(Vec3(from.x(), from.y(), from.z()), col);
		Imm::vertex(Vec3(to.x(), to.y(), to.z()), col);
	}
};

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
	void render(EntityWorld& world);
	void entityCreated(EntityWorld& world, Entity& ent) override;
	void entityDestroyed(EntityWorld& world, Entity& ent) override;

private:
	btBroadphaseInterface *m_broadPhase;
	btCollisionConfiguration *m_collisionConfiguration;
	btCollisionDispatcher *m_collisionDispatcher;
	btConstraintSolver *m_solver;
	btDynamicsWorld *m_world;
	EngDebugDraw *m_debugDraw;
};

NS_END

#endif // PHYSICS_SYSTEM_H
