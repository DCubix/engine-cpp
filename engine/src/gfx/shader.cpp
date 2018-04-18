#include "shader.h"

NS_BEGIN

ShaderProgram::ShaderProgram() {
	m_program = GLProgram::create();
	m_valid = false;
}

ShaderProgram::~ShaderProgram() {
	if (m_program) {
		GLProgram::destroy(m_program);
	}
}

void ShaderProgram::add(const String& source, ShaderType type) {
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
		m_valid = false;
	}

	GLShader::destroy(s);
}

void ShaderProgram::link() {
	if (m_valid) {
		glLinkProgram(m_program);
	}
}

void ShaderProgram::bind() {
	glUseProgram(m_program);
}

void ShaderProgram::unbind() {
	glUseProgram(0);
}

i32 ShaderProgram::getAttributeLocation(const String& name) {
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

NS_END

