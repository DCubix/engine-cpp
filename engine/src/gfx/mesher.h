#ifndef MESHER_H
#define MESHER_H

#include "shader.h"

#include "../core/builder.h"
#include "../core/types.h"
#include "../math/vec.h"

#include "../core/filesys.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NS_BEGIN

enum AttributeType {
	AttrFloat = 1,
	AttrVector2,
	AttrVector3,
	AttrVector4
};

struct VertexAttribute {
	String name;
	i32 location;
	u32 size;
	bool normalized;
};

class VertexFormat {
public:
	VertexFormat() : m_stride(0) {}
	
	void put(const String& name, AttributeType type, bool normalized, i32 location = -1);

	u32 stride() const { return m_stride; }

	void bind(ShaderProgram* shader = nullptr);
	void unbind(ShaderProgram* shader);

private:
	Vector<VertexAttribute> m_attributes;
	u32 m_stride;
};

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
};

class Mesh {
public:
	Mesh() : m_vbo(0), m_ibo(0), m_vao(0) {}
	Mesh(GLuint vbo, GLuint ibo, GLuint vao) : m_vbo(vbo), m_ibo(ibo), m_vao(vao) {
		m_format.put("vPosition", AttributeType::AttrVector3, false, 0);
		m_format.put("vNormal", AttributeType::AttrVector3, false, 1);
		m_format.put("vTangent", AttributeType::AttrVector3, false, 2);
		m_format.put("vTexCoord", AttributeType::AttrVector2, false, 3);
		m_format.put("vColor", AttributeType::AttrVector4, false, 4);
	}

	Mesh& addVertex(const Vertex& vert);

	Mesh& addIndex(i32 index);
	Mesh& addTriangle(i32 i0, i32 i1, i32 i2);

	Mesh& addData(const Vector<Vertex>& vertices, const Vector<i32>& indices);

	Mesh& addFromFile(const String& file);

	Mesh& calculateNormals(PrimitiveType primitive = PrimitiveType::Triangles);
	Mesh& calculateTangents(PrimitiveType primitive = PrimitiveType::Triangles);
	Mesh& transformTexCoords(const Mat4& t);
	
	void flush();
	
	void bind();
	void unbind();

	u8* map();
	void unmap();

	u32 vertexCount() const { return m_vertexCount; }
	u32 indexCount() const { return m_indexCount; }
	
	i32 index(u32 i) const { return m_indexData[i]; }

protected:
	VertexFormat m_format;

	GLuint m_vbo, m_ibo, m_vao;
	i32 m_vertexCount, m_indexCount;
	
	Vector<Vertex> m_vertexData;
	Vector<i32> m_indexData;
	
	void triNormal(i32 i0, i32 i1, i32 i2);
	void triTangent(i32 i0, i32 i1, i32 i2);

	void addAIScene(const aiScene* scene);
};

template <>
class Builder<Mesh> {
public:
	static Mesh build() {
		g_vbos.push_back(GLBuffer::create());
		g_ibos.push_back(GLBuffer::create());
		g_vaos.push_back(GLVertexArray::create());
		return Mesh(g_vbos.back(), g_ibos.back(), g_vaos.back());
	}
	
	static void clean() {
		for (GLuint b : g_vbos) {
			GLBuffer::destroy(b);
		}
		for (GLuint b : g_ibos) {
			GLBuffer::destroy(b);
		}
		for (GLuint b : g_vaos) {
			GLVertexArray::destroy(b);
		}
	}
private:
	static Vector<GLuint> g_vbos, g_ibos, g_vaos;
};

NS_END

#endif // MESHER_H