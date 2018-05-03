#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "api.h"
#include "texture.h"
#include "../core/builder.h"
#include "../core/logging/log.h"

using namespace api;

NS_BEGIN

struct RenderBuffer {
	GLuint id;
	RenderBuffer() : id(0) {}
	RenderBuffer(GLuint id) : id(id) {}
};

class FrameBuffer {
public:
	FrameBuffer() = default;
	FrameBuffer(GLuint fbo);
	
	FrameBuffer& setSize(u32 width, u32 height);
	FrameBuffer& addColorAttachment(TextureFormat format);
	FrameBuffer& addDepthAttachment();
	FrameBuffer& addStencilAttachment();
	FrameBuffer& addRenderBuffer(TextureFormat storage, Attachment attachment);
	
	void bind(FrameBufferTarget target = FrameBufferTarget::Framebuffer);
	void unbind(bool resetViewport = true);
	
	void setDrawBuffer(u32 index);
	void resetDrawBuffers();
	
	void blit(
		int sx0, int sy0, int sx1, int sy1,
		int dx0, int dy0, int dx1, int dy1,
		ClearBufferMask mask,
		TextureFilter filter
	);
	
	Texture& getColorAttachment(u32 index) { return m_colorAttachments[index]; }
	Texture& getDepthAttachment() { return m_depthAttachment; }
	Texture& getStencilAttachment() { return m_stencilAttachment; }
	
private:
	GLuint m_fbo;
	FrameBufferTarget m_boundTarget;
	u32 m_width, m_height;
	i32 m_previousViewport[4];
	
	Vector<Texture> m_colorAttachments;
	Texture m_depthAttachment, m_stencilAttachment;
	
	RenderBuffer m_renderBuffer;
};

template <>
class Builder<RenderBuffer> {
public:
	static RenderBuffer build() {
		m_renderbuffers.push_back(GLRenderbuffer::create());
		return RenderBuffer(m_renderbuffers.back());
	}
	
	static void clean() {
		for (GLuint rbo : m_renderbuffers) {
			GLRenderbuffer::destroy(rbo);
		}
		m_renderbuffers.clear();
	}
private:
	static Vector<GLuint> m_renderbuffers;
};

template <>
class Builder<FrameBuffer> {
public:
	static FrameBuffer build() {
		m_framebuffers.push_back(GLFramebuffer::create());
		return FrameBuffer(m_framebuffers.back());
	}
	
	static void clean() {
		for (GLuint fbo : m_framebuffers) {
			GLFramebuffer::destroy(fbo);
		}
		m_framebuffers.clear();
	}
private:
	static Vector<GLuint> m_framebuffers;
};

NS_END

#endif /* FRAMEBUFFER_H */