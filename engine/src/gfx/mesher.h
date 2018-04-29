#ifndef MESHER_H
#define MESHER_H

#include "shader.h"

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
	friend class MeshFactory;
public:
	virtual ~Mesh();

	void bind(ShaderProgram* shader = nullptr);
	void unbind(ShaderProgram* shader = nullptr);

	bool indexed() const { return m_indexed; }
	bool useVertexArray() const { return m_useVertexArrays; }

	u8* map();
	void unmap();

	u32 vertexCount() const { return m_vertexCount; }
	u32 indexCount() const { return m_indexCount; }

protected:
	Mesh(bool indexed = true, bool vao = true);

	uptr<VertexFormat> m_format;

	bool m_indexed, m_useVertexArrays;

	GLuint m_vbo, m_ibo, m_vao;
	i32 m_vertexCount, m_indexCount;
};


class MeshFactory {
public:
	MeshFactory() = default;

	MeshFactory& addVertex(const Vertex& vert);

	MeshFactory& addIndex(i32 index);
	MeshFactory& addTriangle(i32 i0, i32 i1, i32 i2);

	MeshFactory& addData(const Vector<Vertex>& vertices, const Vector<i32>& indices);

	MeshFactory& addFromFile(const String& file);
	MeshFactory& addFromFile(VirtualFile* file);

	MeshFactory& calculateNormals(PrimitiveType primitive = PrimitiveType::Triangles);
	MeshFactory& calculateTangents(PrimitiveType primitive = PrimitiveType::Triangles);
	MeshFactory& transformTexCoords(const Mat4& t);

	uptr<Mesh> build(bool indexed = true, bool vao = true);

	i32 index(u32 i) const { return m_indexData[i]; }

private:
	Vector<Vertex> m_vertexData;
	Vector<i32> m_indexData;

	void triNormal(i32 i0, i32 i1, i32 i2);
	void triTangent(i32 i0, i32 i1, i32 i2);

	void addAIScene(const aiScene* scene);

};

NS_END

#endif // MESHER_H