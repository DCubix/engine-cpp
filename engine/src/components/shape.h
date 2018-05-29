#ifndef SHAPE_H
#define SHAPE_H

#include "../core/types.h"
#include "../core/builder.h"
#include "../core/ecs.h"

#include "../math/vec.h"

#include "../gfx/mesher.h"

#include "bullet/btBulletCollisionCommon.h"
#include "bullet/BulletCollision/CollisionShapes/btShapeHull.h"

NS_BEGIN

class CollisionShape;

class ShapeBuilder {
public:
	template <typename T>
	static T* save(T* shape) {
		g_shapes.push_back(shape);
		return shape;
	}

	static void destroy() {
		for (btCollisionShape* s : g_shapes) {
			delete s;
		}
		g_shapes.clear();
	}
private:
	static Vector<btCollisionShape*> g_shapes;
};
#define SHAPE_SAVE(s) ShapeBuilder::save(s)

template <typename St, typename Arg1>
struct CSParams1Arg {
	static St* create(Arg1 a) { return nullptr; }
};

template <typename St, typename Arg1, typename Arg2>
struct CSParams2Arg {
	static St* create(Arg1 a, Arg2 b) { return nullptr; }
};

// Some shapes
template <>
struct CSParams2Arg<btStaticPlaneShape, const Vec3&, float> {
	static btStaticPlaneShape* create(const Vec3& n, float pc) {
		return SHAPE_SAVE(new btStaticPlaneShape(btVector3(n.x, n.y, n.z), pc));
	}
};

template <>
struct CSParams1Arg<btBoxShape, const Vec3&> {
	static btBoxShape* create(const Vec3& hextents) {
		return SHAPE_SAVE(new btBoxShape(btVector3(hextents.x, hextents.y, hextents.z)));
	}
};

template <>
struct CSParams1Arg<btSphereShape, float> {
	static btSphereShape* create(float radius) {
		return SHAPE_SAVE(new btSphereShape(radius));
	}
};

template <>
struct CSParams1Arg<btCylinderShape, const Vec3&> {
	static btCylinderShape* create(const Vec3& hextents) {
		return SHAPE_SAVE(new btCylinderShape(btVector3(hextents.x, hextents.y, hextents.z)));
	}
};

template <>
struct CSParams2Arg<btCapsuleShape, float, float> {
	static btCapsuleShape* create(float radius, float height) {
		return SHAPE_SAVE(new btCapsuleShape(radius, height));
	}
};

template <>
struct CSParams1Arg<btConvexHullShape, const Vector<Vertex>&> {
	static btConvexHullShape* create(const Vector<Vertex>& vertices) {
		Vector<float> points;
		for (Vertex v : vertices) {
			points.push_back(v.position.x);
			points.push_back(v.position.y);
			points.push_back(v.position.z);
		}

		btConvexHullShape convexHullShape(points.data(), points.size());
		convexHullShape.setMargin(0);

		btShapeHull* hull = new btShapeHull(&convexHullShape);
		hull->buildHull(0);

		btConvexHullShape* ret = new btConvexHullShape(
									 (const btScalar*)hull->getVertexPointer(),
									 hull->numVertices()
		);

		delete hull;
		return SHAPE_SAVE(ret);
	}
};

template <>
struct CSParams2Arg<btBvhTriangleMeshShape, const Vector<Vertex>&, const Vector<u32>&> {
	static btBvhTriangleMeshShape* create(const Vector<Vertex>& vertices, const Vector<u32>& indices) {
		Vector<Vec3> verts;
		for (Vertex v : vertices) {
			verts.push_back(v.position);
		}

		btIndexedMesh imsh;
		imsh.m_indexType = PHY_INTEGER;
		imsh.m_numTriangles = indices.size() / 3;
		imsh.m_numVertices = vertices.size();
		imsh.m_triangleIndexBase = (const unsigned char*) indices.data();
		imsh.m_triangleIndexStride = sizeof(u32) * 3;
		imsh.m_vertexBase = (const unsigned char*) verts.data();
		imsh.m_vertexStride = sizeof(Vec3);
		imsh.m_vertexType = PHY_FLOAT;

		btTriangleIndexVertexArray *mi = new btTriangleIndexVertexArray();
		mi->addIndexedMesh(imsh);

		return SHAPE_SAVE(new btBvhTriangleMeshShape(mi, true));
	}
};

using PlaneShape = CSParams2Arg<btStaticPlaneShape, const Vec3&, float>;
using BoxShape = CSParams1Arg<btBoxShape, const Vec3&>;
using SphereShape = CSParams1Arg<btSphereShape, float>;
using CylinderShape = CSParams1Arg<btCylinderShape, const Vec3&>;
using CapsuleShape = CSParams2Arg<btCapsuleShape, float, float>;
using ConvexHullShape = CSParams1Arg<btConvexHullShape, const Vector<Vertex>&>;
using TriangleMeshShape = CSParams2Arg<btBvhTriangleMeshShape, const Vector<Vertex>&, const Vector<u32>&>;

class CollisionShape : public Component {
public:
	CollisionShape() = default;
	virtual ~CollisionShape() = default;

	CollisionShape(btCollisionShape* shape) : m_shape(shape) {}

	btCollisionShape* shape() { return m_shape; }
private:
	btCollisionShape* m_shape;
};

NS_END

#endif // SHAPE_H
