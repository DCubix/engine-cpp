R"(#version 330 core
out vec4 fragColor;

in DATA {
	vec4 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec4 color;
	mat3 tbn;
} FSIn;

#define FRAGMENT_SHADER_COMMON
#include common
#include brdf

uniform sampler2D tLightingDiffuse;
uniform sampler2D tLightingSpecular;
uniform sampler2D tMaterial;

TexSlot2D(Albedo0)
TexSlot2D(Albedo1)
uniform vec4 uAlbedoColor;

uniform vec2 uResolution;

void main() {
	vec2 suv = gl_FragCoord.xy / uResolution;

	vec3 Dl = texture(tLightingDiffuse, suv).rgb;
	vec3 Sl = texture(tLightingSpecular, suv).rgb;

	vec3 N = FSIn.normal;
	vec3 wP = FSIn.position.xyz;

	float E = texture(tMaterial, suv).b;

	vec4 finalColor = vec4(1.0);

	// Apply textures
	if (TexSlotEnabled(Albedo0)) {
		vec2 uv = transformUV(TexSlotGet(Albedo0).opt, FSIn.uv);
		finalColor = texture(TexSlotGet(Albedo0).img, uv);
	}
	if (TexSlotEnabled(Albedo1)) {
		vec2 uv = transformUV(TexSlotGet(Albedo1).opt, FSIn.uv);
		vec4 col = texture(TexSlotGet(Albedo1).img, uv);
		finalColor = mix(finalColor, col, col.a);
	}

	vec4 diffuse = uAlbedoColor * finalColor * vec4(Dl, 1.0);
	vec4 specular = vec4(Sl, 1.0);

	fragColor = (diffuse + specular) + vec4(diffuse.rgb * E, 1.0);
}

)"