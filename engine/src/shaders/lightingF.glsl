R"(#version 330 core
#define FRAGMENT_SHADER_COMMON
#include common
#include brdf

layout (location = 0) out vec3 oDiffuse;
layout (location = 1) out vec3 oSpecular;

in vec2 oScreenPosition;

uniform mat4 mProjection;
uniform mat4 mView;

uniform sampler2D tNormalDepth;
uniform sampler2D tMaterial;

uniform Light uLight;
uniform vec3 uEye;

const vec3 Fdielectric = vec3(0.05);

void main() {
	vec4 nd = texture(tNormalDepth, oScreenPosition);
	vec3 mat = texture(tMaterial, oScreenPosition).xyz;

	vec3 N = nd.xyz;
	float D = nd.w;
	
	float R = mat.r;
	float M = mat.g;

	vec3 wP = worldPosition(mProjection, mView, oScreenPosition, D);
	vec3 V = normalize(uEye - wP);
	vec3 F0 = mix(Fdielectric, vec3(1.0), M);

	if (uLight.intensity > 0.0 && uLight.type != -1) {
		vec3 L = vec3(0.0);
		float att = 1.0;
		

		if (uLight.type == 0) {
			L = -uLight.direction;
			att = 1.0;
		} else if (uLight.type == 1) {
			L = uLight.position - wP;
			float dist = length(L); L = normalize(L);

			att = lightAttenuation(uLight, L, dist);
		} else if (uLight.type == 2) {
			L = uLight.position - wP;
			float dist = length(L); L = normalize(L);

			att = lightAttenuation(uLight, L, dist);

			const float minSpot = 0.1; // TODO: Don't hardcode this... use min = spot smoothness
			float S = dot(L, -uLight.direction);
			att *= smoothstep(uLight.spotCutoff, uLight.spotCutoff - minSpot, S);
		}

		vec3 H = normalize(V + L);
		float NoL = dot(N, L);
		float NoV = dot(N, V);
		float NoH = saturate(dot(N, H));
		float VoH = saturate(dot(V, H));
		NoL = saturate(NoL);
		NoV = saturate(abs(NoV) + 1e-5);

		float fact = NoL * att * uLight.intensity;

		vec3 F = F_Schlick(F0, VoH);
		float D = D_GGX(R, NoH);
		float G = G_Schlick(R, NoV, NoL);

		vec3 kd = mix(vec3(1.0) - F, vec3(0.0), M);

		oDiffuse = uLight.color * kd * fact;
		oSpecular = uLight.color * ((D * G * F) / max(Epsilon, 4.0 * NoL * NoV)) * fact;
	} else {
		oDiffuse = vec3(0.0);
		oSpecular = vec3(0.0);
	}
}
)"