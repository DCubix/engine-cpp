R"(#version 330 core
out vec4 fragColor;
in vec2 oScreenPosition;

uniform sampler2D tTex;

void main() {
	fragColor = texture(tTex, oScreenPosition);
}
)"