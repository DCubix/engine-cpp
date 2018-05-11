#include "api.h"

NS_BEGIN

namespace api {

}

GLObjectList Builder<VertexBuffer>::g_vbos;
GLObjectList Builder<VertexArray>::g_vaos;

VertexBuffer& VertexBuffer::bind() {
	glBindBuffer(m_type, m_id);
	return *this;
}

VertexBuffer& VertexBuffer::bind(api::BufferType type) {
	glBindBuffer(type, m_id);
	m_type = type;
	return *this;
}

VertexBuffer& VertexBuffer::unbind() {
	glBindBuffer(m_type, 0);
	return *this;
}

VertexBuffer& VertexBuffer::addVertexAttrib(u32 index, u32 size, api::DataType type, bool normalized, u32 stride, u32 offset) {
	glEnableVertexAttribArray(index);
	glVertexAttribPointer(index, size, type, normalized, stride, (void*)(offset));
	return *this;
}

VertexBuffer& VertexBuffer::setAttribDivisor(u32 index, u32 divisor) {
	glVertexAttribDivisor(index, divisor);
	return *this;
}

void VertexArray::bind() {
	glBindVertexArray(m_id);
}

void VertexArray::unbind() {
	glBindVertexArray(0);
}

NS_END
