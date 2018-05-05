R"(#version 330 core
out vec4 fragColor;
in vec3 oLocal;

uniform samplerCube tCubeMap;

void main() {
	vec3 envColor = texture(tCubeMap, oLocal).rgb;
	fragColor = vec4(envColor, 1.0);
}
)"