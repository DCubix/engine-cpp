R"(#version 330 core
layout (location = 0) out float oDepth;

void main() {
	oDepth = gl_FragCoord.z;
})"