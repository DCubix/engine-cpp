#include "mesher.h"

NS_BEGIN

void VertexFormat::put(const String& name, AttributeType type, bool normalized, i32 location) {
	VertexAttribute attr;
	attr.size = type;
	attr.normalized = normalized;
	attr.location = location;
	m_attributes[name] = attr;

	m_stride += u32(type) * 4;
}

void VertexFormat::bind(ShaderProgram* shader) {
	u32 off = 0;
	bool shaderValid = shader != nullptr;
	for (auto e : m_attributes) {
		VertexAttribute attr = e.second;
		i32 loc = attr.location;
		if (shaderValid && attr.location == -1) {
			loc = shader->getAttributeLocation(e.first);
		}
		if (loc != -1) {
			glEnableVertexAttribArray(loc);
			glVertexAttribPointer(loc, attr.size, GL_FLOAT, attr.normalized, m_stride, ((void*) off));
		}
		off += attr.size * 4;
	}
}

void VertexFormat::unbind(ShaderProgram* shader) {
	if (shader == nullptr) return;
	for (auto e : m_attributes) {
		VertexAttribute attr = e.second;
		i32 loc = attr.location;
		if (attr.location == -1) {
			loc = shader->getAttributeLocation(e.first);
		}
		if (loc != -1) {
			glDisableVertexAttribArray(loc);
		}
	}
}

Model::~Model() {
	if (m_vbo) GLBuffer::destroy(m_vbo);
	if (m_ibo && m_indexed) GLBuffer::destroy(m_ibo);
	if (m_vao && m_useVertexArrays) GLVertexArray::destroy(m_vao);
}

Model::Model(bool dynamic, bool indexed, bool vao)
	: m_dynamic(dynamic), m_indexed(indexed), m_useVertexArrays(vao),
	m_vboSize(0), m_iboSize(0), m_vaoOk(false) {
	m_format = uptr<VertexFormat>(new VertexFormat());
	m_vbo = GLBuffer::create();
	if (indexed) m_ibo = GLBuffer::create();
	if (vao) m_vao = GLVertexArray::create();
}

void Model::addIndex(i32 index) {
	m_indexData.push_back(index);
}

void Model::addTriangle(i32 i0, i32 i1, i32 i2) {
	m_indexData.push_back(i0);
	m_indexData.push_back(i1);
	m_indexData.push_back(i2);
}

void Model::flush(i32 voff, i32 ioff) {
	if (m_vertexData.empty()) return;

	if (!m_vaoOk && m_useVertexArrays) {
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

		m_format->bind();

		if (m_indexed) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
		glBindVertexArray(0);

		m_vaoOk = true;
	}

	i32 vsize = m_format->stride() * m_vertexData.size();

	BufferUsage usage = m_dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	if (vsize > m_vboSize) {
		if (!m_dynamic) {
			glBufferData(GL_ARRAY_BUFFER, vsize, m_vertexData.data(), usage);
		} else {
			glBufferData(GL_ARRAY_BUFFER, vsize, nullptr, usage);
		}
		m_vboSize = vsize;
	}
	if (m_dynamic) glBufferSubData(GL_ARRAY_BUFFER, voff, vsize, m_vertexData.data());

	if (m_indexed) {
		i32 isize = 4 * m_indexData.size();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
		if (isize > m_iboSize) {
			if (!m_dynamic) {
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, isize, m_indexData.data(), usage);
			} else {
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, isize, nullptr, usage);
			}
			m_iboSize = isize;
		}
		if (m_dynamic) glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, ioff, isize, m_indexData.data());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Model::bind(ShaderProgram* shader) {
	if (m_useVertexArrays) {
		glBindVertexArray(m_vao);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		m_format->bind(shader);
		if (m_indexed) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	}
}

void Model::unbind(ShaderProgram* shader) {
	if (m_useVertexArrays) {
		glBindVertexArray(0);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_format->unbind(shader);
		if (m_indexed) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

NS_END
