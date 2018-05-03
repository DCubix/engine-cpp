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

void main() {
	oMaterial = vec2(0.0);
	if (TexSlotEnabled(MaterialMap)) {
		// R = roughness, G = Metallic, B = Emission
		TextureSlot2D slot = TexSlotGet(MaterialMap);
		vec2 uv = transformUV(slot.opt, FSIn.uv);
		oMaterial = texture(slot.img, uv).rgb;
	}

	vec3 N = FSIn.normal;
	if (TexSlotEnabled(NormalMap)) {
		TextureSlot2D slot = TexSlotGet(NormalMap);
		vec2 uv = transformUV(slot.opt, FSIn.uv);
		N = normalMap(FSIn.tbn, texture(slot.img, uv));
	}

	oNormalDepth.rgb = N;
	oNormalDepth.a = gl_FragCoord.z;
}
)"