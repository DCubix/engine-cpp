#ifndef FILTER_H
#define FILTER_H

#include "shader.h"

NS_BEGIN

class Filter;
using Pass = Fn<void(Filter&)>;
using PassList = Vector<Pass>;

class Filter {
public:
	Filter() = default;

	void addPass(const Pass& fn);
	void setSource(const String& frag);

	PassList passes() const { return m_passes; }
	ShaderProgram& shader() { return m_shader; }

	String name() const { return m_name; }
	void setName(const String& name) { m_name = name; }

private:
	ShaderProgram m_shader;
	PassList m_passes;
	String m_name;
};

NS_END

#endif // FILTER_H
