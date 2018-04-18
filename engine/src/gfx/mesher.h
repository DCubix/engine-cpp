#ifndef MESHER_H
#define MESHER_H

#include "shader.h"
#include "../core/types.h"
#include "../math/vec.h"

NS_BEGIN

enum AttributeType {
	AttrFloat = 1,
	AttrVector2,
	AttrVector3,
	AttrVector4
};

struct VertexAttribute {
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
	Map<String, VertexAttribute> m_attributes;
	u32 m_stride;
};

template <typename V>
static void appendVertex(Vector<float>& data, const V& v) {
	data.insert(data.begin(), v.begin(), v.end());
}

template <typename V> struct VertexAppender {
	static void append(Vector<float>& data, const V& v) {
		appendVertex(data, v);
	}
};

class Model {
public:
	Model(bool dynamic = false, bool indexed = true, bool vao = true);
	virtual ~Model();

	template <typename V> void addVertex(const V& vert) {
		VertexAppender<V>::append(m_vertexData, vert);
	}

	void addIndex(i32 index);
	void addTriangle(i32 i0, i32 i1, i32 i2);
	void flush(i32 voff = 0, i32 ioff = 0);

	void bind(ShaderProgram* shader = nullptr);
	void unbind(ShaderProgram* shader = nullptr);

	bool indexed() const { return m_indexed; }
	bool useVertexArray() const { return m_useVertexArrays; }
	VertexFormat* format() { return m_format.get(); }

	template <typename V> V* getVertex(u32 index) {
		u32 off = index * (m_format->stride() / 4);
		return (V*) &m_vertexData[off];
	}

	i32 getIndex(u32 index) const { return m_indexData[index]; }

private:
	Vector<float> m_vertexData;
	Vector<i32> m_indexData;
	uptr<VertexFormat> m_format;

	bool m_dynamic, m_indexed, m_useVertexArrays, m_vaoOk;

	GLuint m_vbo, m_ibo, m_vao;
	i32 m_vboSize, m_iboSize;
};

NS_END

#endif // MESHER_H