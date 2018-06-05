#include "shader.h"

#include "../core/logging/log.h"

NS_BEGIN

GLObjectList Builder<ShaderProgram>::g_programs;

static String trim(const String& str) {
	size_t first = str.find_first_not_of(' ');
	if (String::npos == first) {
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

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
			m_uniformValues[loc] = UniformValue();
		} else {
			return -1;
		}
	}
	return m_uniforms[name];
}

Uniform ShaderProgram::get(const String& name) {
	Uniform uni(getUniformLocation(name));
	uni.m_name = name;
	glGetActiveUniform(m_program, uni.location, 0, NULL, NULL, &uni.m_type, NULL);
	return uni;
}

UniformValue& ShaderProgram::getValue(const String& name) {
	return m_uniformValues[getUniformLocation(name)];
}

bool ShaderProgram::has(const String& name) {
	return getUniformLocation(name) != -1;
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
		m_valid = false;
	}

	GLShader::destroy(s);

	return *this;
}

void ShaderProgram::link() {
	if (!m_valid) return;
	glLinkProgram(m_program);
}

void ShaderProgram::cacheUniforms() {
	i32 ucount = 0;
	glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &ucount);
	for (i32 i = 0; i < ucount; i++) {
		char name[256];
		i32 sz = 0;
		glGetActiveUniform(m_program, i, 256, &sz, NULL, NULL, name);
		getUniformLocation(String(name, sz));
	}
}

NS_END

