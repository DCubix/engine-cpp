#ifndef MESHER_H
#define MESHER_H

#include "shader.h"

#include "../core/builder.h"
#include "../core/types.h"
#include "../math/vec.h"
#include "../math/aabb.h"

#include "../core/filesys.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "api.h"
using namespace api;

NS_BEGIN

struct Vertex {
	Vec3 position;
	Vec3 normal;
	Vec3 tangent;
	Vec2 texCoord;
	Vec4 color;

	Vertex()
		: position(Vec3()), normal(Vec3()), tangent(Vec3()), texCoord(Vec2()), color(Vec4(1.0f))
	{}
	Vertex(const Vec3& pos)
		: position(pos), normal(Vec3()), tangent(Vec3()), texCoord(Vec2()), color(Vec4(1.0f))
	{}
	Vertex(const Vec3& pos, const Vec3& nrm)
		: position(pos), normal(nrm), tangent(Vec3()), texCoord(Vec2()), color(Vec4(1.0f))
	{}
	Vertex(const Vec3& pos, const Vec3& nrm, const Vec3& tan)
		: position(pos), normal(nrm), tangent(tan), texCoord(Vec2()), color(Vec4(1.0f))
	{}
	Vertex(const Vec3& pos, const Vec3& nrm, const Vec3& tan, const Vec2& uv)
		: position(pos), normal(nrm), tangent(tan), texCoord(uv), color(Vec4(1.0f))
	{}
	Vertex(const Vec3& pos, const Vec3& nrm, const Vec3& tan, const Vec2& uv, const Vec4& col)
		: position(pos), normal(nrm), tangent(tan), texCoord(uv), color(col)
	{}
	Vertex(const Vec3& pos, const Vec2& uv)
		: position(pos), texCoord(uv), normal(Vec3()), tangent(Vec3()), color(Vec4(1.0f))
	{}
	Vertex(const Vec3& pos, const Vec2& uv, const Vec3& nrm)
		: position(pos), texCoord(uv), normal(nrm), tangent(Vec3()), color(Vec4(1.0f))
	{}
};

enum Axis {
	X,
	Y,
	Z
};

class Mesh {
	friend class Builder<Mesh>;
public:
	Mesh() : m_vertexCount(0), m_indexCount(0) {}
	Mesh(const VertexBuffer& vbo, const VertexBuffer& ibo, const VertexArray& vao)
		: m_vbo(vbo), m_ibo(ibo), m_vao(vao),
		m_vertexCount(0), m_indexCount(0)
	{}

	Mesh& addVertex(const Vertex& vert);

	Mesh& addIndex(u32 index);
	Mesh& addTriangle(u32 i0, u32 i1, u32 i2);

	Mesh& addPlane(Axis axis, float size, const Vec3& off, bool flip = false);
	Mesh& addCube(float size, bool flip = false);

	Mesh& addData(const Vector<Vertex>& vertices, const Vector<u32>& indices);

	Mesh& addFromFile(const String& file);

	Mesh& calculateNormals(PrimitiveType primitive = PrimitiveType::Triangles);
	Mesh& calculateTangents(PrimitiveType primitive = PrimitiveType::Triangles);
	Mesh& transformTexCoords(const Mat4& t);

	void flush();

	void bind();
	void unbind();

	void drawIndexed(PrimitiveType primitive, u32 start = 0, u32 count = 0);
	void drawIndexedInstanced(PrimitiveType primitive, u32 instances, u32 start = 0, u32 count = 0);

	u8* map();
	void unmap();

	u32 vertexCount() const { return m_vertexCount; }
	u32 indexCount() const { return m_indexCount; }

	Vector<Vertex> vertexData() const { return m_vertexData; }
	Vector<u32> indexData() const { return m_indexData; }

	u32 index(u32 i) const { return m_indexData[i]; }

	AABB aabb() const { return m_aabb; }

	bool valid() const {
		return m_vbo.id() != 0 && m_vao.id() != 0 && m_ibo.id() != 0 && m_vertexCount > 0 && m_indexCount > 0;
	}

	void invalidate() {
		m_vbo = VertexBuffer();
		m_ibo = VertexBuffer();
		m_vao = VertexArray();
	}

protected:
	VertexBuffer m_vbo, m_ibo;
	VertexArray m_vao;
	u32 m_vertexCount, m_indexCount;

	Vector<Vertex> m_vertexData;
	Vector<u32> m_indexData;

	AABB m_aabb;

	void triNormal(u32 i0, u32 i1, u32 i2);
	void triTangent(u32 i0, u32 i1, u32 i2);

	void addAIScene(const aiScene* scene);

};

template <>
class Builder<Mesh> {
public:
	static Mesh build() {
		return Mesh(
					Builder<VertexBuffer>::build(),
					Builder<VertexBuffer>::build(),
					Builder<VertexArray>::build()
		);
	}

	static void destroy(Mesh ob) {
		if (ob.valid()) {
			Builder<VertexBuffer>::destroy(ob.m_vbo);
			Builder<VertexBuffer>::destroy(ob.m_ibo);
			Builder<VertexArray>::destroy(ob.m_vao);
		}
	}
};

NS_END

#endif // MESHER_H
