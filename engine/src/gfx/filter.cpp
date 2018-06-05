#include "filter.h"

NS_BEGIN

static const String POST_FX_VS =
#include "../shaders/lightingV.glsl"
		;

void Filter::addPass(const Pass& fn) {
	m_passes.push_back(fn);
}

void Filter::setSource(const String& frag) {
	if (m_shader.valid()) {
		Builder<ShaderProgram>::destroy(m_shader);
	}
	m_shader = Builder<ShaderProgram>::build()
			 .add(POST_FX_VS, ShaderType::VertexShader)
			 .add(frag, ShaderType::FragmentShader);
	m_shader.link();
	m_shader.cacheUniforms();
}

NS_END
