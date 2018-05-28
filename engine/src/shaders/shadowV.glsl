R"(#version 330 core
layout (location = 0) in vec3 vPosition;

uniform mat4 mProjection;
uniform mat4 mView;
uniform mat4 mModel;

void main() {
	gl_Position = mProjection * mView * mModel * vec4(vPosition, 1.0);
}
)"