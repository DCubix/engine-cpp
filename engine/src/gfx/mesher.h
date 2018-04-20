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

class Model {
public:
	Model(bool dynamic = false, bool indexed = true, bool vao = true);
	virtual ~Model();

	void addVertex(const Vertex& vert);

	void addIndex(i32 index);
	void addTriangle(i32 i0, i32 i1, i32 i2);

	void addData(const Vector<Vertex>& vertices, const Vector<i32>& indices);

	void addFromFile(const String& file);
	void addFromFile(VirtualFile* file);

	void calculateNormals(PrimitiveType primitive = PrimitiveType::Triangles);
	void calculateTangents(PrimitiveType primitive = PrimitiveType::Triangles);
	void transformTexCoords(const Mat4& t);

	void flush(i32 voff = 0, i32 ioff = 0);

	void bind(ShaderProgram* shader = nullptr);
	void unbind(ShaderProgram* shader = nullptr);

	bool indexed() const { return m_indexed; }
	bool useVertexArray() const { return m_useVertexArrays; }

	const Vertex& vertex(u32 index) const {
		return m_vertexData[index];
	}

	Vertex& vertex(u32 index) {
		return m_vertexData[index];
	}

	i32 index(u32 index) const { return m_indexData[index]; }
	u32 vertexCount() const { return m_vertexData.size(); }
	u32 indexCount() const { return m_indexData.size(); }

private:
	Vector<Vertex> m_vertexData;
	Vector<i32> m_indexData;
	uptr<VertexFormat> m_format;

	bool m_dynamic, m_indexed, m_useVertexArrays, m_vaoOk;

	GLuint m_vbo, m_ibo, m_vao;
	i32 m_vboSize, m_iboSize;

	void triNormal(i32 i0, i32 i1, i32 i2);
	void triTangent(i32 i0, i32 i1, i32 i2);

	void addAIScene(const aiScene* scene);
};

NS_END

#endif // MESHER_H