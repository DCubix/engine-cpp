#ifndef SHADER_H
#define SHADER_H

#include "api.h"
using namespace api;

#include "../core/builder.h"
#include "../core/types.h"

#include "../math/vec.h"
#include "../math/mat.h"

NS_BEGIN

#define glUniform(T, ...) glUniform##T(location, __VA_ARGS__)

struct UniformValue {
	enum { VInt = 0, VBool, VFloat, VVec2, VVec3, VVec4, VMat4 } type;
	union {
		int val_i;
		bool val_b;
		float val_f;
		Vec2 val_vec2;
		Vec3 val_vec3;
		Vec4 val_vec4;
		Mat4 val_mat4;
	} value;
};

class Uniform {
	friend class ShaderProgram;
public:
	void set(u64 v) { glUniform(1ui, v); }
	void set(i32 v) { glUniform(1i, v); }
	void set(float v) { glUniform(1f, v); }
	void set(bool v) { glUniform(1i, v ? 1 : 0); }
	void set(Vec2 v) { glUniform(2f, v.x, v.y); }
	void set(Vec3 v) { glUniform(3f, v.x, v.y, v.z); }
	void set(Vec4 v) { glUniform(4f, v.x, v.y, v.z, v.w); }
	void set(Mat4 v) { glUniform(Matrix4fv, 1, false, glm::value_ptr(v)); }

	void set(const Mat4* v, u32 size) {
		Vector<float> mv; mv.reserve(size * 16);
		for (u32 i = 0; i < size; i++) {
			const float* mval = glm::value_ptr(v[i]);
			mv.insert(mv.begin(), mval, mval + 16);
		}
		glUniform(Matrix4fv, size, false, mv.data());
	}

	void set(const Vector<Mat4>& v) {
		Vector<float> mv; mv.reserve(v.size() * 16);
		for (Mat4 m : v) {
			const float* mval = glm::value_ptr(m);
			mv.insert(mv.begin(), mval, mval + 16);
		}
		glUniform(Matrix4fv, v.size(), false, mv.data());
	}

	GLenum type() const { return m_type; }
	String name() const { return m_name; }

protected:
	i32 location;
	GLenum m_type;
	String m_name;

	Uniform(i32 loc) : location(loc) {}
};

class ShaderProgram {
public:
	ShaderProgram() : m_valid(false), m_program(0) {}
	ShaderProgram(GLuint prog) : m_valid(false), m_program(prog) {}

	ShaderProgram& add(const String& source, ShaderType type);
	void link();

	void bind();
	void unbind();

	i32 getAttributeLocation(const String& name);
	i32 getUniformLocation(const String& name);

	Uniform get(const String& name);
	UniformValue& getValue(const String& name);

	bool has(const String& name);
	Map<String, i32> uniforms() const { return m_uniforms; }

	bool valid() const { return m_valid; }

	GLuint id() const { return m_program; }

	void cacheUniforms();

protected:
	GLuint m_program;
	bool m_valid;
	Map<String, i32> m_uniforms, m_attributes;
	Map<i32, UniformValue> m_uniformValues;
};

template <>
class Builder<ShaderProgram> {
public:
	static ShaderProgram build() {
		g_programs.push_back(GLProgram::create());
		return ShaderProgram(g_programs.back());
	}

	static void clean() {
		for (GLuint prog : g_programs) {
			GLProgram::destroy(prog);
		}
	}

	static void destroy(ShaderProgram obj) {
		if (obj.valid()) {
			GLProgram::destroy(obj.id());
			g_programs.eraseObject(obj.id());
		}
	}
private:
	static GLObjectList g_programs;
};

NS_END

#endif // SHADER_H
