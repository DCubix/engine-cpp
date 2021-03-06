R"(#version 330 core
out vec4 fragColor;

in DATA {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	mat3 tbn;
} FSIn;

uniform uint uEID;

void main() {
	uint r = (uEID & uint(0xFF0000)) >> uint(16);
	uint g = (uEID & uint(0x00FF00)) >> uint(8);
	uint b = (uEID & uint(0x0000FF));
	fragColor = vec4(
		float(r) / 255.0,
		float(g) / 255.0,
		float(b) / 255.0,
		1.0
	);
}
)"
