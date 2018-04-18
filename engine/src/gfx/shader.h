#ifndef SHADER_H
#define SHADER_H

#include "api.h"
using namespace api;

#include "../core/types.h"

#include "../math/vec.h"
#include "../math/mat.h"

NS_BEGIN

class Uniform {
	friend class ShaderProgram;

	void set(i32 v);
	void set(float v);
	void set(Vec2 v);
	void set(Vec3 v);
	void set(Vec4 v);
	void set(Mat4 v);

protected:
	i32 location;
};

class ShaderProgram {
public:
	ShaderProgram();
	virtual ~ShaderProgram();

	void add(const String& source, ShaderType type);
	void link();

	void bind();
	void unbind();

	i32 getAttributeLocation(const String& name);
	i32 getUniformLocation(const String& name);

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator =(const ShaderProgram&) = delete;
private:
	GLuint m_program;
	bool m_valid;
	Map<String, i32> m_uniforms, m_attributes;
};

NS_END

#endif // SHADER_H