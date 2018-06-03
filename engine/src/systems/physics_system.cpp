#include "physics_system.h"

#include "../components/transform.h"
#include "../core/logging/log.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "../math/glm/gtx/matrix_decompose.hpp"

NS_BEGIN

PhysicsSystem::PhysicsSystem() {
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_collisionDispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_broadPhase = new btDbvtBroadphase();
	m_solver = new btSequentialImpulseConstraintSolver();
	m_world = new btDiscreteDynamicsWorld(
				  m_collisionDispatcher,
				  m_broadPhase,
				  m_solver,
				  m_collisionConfiguration
	);
	m_world->setGravity(btVector3(0, -10, 0));

	m_debugDraw = new EngDebugDraw();
	m_debugDraw->setDebugMode(
				btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb
	);
	m_world->setDebugDrawer(m_debugDraw);
}

PhysicsSystem::~PhysicsSystem() {
	ShapeBuilder::destroy();
	SAFE_RELEASE(m_broadPhase);
	SAFE_RELEASE(m_collisionConfiguration);
	SAFE_RELEASE(m_collisionDispatcher);
	SAFE_RELEASE(m_solver);
	SAFE_RELEASE(m_debugDraw);
//	SAFE_RELEASE(m_world);
}

void PhysicsSystem::update(EntityWorld& world, float dt) {
	m_world->stepSimulation(dt, 10);
	world.each([=](Entity& ent, RigidBody& R, Transform& T) {
		btRigidBody *body = R.rigidBody();

		btTransform trans; body->getMotionState()->getWorldTransform(trans);

		Vec3 position(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
		Quat rotation(
					trans.getRotation().getW(),
					trans.getRotation().getX(),
					trans.getRotation().getY(),
					trans.getRotation().getZ()
		);

		T.position = position;
		T.rotation = rotation;
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

	btCollisionShape* shape = S->shape().shape;

	btVector3 inertia(0, 0, 0);
	if (R->m_mass != 0.0f)
		shape->calculateLocalInertia(R->m_mass, inertia);

	btTransform xform(
				btQuaternion(rot.x, rot.y, rot.z, rot.w),
				btVector3(pos.x, pos.y, pos.z)
	);
	btDefaultMotionState *mst = new btDefaultMotionState(xform);
	btRigidBody::btRigidBodyConstructionInfo rbinfo(R->m_mass, mst, shape, inertia);
	rbinfo.m_startWorldTransform = xform;

	R->m_rigidBody = new btRigidBody(rbinfo);
	m_world->addRigidBody(R->m_rigidBody);
}

void PhysicsSystem::entityDestroyed(EntityWorld& world, Entity& ent) {
	bool entOK = ent.has<RigidBody, CollisionShape>();
	if (!entOK) return;

	CollisionShape* S = ent.get<CollisionShape>();
	RigidBody* R = ent.get<RigidBody>();

	m_world->removeRigidBody(R->rigidBody());

	ShapeBuilder::destroy(S->shape().id);
	delete R->m_rigidBody;
}

NS_END
