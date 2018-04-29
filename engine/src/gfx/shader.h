#ifndef SHADER_H
#define SHADER_H

#include "api.h"
using namespace api;

#include "../core/types.h"

#include "../math/vec.h"
#include "../math/mat.h"

NS_BEGIN

#define glUniform(T, ...) glUniform##T(location, __VA_ARGS__)

class Uniform {
	friend class ShaderProgram;
public:
	void set(i32 v) { glUniform(1i, v); }
	void set(float v) { glUniform(1f, v); }
	void set(Vec2 v) { glUniform(2f, v.x, v.y); }
	void set(Vec3 v) { glUniform(3f, v.x, v.y, v.z); }
	void set(Vec4 v) { glUniform(4f, v.x, v.y, v.z, v.w); }
	void set(Mat4 v) { glUniform(Matrix4fv, 1, false, v.val); }

protected:
	i32 location;
	Uniform(i32 loc) : location(loc) {}
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

	opt<Uniform> get(const String& name);

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator =(const ShaderProgram&) = delete;
private:
	GLuint m_program;
	bool m_valid;
	Map<String, i32> m_uniforms, m_attributes;
};

NS_END

#endif // SHADER_H