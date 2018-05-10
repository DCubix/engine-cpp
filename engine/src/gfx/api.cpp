#include "api.h"

NS_BEGIN

namespace api {

}

Vector<GLuint> Builder<VertexBuffer>::g_vbos;

VertexBuffer& VertexBuffer::bind() {
	glBindBuffer(m_type, m_id);
}

VertexBuffer& VertexBuffer::bind(api::BufferType type) {
	glBindBuffer(type, m_id);
	m_type = type;
}

VertexBuffer& VertexBuffer::unbind() {
	glBindBuffer(m_type, 0);
}

NS_END
