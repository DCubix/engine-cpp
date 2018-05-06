R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;

out vec3 oFragPos;

uniform mat4 mProjection;
uniform mat4 mView;

void main() {
	oFragPos = vPosition;
	gl_Position = mProjection * mView * vec4(oFragPos, 1.0);
}
)"
