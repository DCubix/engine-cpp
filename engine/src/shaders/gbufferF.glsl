R"(#version 330 core
layout (location = 0) out vec2 oNormals;
layout (location = 1) out vec3 oAlbedo;
layout (location = 2) out vec3 oRME;

#define FRAGMENT_SHADER_COMMON
#include common

in DATA {
	vec3 position;
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
TexSlot2D(HeightMap)

uniform Material material;

uniform vec3 uEye;

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
	vec2 tuv = transformUV(TexSlotGet(HeightMap).opt, texCoords);

	// number of depth layers
	const float numLayers = 32;
	// calculate the size of each layer
	float layerDepth = 1.0 / numLayers;
	// depth of current layer
	float currentLayerDepth = 0.0;
	// the amount to shift the texture coordinates per layer (from vector P)
	vec2 P = viewDir.xy * material.heightScale;
	vec2 deltaTexCoords = P / numLayers;

	// get initial values
	vec2  currentTexCoords     = tuv;
	float currentDepthMapValue = 1.0 - texture(TexSlotGet(HeightMap).img, currentTexCoords).r;

	while (currentLayerDepth < currentDepthMapValue) {
		// shift texture coordinates along direction of P
		currentTexCoords -= deltaTexCoords;
		// get depthmap value at current texture coordinates
		currentDepthMapValue = 1.0 - texture(TexSlotGet(HeightMap).img, currentTexCoords).r;
		// get depth of next layer
		currentLayerDepth += layerDepth;
	}

	// get texture coordinates before collision (reverse operations)
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = (1.0 - texture(TexSlotGet(HeightMap).img, prevTexCoords).r) - currentLayerDepth + layerDepth;

	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}

void main() {
	vec2 iuv = FSIn.uv;

	if (TexSlotEnabled(HeightMap)) {
		mat3 ttbn = transpose(FSIn.tbn);
		vec3 tanViewPos = ttbn * uEye;
		vec3 tanFragPos = ttbn * FSIn.position.xyz;
		vec3 V = normalize(tanViewPos - tanFragPos);
		iuv = parallaxMapping(FSIn.uv, V);
		if (material.discardEdges && (iuv.x > 1.0 || iuv.y > 1.0 || iuv.x < 0.0 || iuv.y < 0.0)) {
			discard;
		}
	}

	if (TexSlotEnabled(NormalMap)) {
		vec2 uv = transformUV(TexSlotGet(NormalMap).opt, iuv);
		oNormals.rg = encodeNormals(normalMap(FSIn.tbn, texture(TexSlotGet(NormalMap).img, uv)));
	} else {
		oNormals.rg = encodeNormals(FSIn.normal);
	}

	oAlbedo = material.baseColor;
	if (TexSlotEnabled(Albedo0)) {
		vec2 uv = transformUV(TexSlotGet(Albedo0).opt, iuv);
		oAlbedo *= texture(TexSlotGet(Albedo0).img, uv).rgb;
	}
	if (TexSlotEnabled(Albedo1)) {
		vec2 uv = transformUV(TexSlotGet(Albedo1).opt, iuv);
		vec4 col = texture(TexSlotGet(Albedo1).img, uv);
		oAlbedo = mix(oAlbedo, col.rgb, col.a);
	}

	oRME = vec3(material.roughness, material.metallic, material.emission);
	if (TexSlotEnabled(RMEMap)) {
		// R = roughness, G = Metallic, B = Emission
		vec2 uv = transformUV(TexSlotGet(RMEMap).opt, iuv);
		oRME *= texture(TexSlotGet(RMEMap).img, uv).rgb;
	}
	oRME = saturate(oRME);

}
)"
