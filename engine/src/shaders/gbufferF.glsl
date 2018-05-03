R"(#version 330 core
layout (location = 0) out vec3 oMaterial;
layout (location = 1) out vec4 oNormalDepth;

#define FRAGMENT_SHADER_COMMON
#include common

in DATA {
	vec4 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	mat3 tbn;
} FSIn;

TexSlot2D(NormalMap)
TexSlot2D(MaterialMap)

uniform Material material;

void main() {
	oMaterial = vec3(material.roughness, material.metallic, material.emission);
	if (TexSlotEnabled(MaterialMap)) {
		// R = roughness, G = Metallic, B = Emission
		vec2 uv = transformUV(TexSlotGet(MaterialMap).opt, FSIn.uv);
		oMaterial *= texture(TexSlotGet(MaterialMap).img, uv).rgb;
	}

	vec3 N = FSIn.normal;
	if (TexSlotEnabled(NormalMap)) {
		vec2 uv = transformUV(TexSlotGet(NormalMap).opt, FSIn.uv);
		N = normalMap(FSIn.tbn, texture(TexSlotGet(NormalMap).img, uv));
	}

	oNormalDepth.rgb = N;
	oNormalDepth.a = gl_FragCoord.z;
}
)"