#include "shader.h"

#include "../core/logging/log.h"

NS_BEGIN

Vector<GLuint> Builder<ShaderProgram>::g_programs;

void ShaderProgram::bind() {
	if (m_valid) glUseProgram(m_program);
}

void ShaderProgram::unbind() {
	glUseProgram(0);
}

i32 ShaderProgram::getAttributeLocation(const String& name) {
	if (!m_valid) return -1;
	if (m_attributes.find(name) == m_attributes.end()) {
		i32 loc = glGetAttribLocation(m_program, name.c_str());
		if (loc != -1) {
			m_attributes[name] = loc;
		} else {
			return -1;
		}
	}
	return m_attributes[name];
}

i32 ShaderProgram::getUniformLocation(const String& name) {
	if (!m_valid) return -1;
	if (m_uniforms.find(name) == m_uniforms.end()) {
		i32 loc = glGetUniformLocation(m_program, name.c_str());
		if (loc != -1) {
			m_uniforms[name] = loc;
		} else {
			return -1;
		}
	}
	return m_uniforms[name];
}

Uniform ShaderProgram::get(const String& name) {
	return Uniform(getUniformLocation(name));
}

ShaderProgram& ShaderProgram::add(const String& source, ShaderType type) {
	const char *c_str = source.c_str();
	GLuint s = GLShader::create(type);
	glShaderSource(s, 1, &c_str, NULL);
	glCompileShader(s);

	GLint err;
	glGetShaderiv(s, GL_COMPILE_STATUS, &err);

	if (err != GL_FALSE) {
		glAttachShader(m_program, s);
		m_valid = true;
	} else {
//		LogInfo(source);
		m_valid = false;
	}

	GLShader::destroy(s);

	return *this;
}

void ShaderProgram::link() {
	if (!m_valid) return;
	glLinkProgram(m_program);
}

NS_END

