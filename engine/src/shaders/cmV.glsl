R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;

out vec3 oLocal;

uniform mat4 mProjection;
uniform mat4 mView;

void main() {
	oLocal = vPosition;
	mat4 rotView = mat4(mat3(mView)); // remove translation
	vec4 cpos = mProjection * rotView * vec4(oLocal, 1.0);
	gl_Position = cpos.xyww;
}
)"
