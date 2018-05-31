#ifndef API_H
#define API_H

#include "../core/builder.h"
#include "../core/types.h"

extern "C" {
	#include "glad/glad.h"
}

NS_BEGIN

namespace api {

#define DEF_GL_TYPE_TRAIT_R(name, gen, del) \
struct GL##name { \
	static GLuint create(GLenum param = GL_NONE) gen \
	static void destroy(GLuint v) del \
}

#define DEF_GL_TYPE_TRAIT(name, gen, del) \
DEF_GL_TYPE_TRAIT_R(name, { GLuint v; gen(1, &v); return v; }, { del(1, &v); })

	DEF_GL_TYPE_TRAIT(Texture, glGenTextures, glDeleteTextures);
	DEF_GL_TYPE_TRAIT(Sampler, glGenSamplers, glDeleteSamplers);
	DEF_GL_TYPE_TRAIT(Buffer, glGenBuffers, glDeleteBuffers);
	DEF_GL_TYPE_TRAIT(Framebuffer, glGenFramebuffers, glDeleteFramebuffers);
	DEF_GL_TYPE_TRAIT(Renderbuffer, glGenRenderbuffers, glDeleteRenderbuffers);
	DEF_GL_TYPE_TRAIT(VertexArray, glGenVertexArrays, glDeleteVertexArrays);

	DEF_GL_TYPE_TRAIT_R(Shader, { return glCreateShader(param); }, { glDeleteShader(v); });
	DEF_GL_TYPE_TRAIT_R(Program, { return glCreateProgram(); }, { glDeleteProgram(v); });

	enum ClearBufferMask {
		ColorBuffer = GL_COLOR_BUFFER_BIT,
		DepthBuffer = GL_DEPTH_BUFFER_BIT,
		StencilBuffer = GL_STENCIL_BUFFER_BIT
	};

	enum PrimitiveType {
		Points = GL_POINTS,
		Lines = GL_LINES,
		LineLoop = GL_LINE_LOOP,
		LineStrip = GL_LINE_STRIP,
		Triangles = GL_TRIANGLES,
		TriangleFan = GL_TRIANGLE_FAN,
		TriangleStrip = GL_TRIANGLE_STRIP
	};

	enum DataType {
		Int = GL_INT,
		Short = GL_SHORT,
		Float = GL_FLOAT,
		Byte = GL_BYTE,
		UInt = GL_UNSIGNED_INT,
		UShort = GL_UNSIGNED_SHORT,
		UByte = GL_UNSIGNED_BYTE
	};

	enum ShaderType {
		VertexShader = GL_VERTEX_SHADER,
		FragmentShader = GL_FRAGMENT_SHADER,
		GeometryShader = GL_GEOMETRY_SHADER
	};

	enum BufferType {
		ArrayBuffer = GL_ARRAY_BUFFER,
		IndexBuffer = GL_ELEMENT_ARRAY_BUFFER,
		UniformBuffer = GL_UNIFORM_BUFFER
	};

	enum BufferUsage {
		Static = GL_STATIC_DRAW,
		Dynamic = GL_DYNAMIC_DRAW,
		Stream = GL_STREAM_DRAW
	};

	enum TextureTarget {
		Texture1D = GL_TEXTURE_1D,
		Texture1DArray = GL_TEXTURE_1D_ARRAY,
		Texture2D = GL_TEXTURE_2D,
		Texture2DArray = GL_TEXTURE_2D_ARRAY,
		Texture3D = GL_TEXTURE_3D,
		CubeMap = GL_TEXTURE_CUBE_MAP,
		CubeMapPX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		CubeMapNX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		CubeMapPY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		CubeMapNY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		CubeMapPZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		CubeMapNZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	enum TextureWrap {
		None = 0,
		Repeat = GL_REPEAT,
		ClampToEdge = GL_CLAMP_TO_EDGE,
		ClampToBorder = GL_CLAMP_TO_BORDER
	};

	enum TextureFilter {
		Nearest = GL_NEAREST,
		Linear = GL_LINEAR,
		NearestMipNearest = GL_NEAREST_MIPMAP_NEAREST,
		NearestMipLinear = GL_NEAREST_MIPMAP_LINEAR,
		LinearMipLinear = GL_LINEAR_MIPMAP_LINEAR,
		LinearMipNearest = GL_LINEAR_MIPMAP_NEAREST
	};

	enum TextureFormat {
		R = 0,
		RG,
		RGB,
		RGBA,
		Rf,
		RGf,
		RGBf,
		RGBAf,
		Depthf,
		DepthStencil
	};

	enum FrameBufferTarget {
		Framebuffer = GL_FRAMEBUFFER,
		DrawFramebuffer = GL_DRAW_FRAMEBUFFER,
		ReadFramebuffer = GL_READ_FRAMEBUFFER
	};

	enum Attachment {
		ColorAttachment = GL_COLOR_ATTACHMENT0,
		DepthAttachment = GL_DEPTH_ATTACHMENT,
		StencilAttachment = GL_STENCIL_ATTACHMENT,
		DepthStencilAttachment = GL_DEPTH_STENCIL_ATTACHMENT
	};

	enum DataAccess {
		ReadOnly = GL_READ_ONLY,
		WriteOnly = GL_WRITE_ONLY,
		ReadWrite = GL_READ_WRITE
	};

	static i32 getDataTypeSize(DataType dt) {
		switch (dt) {
			case DataType::Byte:
			case DataType::UByte:
				return 1;
			case DataType::Float:
			case DataType::Int:
			case DataType::UInt:
				return 4;
			case DataType::Short:
			case DataType::UShort:
				return 2;
		}
		return 0;
	}

	static std::tuple<GLint, GLenum, DataType> getTextureFormat(TextureFormat format) {
		GLenum ifmt;
		GLint fmt;
		DataType type;
		switch (format) {
			case TextureFormat::R: ifmt = GL_R8; fmt = GL_RED; type = DataType::UByte; break;
			case TextureFormat::RG: ifmt = GL_RG8; fmt = GL_RG; type = DataType::UByte; break;
			case TextureFormat::RGB: ifmt = GL_RGB8; fmt = GL_RGB; type = DataType::UByte; break;
			case TextureFormat::RGBA: ifmt = GL_RGBA8; fmt = GL_RGBA; type = DataType::UByte; break;
			case TextureFormat::Rf: ifmt = GL_R32F; fmt = GL_RED; type = DataType::Float; break;
			case TextureFormat::RGf: ifmt = GL_RG32F; fmt = GL_RG; type = DataType::Float; break;
			case TextureFormat::RGBf: ifmt = GL_RGB32F; fmt = GL_RGB; type = DataType::Float; break;
			case TextureFormat::RGBAf: ifmt = GL_RGBA32F; fmt = GL_RGBA; type = DataType::Float; break;
			case TextureFormat::Depthf: ifmt = GL_DEPTH_COMPONENT24; fmt = GL_DEPTH_COMPONENT; type = DataType::Float; break;
			case TextureFormat::DepthStencil: ifmt = GL_DEPTH24_STENCIL8; fmt = GL_DEPTH_STENCIL; type = DataType::Float; break;
		}
		return tup(ifmt, fmt, type);
	}
}

class GLObjectList : public Vector<GLuint> {
public:
	void eraseObject(GLuint obj) {
		erase(std::remove(begin(), end(), obj), end());
	}
};

class VertexBuffer {
public:
	VertexBuffer() : m_id(0), m_size(0) {}
	VertexBuffer(GLuint id) : m_id(id), m_size(0) {}

	VertexBuffer& bind();
	VertexBuffer& bind(api::BufferType type);
	VertexBuffer& unbind();

	template <typename T>
	VertexBuffer& setData(u32 count, T* data, api::BufferUsage usage = api::BufferUsage::Static, u32 offset = 0) {
		if (m_size < count) {
			glBufferData(m_type, sizeof(T) * count, data, usage);
			m_size = count;
			m_usage = usage;
		} else {
			if (m_usage != api::BufferUsage::Static)
				glBufferSubData(m_type, offset, sizeof(T) * count, data);
		}
		return *this;
	}

	VertexBuffer& addVertexAttrib(u32 index, u32 size, api::DataType type, bool normalized, u32 stride, u32 offset);
	VertexBuffer& setAttribDivisor(u32 index, u32 divisor);

	u32 size() const { return m_size; }
	GLuint id() const { return m_id; }

private:
	GLuint m_id;
	api::BufferType m_type;
	api::BufferUsage m_usage;
	u32 m_size;
};

template <>
class Builder<VertexBuffer> {
public:
	static VertexBuffer build() {
		g_vbos.push_back(api::GLBuffer::create());
		return VertexBuffer(g_vbos.back());
	}

	static void clean() {
		for (GLuint b : g_vbos) {
			api::GLBuffer::destroy(b);
		}
	}

	static void destroy(VertexBuffer buf) {
		if (buf.id() != 0) {
			api::GLBuffer::destroy(buf.id());
			g_vbos.eraseObject(buf.id());
		}
	}
private:
	static GLObjectList g_vbos;
};

class VertexArray {
public:
	VertexArray() : m_id(0) {}
	VertexArray(GLuint id) : m_id(id) {}

	void bind();
	void unbind();

	GLuint id() const { return m_id; }

private:
	GLuint m_id;
};

template <>
class Builder<VertexArray> {
public:
	static VertexArray build() {
		g_vaos.push_back(api::GLVertexArray::create());
		return VertexArray(g_vaos.back());
	}

	static void clean() {
		for (GLuint b : g_vaos) {
			api::GLVertexArray::destroy(b);
		}
	}

	static void destroy(VertexArray buf) {
		if (buf.id() != 0) {
			api::GLVertexArray::destroy(buf.id());
			g_vaos.eraseObject(buf.id());
		}
	}
private:
	static GLObjectList g_vaos;
};

NS_END

#endif // API_H
