R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 5) in mat4 vModel;

uniform mat4 mProjection;
uniform mat4 mView;

void main() {
	gl_Position = mProjection * mView * vModel * vec4(vPosition, 1.0);
}
)"