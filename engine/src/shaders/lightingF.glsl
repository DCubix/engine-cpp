R"(#version 330 core
#define FRAGMENT_SHADER_COMMON
#include common
#include brdf

out vec4 fragColor;

in vec2 oScreenPosition;

uniform mat4 mProjection;
uniform mat4 mView;

uniform sampler2D tNormals;
uniform sampler2D tAlbedo;
uniform sampler2D tRME;
//uniform sampler2D tDepth;
//uniform sampler2D tBRDFLUT;
uniform samplerCube tIrradiance;
uniform samplerCube tRadiance;

uniform Light uLight;
uniform vec3 uEye;

uniform bool uIBL;
uniform bool uEmit;

const vec3 Fdielectric = vec3(0.08);
const float MAX_REFLECTION_LOD = 7.0;

void main() {
	vec4 rN = texture(tNormals, oScreenPosition);
	vec3 N = decodeNormals(rN);
	float D = rN.b;
	
	vec3 A = texture(tAlbedo, oScreenPosition).rgb;
	vec3 rme = texture(tRME, oScreenPosition).xyz;
	float R = rme.r;
	float M = rme.g;
	float E = rme.b;

	vec3 wP = worldPosition(mProjection, mView, oScreenPosition, D);
	vec3 V = normalize(uEye - wP);

	fragColor = vec4(0.0);
	if (uIBL && !uEmit) {
		float NoV = saturate(dot(N, V));
		vec3 F0 = mix(Fdielectric, A, M);
		vec3 F = F_Schlick(F0, NoV, R);

		vec3 kS = F;
		vec3 kD = 1.0 - kS;
			 kD *= 1.0 - M;

		vec3 Rf = reflect(-V, N);
//		vec2 envBRDF = texture(tBRDFLUT, vec2(NoV, R)).rg;
		vec3 envBRDF = envBRDFApprox(F0, R, NoV);

		vec3 diff = texture(tIrradiance, N).rgb * A;
		vec3 spec = textureLod(tRadiance, Rf, R * MAX_REFLECTION_LOD).rgb * (F * envBRDF.x + envBRDF.y);

		fragColor = vec4(diff * kD + spec, 1.0);
	} else if (!uIBL && uEmit) {
		fragColor = vec4(A * E, 1.0);
	} else {
		Material mat;
		mat.roughness = R;
		mat.metallic = M;
		mat.emission = E;
		mat.baseColor = A;

		if (uLight.intensity > 0.0 && uLight.type != -1) {
			vec3 L = vec3(0.0);
			float att = 1.0;

			vec3 Lp = uLight.position;

			if (uLight.type == 0) {
				L = -uLight.direction;
				att = 1.0;
			} else if (uLight.type == 1) {
				L = Lp - wP;
				float dist = length(L); L = normalize(L);

				att = lightAttenuation(uLight, L, dist);
			} else if (uLight.type == 2) {
				L = Lp - wP;
				float dist = length(L); L = normalize(L);

				att = lightAttenuation(uLight, L, dist);

				const float minSpot = 0.1; // TODO: Don't hardcode this... use min = spot smoothness
				float S = dot(L, normalize(-uLight.direction));
				if (S > uLight.spotCutoff) {
					att *= (1.0 - (1.0 - S) * 1.0 / (1.0 - uLight.spotCutoff));
				} else {
					att = 0.0;
				}
			}

			float NoL = lambert(N, L);
			float fact = NoL * att;
//			vec3 H = normalize(V + L);
//			float NoV = dot(N, V);
//			float NoH = saturate(dot(N, H));
//			float VoH = saturate(dot(V, H));
//			NoL = saturate(NoL);
//			NoV = saturate(abs(NoV) + 1e-5);

//			vec3 F = F_Schlick(F0, VoH, R);
//			float D = D_GGX(R, NoH);
//			float G = G_Schlick(R, NoV, NoL);

//			vec3 kd = 1.0 - F;

//			vec3 diffuse = A * kd;
//			vec3 specular = (D * G * F) / max(Epsilon, 4.0 * NoL * NoV);

//			fragColor = vec4(uLight.color * (diffuse + specular) * fact, 1.0);

			vec3 b = principledBRDF(mat, L, V, N);

			//fragColor = vec4(uLight.color * uLight.intensity * b * fact, 1.0);
			fragColor = vec4(wP, 1.0);
		}
	}
}
)"