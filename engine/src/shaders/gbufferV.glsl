R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;

#define VERTEX_SHADER_COMMON
#include common

out DATA {
	vec4 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	mat3 tbn;
} VSOut;

uniform mat4 mProjection;
uniform mat4 mView;
uniform mat4 mModel;

void main() {
	vec4 pos = mView * mModel * vec4(vPosition, 1.0);
	gl_Position = mProjection * pos;

	VSOut.position = pos;
	VSOut.uv = vTexCoord;
	VSOut.color = vColor;

	VSOut.normal = normalize((mModel * vec4(vNormal, 0.0)).xyz);
	VSOut.tangent = normalize((mModel * vec4(vTangent, 0.0)).xyz);
	VSOut.tangent = normalize(VSOut.tangent - dot(VSOut.tangent, VSOut.normal) * VSOut.normal);

	vec3 b = cross(VSOut.tangent, VSOut.normal);
    VSOut.tbn = mat3(VSOut.tangent, b, VSOut.normal);
}
)"