R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;
layout (location = 5) in mat4 vModel;

out DATA {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	mat3 tbn;
} VSOut;

uniform mat4 mProjection;
uniform mat4 mView;

void main() {
	vec4 pos = vModel * vec4(vPosition, 1.0);
	gl_Position = mProjection * mView * pos;

	VSOut.position = pos.xyz;
	VSOut.uv = vTexCoord;
	VSOut.color = vColor;

	VSOut.normal = normalize((vModel * vec4(vNormal, 0.0)).xyz);
	VSOut.tangent = normalize((vModel * vec4(vTangent, 0.0)).xyz);
	VSOut.tangent = normalize(VSOut.tangent - dot(VSOut.tangent, VSOut.normal) * VSOut.normal);

	vec3 b = cross(VSOut.tangent, VSOut.normal);
    VSOut.tbn = mat3(VSOut.tangent, b, VSOut.normal);
}
)"