#include "framebuffer.h"

#include <assert.h>

NS_BEGIN

Vector<GLuint> Builder<FrameBuffer>::m_framebuffers;
Vector<GLuint> Builder<RenderBuffer>::m_renderbuffers;

FrameBuffer::FrameBuffer(GLuint fbo)
	:	m_fbo(fbo), m_boundTarget(FrameBufferTarget::Framebuffer),
		m_depthAttachment(0), m_stencilAttachment(0)
{
	if (fbo) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void FrameBuffer::bind(FrameBufferTarget target) {
	m_boundTarget = target;
	glGetIntegerv(GL_VIEWPORT, m_previousViewport);
	glBindFramebuffer(target, m_fbo);
	glViewport(0, 0, m_width, m_height);
}

void FrameBuffer::unbind(bool resetViewport) {
	glBindFramebuffer(m_boundTarget, 0);
	if (resetViewport) {
		glViewport(
			m_previousViewport[0],
			m_previousViewport[1],
			m_previousViewport[2],
			m_previousViewport[3]
		);
	}
}

FrameBuffer& FrameBuffer::setSize(u32 width, u32 height) {
	m_width = width;
	m_height = height;
	return *this;
}

FrameBuffer& FrameBuffer::addColorAttachment(TextureFormat format) {
	assert(m_width > 0);
	assert(m_height > 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	
	Texture tex = Builder<Texture>::build()
			.bind(TextureTarget::Texture2D)
			.setNull(m_width, m_height, format);
	
	Vector<GLenum> db;
	i32 att = m_colorAttachments.size();
	for (int i = 0; i < att + 1; i++) {
		db.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	
	glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + att,
			GL_TEXTURE_2D,
			tex.id(),
			0
	);
	
	glDrawBuffers(db.size(), db.data());
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return *this;
	}

	m_colorAttachments.push_back(tex);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	return *this;
}

FrameBuffer& FrameBuffer::addDepthAttachment() {
	assert(m_width > 0);
	assert(m_height > 0);
	
	if (m_depthAttachment.id() != 0) {
		LogError("Framebuffer already has a Depth Attachment.");
		return *this;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	
	Texture tex = Builder<Texture>::build()
			.bind(TextureTarget::Texture2D)
			.setNull(m_width, m_height, TextureFormat::Depth);
	
	glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D,
			tex.id(),
			0
	);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	m_depthAttachment = tex;
	
	return *this;
}

FrameBuffer& FrameBuffer::addStencilAttachment() {
	assert(m_width > 0);
	assert(m_height > 0);
	
	if (m_stencilAttachment.id() != 0) {
		LogError("Framebuffer already has a Stencil Attachment.");
		return *this;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	
	Texture tex = Builder<Texture>::build()
			.bind(TextureTarget::Texture2D)
			.setNull(m_width, m_height, TextureFormat::Rf);
	
	glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_STENCIL_ATTACHMENT,
			GL_TEXTURE_2D,
			tex.id(),
			0
	);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	m_stencilAttachment = tex;
	
	return *this;
}

FrameBuffer& FrameBuffer::addRenderBuffer(TextureFormat storage, Attachment attachment) {
	// TODO: More than 1 renderbuffer per framebuffer...
	if (m_renderBuffer.id != 0) {
		LogError("Framebuffer already has a Renderbuffer.");
		return *this;
	}
	auto stor = getTextureFormat(storage);
	m_renderBuffer = Builder<RenderBuffer>::build();
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffer.id);
	glRenderbufferStorage(GL_RENDERBUFFER, std::get<0>(stor), m_width, m_height);
	glFramebufferRenderbuffer(
			GL_FRAMEBUFFER,
			attachment,
			GL_RENDERBUFFER,
			m_renderBuffer.id
	);
	
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return *this;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return *this;
}

FrameBuffer& FrameBuffer::setRenderBufferStorage(TextureFormat storage, u32 w, u32 h) {
	if (m_renderBuffer.id == 0) {
		LogError("Framebuffer has no Renderbuffer.");
		return *this;
	}
	auto stor = getTextureFormat(storage);
	glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffer.id);
	glRenderbufferStorage(GL_RENDERBUFFER, std::get<0>(stor), w == 0 ? m_width : w, h == 0 ? m_height : h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	return *this;
}

void FrameBuffer::setDrawBuffer(u32 index) {
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + index);
}

void FrameBuffer::resetDrawBuffers() {
	Vector<GLenum> db;
	i32 att = m_colorAttachments.size();
	for (int i = 0; i < att; i++) {
		db.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers(db.size(), db.data());
}

void FrameBuffer::blit(
	int sx0, int sy0, int sx1, int sy1,
	int dx0, int dy0, int dx1, int dy1,
	ClearBufferMask mask,
	TextureFilter filter)
{
	glBlitFramebuffer(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, mask, filter);
}

FrameBuffer& FrameBuffer::setColorAttachment(u32 attachment, TextureTarget target, const Texture& tex) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, target, tex.id(), 0);
}

NS_END