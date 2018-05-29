#include "physics_system.h"

#include "../components/transform.h"
#include "../core/logging/log.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "../math/glm/gtx/matrix_decompose.hpp"

NS_BEGIN

PhysicsSystem::PhysicsSystem() {
	m_broadPhase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_collisionDispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver();
	m_world = new btDiscreteDynamicsWorld(
				  m_collisionDispatcher,
				  m_broadPhase,
				  m_solver,
				  m_collisionConfiguration
	);
	m_world->setGravity(btVector3(0, -10, 0));
}

PhysicsSystem::~PhysicsSystem() {
	ShapeBuilder::destroy();
	SAFE_RELEASE(m_broadPhase);
	SAFE_RELEASE(m_collisionConfiguration);
	SAFE_RELEASE(m_collisionDispatcher);
	SAFE_RELEASE(m_solver);
//	SAFE_RELEASE(m_world);
}

void PhysicsSystem::update(EntityWorld& world, float dt) {
	m_world->stepSimulation(dt, 10);

	world.each([=](Entity& ent, RigidBody& R, Transform& T) {
		btRigidBody *body = R.rigidBody();
		btTransform trans; body->getMotionState()->getWorldTransform(trans);

		Vec3 pos(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
		Quat rotation(
					trans.getRotation().getW(),
					trans.getRotation().getX(),
					trans.getRotation().getY(),
					trans.getRotation().getZ()
		);

//		LogInfo(pos.x, ", ", pos.y, ", ", pos.z);

		T.position.x = pos.x;
		T.position.y = pos.y;
		T.position.z = pos.z;
		T.rotation.x = rotation.x;
		T.rotation.y = rotation.y;
		T.rotation.z = rotation.z;
		T.rotation.w = rotation.w;
	});
}

void PhysicsSystem::entityCreated(EntityWorld& world, Entity& ent) {
	bool entOK = ent.has<Transform, RigidBody, CollisionShape>();
	if (!entOK) return;

	Transform* T = ent.get<Transform>();
	RigidBody* R = ent.get<RigidBody>();
	CollisionShape* S = ent.get<CollisionShape>();

	Vec3 pos = T->position;
	Quat rot = T->rotation;

//	LogInfo(pos.x, ", ", pos.y, ", ", pos.z);

	btTransform st;
	st.setIdentity();
	st.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
	st.setOrigin(btVector3(pos.x, pos.y, pos.z));

	btVector3 inertia(0, 0, 0);
	if (R->m_mass != 0.0f)
		S->shape()->calculateLocalInertia(R->m_mass, inertia);

	btDefaultMotionState *mst = new btDefaultMotionState(st);
	btRigidBody::btRigidBodyConstructionInfo rbinfo(R->m_mass, mst, S->shape(), inertia);
	R->m_rigidBody = new btRigidBody(rbinfo);

//	btVector3 p = R->rigidBody()->getWorldTransform().getOrigin();
//	LogInfo(p.x(), ", ", p.y(), ", ", p.z());

	m_world->addRigidBody(R->m_rigidBody);
}

void PhysicsSystem::entityDestroyed(EntityWorld& world, Entity& ent) {
	bool entOK = ent.has<RigidBody, CollisionShape>();
	if (!entOK) return;

	CollisionShape* S = ent.get<CollisionShape>();
	RigidBody* R = ent.get<RigidBody>();

	m_world->removeRigidBody(R->rigidBody());

	delete S->shape();
	delete R->m_rigidBody;
}

NS_END
