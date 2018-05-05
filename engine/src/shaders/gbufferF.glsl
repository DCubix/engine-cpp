R"(#version 330 core
layout (location = 0) out vec2 oNormals;
layout (location = 1) out vec3 oAlbedo;
layout (location = 2) out vec3 oRME;
layout (location = 3) out float oDepth; // Future: Stencil

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

TexSlot2D(Albedo0)
TexSlot2D(Albedo1)
TexSlot2D(NormalMap)
TexSlot2D(RMEMap)

uniform Material material;

void main() {
	if (TexSlotEnabled(NormalMap)) {
		vec2 uv = transformUV(TexSlotGet(NormalMap).opt, FSIn.uv);
		oNormals = encodeNormals(normalMap(FSIn.tbn, texture(TexSlotGet(NormalMap).img, uv)));
	} else {
		oNormals = encodeNormals(FSIn.normal);
	}

	oAlbedo = material.albedo;
	if (TexSlotEnabled(Albedo0)) {
		vec2 uv = transformUV(TexSlotGet(Albedo0).opt, FSIn.uv);
		oAlbedo *= texture(TexSlotGet(Albedo0).img, uv).rgb;
	}
	if (TexSlotEnabled(Albedo1)) {
		vec2 uv = transformUV(TexSlotGet(Albedo1).opt, FSIn.uv);
		vec4 col = texture(TexSlotGet(Albedo1).img, uv);
		oAlbedo = mix(oAlbedo, col.rgb, col.a);
	}

	oRME = vec3(material.roughness, material.metallic, material.emission);
	if (TexSlotEnabled(RMEMap)) {
		// R = roughness, G = Metallic, B = Emission
		vec2 uv = transformUV(TexSlotGet(RMEMap).opt, FSIn.uv);
		oRME *= texture(TexSlotGet(RMEMap).img, uv).rgb;
	}

	oDepth = gl_FragCoord.z;
}
)"