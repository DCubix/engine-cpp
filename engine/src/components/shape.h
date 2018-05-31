#ifndef SHAPE_H
#define SHAPE_H

#include "../core/types.h"
#include "../core/builder.h"
#include "../core/ecs.h"

#include "../math/vec.h"

#include "../gfx/mesher.h"

#include "bullet/btBulletCollisionCommon.h"
#include "bullet/BulletCollision/CollisionShapes/btShapeHull.h"

#include "quickhull.h"

NS_BEGIN

class CollisionShape;

struct ShapeWrapper {
	u32 id;
	btCollisionShape *shape;
	ShapeWrapper() {}
	ShapeWrapper(u32 id, btCollisionShape* shape) : id(id), shape(shape) {}
};

class ShapeBuilder {
public:
	static ShapeWrapper save(btCollisionShape* shape) {
		ShapeWrapper sw(0, shape);
		sw.id = g_shapes.size();
		g_shapes.push_back(sw);
		return sw;
	}

	static void destroy() {
		for (ShapeWrapper s : g_shapes) {
			if (s.shape) delete s.shape;
		}
		g_shapes.clear();
	}

	static void destroy(u32 id) {
		assert(id < g_shapes.size());
		delete g_shapes[id].shape;
		g_shapes[id].shape = nullptr;
	}
private:
	static Vector<ShapeWrapper> g_shapes;
};
#define SHAPE_SAVE(s) ShapeBuilder::save(s)

template <typename St, typename Arg1>
struct CSParams1Arg {
	static ShapeWrapper create(Arg1 a) { return nullptr; }
};

template <typename St, typename Arg1, typename Arg2>
struct CSParams2Arg {
	static ShapeWrapper create(Arg1 a, Arg2 b) { return nullptr; }
};

// Some shapes
template <>
struct CSParams2Arg<btStaticPlaneShape, const Vec3&, float> {
	static ShapeWrapper create(const Vec3& n, float pc) {
		return SHAPE_SAVE(new btStaticPlaneShape(btVector3(n.x, n.y, n.z), pc));
	}
};

template <>
struct CSParams1Arg<btBoxShape, const Vec3&> {
	static ShapeWrapper create(const Vec3& hextents) {
		return SHAPE_SAVE(new btBoxShape(btVector3(hextents.x, hextents.y, hextents.z)));
	}
};

template <>
struct CSParams1Arg<btSphereShape, float> {
	static ShapeWrapper create(float radius) {
		return SHAPE_SAVE(new btSphereShape(radius));
	}
};

template <>
struct CSParams1Arg<btCylinderShape, const Vec3&> {
	static ShapeWrapper create(const Vec3& hextents) {
		return SHAPE_SAVE(new btCylinderShape(btVector3(hextents.x, hextents.y, hextents.z)));
	}
};

template <>
struct CSParams2Arg<btCapsuleShape, float, float> {
	static ShapeWrapper create(float radius, float height) {
		return SHAPE_SAVE(new btCapsuleShape(radius, height));
	}
};

template <>
struct CSParams1Arg<btConvexHullShape, const Vector<Vertex>&> {
	static ShapeWrapper create(const Vector<Vertex>& vertices) {
		Vector<qh_vertex_t> points;
		for (Vertex v : vertices) {
			qh_vertex_t vt;
			vt.x = v.position.x;
			vt.y = v.position.y;
			vt.z = v.position.z;
			points.push_back(vt);
		}
		qh_mesh_t mesh = qh_quickhull3d(points.data(), points.size());

		Vector<float> verts;
		for (u32 i = 0; i < mesh.nvertices; i++) {
			verts.push_back(mesh.vertices[i].x);
			verts.push_back(mesh.vertices[i].y);
			verts.push_back(mesh.vertices[i].z);
		}

		qh_free_mesh(mesh);

		btConvexHullShape* ret = new btConvexHullShape(verts.data(), mesh.nvertices, sizeof(btVector3));

		return SHAPE_SAVE(ret);
	}
};

template <>
struct CSParams2Arg<btBvhTriangleMeshShape, const Vector<Vertex>&, const Vector<u32>&> {
	static ShapeWrapper create(const Vector<Vertex>& vertices, const Vector<u32>& indices) {
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

	CollisionShape(const ShapeWrapper& shape) {
		this->m_shape.id = shape.id;
		this->m_shape.shape = shape.shape;
	}

	ShapeWrapper shape() const { return m_shape; }
private:
	ShapeWrapper m_shape;
};

NS_END

#endif // SHAPE_H
