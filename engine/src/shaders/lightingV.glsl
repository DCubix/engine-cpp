R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;

out vec2 oScreenPosition;

void main() {
	gl_Position = vec4(vPosition * 2.0 - 1.0, 1.0);
	oScreenPosition = vPosition.xy;
}
)"