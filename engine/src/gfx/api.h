#ifndef API_H
#define API_H

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
	DEF_GL_TYPE_TRAIT(Buffer, glGenBuffers, glDeleteBuffers);
	DEF_GL_TYPE_TRAIT(Framebuffer, glGenFramebuffers, glDeleteFramebuffers);
	DEF_GL_TYPE_TRAIT(VertexArray, glGenVertexArrays, glDeleteVertexArrays);
	
	DEF_GL_TYPE_TRAIT_R(Shader, { return glCreateShader(param); }, { glDeleteShader(v); });
	DEF_GL_TYPE_TRAIT_R(Program, { return glCreateProgram(); }, { glDeleteProgram(v); });

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

	enum FramebufferTarget {
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

	static i32 getDataTypeSize(DataType dt);
}

NS_END

#endif // API_H