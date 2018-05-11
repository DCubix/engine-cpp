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
uniform sampler2D tDepth;
uniform sampler2D tBRDFLUT;
uniform samplerCube tIrradiance;
uniform samplerCube tRadiance;

uniform sampler2DShadow tShadowMap;
uniform bool uShadowEnabled = false;
uniform mat4 uLightViewProj;
uniform float uLightFrustumSize = 1.0;

uniform Light uLight;

uniform vec3 uEye;
uniform vec2 uNF;

uniform bool uIBL;
uniform bool uEmit;

const vec3 Fdielectric = vec3(0.08);
const float MAX_REFLECTION_LOD = 7.0;
const mat4 mBias = mat4(
	vec4(0.5, 0.0, 0.0, 0.0),
	vec4(0.0, 0.5, 0.0, 0.0),
	vec4(0.0, 0.0, 0.5, 0.0),
	vec4(0.5, 0.5, 0.5, 1.0)
);

const vec2 PoissonDisk[64] = vec2[](
	vec2(-0.613392, 0.617481),
    vec2(0.170019, -0.040254),
    vec2(-0.299417, 0.791925),
    vec2(0.645680, 0.493210),
    vec2(-0.651784, 0.717887),
    vec2(0.421003, 0.027070),
    vec2(-0.817194, -0.271096),
    vec2(-0.705374, -0.668203),
    vec2(0.977050, -0.108615),
    vec2(0.063326, 0.142369),
    vec2(0.203528, 0.214331),
    vec2(-0.667531, 0.326090),
    vec2(-0.098422, -0.295755),
    vec2(-0.885922, 0.215369),
    vec2(0.566637, 0.605213),
    vec2(0.039766, -0.396100),
    vec2(0.751946, 0.453352),
    vec2(0.078707, -0.715323),
    vec2(-0.075838, -0.529344),
    vec2(0.724479, -0.580798),
    vec2(0.222999, -0.215125),
    vec2(-0.467574, -0.405438),
    vec2(-0.248268, -0.814753),
    vec2(0.354411, -0.887570),
    vec2(0.175817, 0.382366),
    vec2(0.487472, -0.063082),
    vec2(-0.084078, 0.898312),
    vec2(0.488876, -0.783441),
    vec2(0.470016, 0.217933),
    vec2(-0.696890, -0.549791),
    vec2(-0.149693, 0.605762),
    vec2(0.034211, 0.979980),
    vec2(0.503098, -0.308878),
    vec2(-0.016205, -0.872921),
    vec2(0.385784, -0.393902),
    vec2(-0.146886, -0.859249),
    vec2(0.643361, 0.164098),
    vec2(0.634388, -0.049471),
    vec2(-0.688894, 0.007843),
    vec2(0.464034, -0.188818),
    vec2(-0.440840, 0.137486),
    vec2(0.364483, 0.511704),
    vec2(0.034028, 0.325968),
    vec2(0.099094, -0.308023),
    vec2(0.693960, -0.366253),
    vec2(0.678884, -0.204688),
    vec2(0.001801, 0.780328),
    vec2(0.145177, -0.898984),
    vec2(0.062655, -0.611866),
    vec2(0.315226, -0.604297),
    vec2(-0.780145, 0.486251),
    vec2(-0.371868, 0.882138),
    vec2(0.200476, 0.494430),
    vec2(-0.494552, -0.711051),
    vec2(0.612476, 0.705252),
    vec2(-0.578845, -0.768792),
    vec2(-0.772454, -0.090976),
    vec2(0.504440, 0.372295),
    vec2(0.155736, 0.065157),
    vec2(0.391522, 0.849605),
    vec2(-0.620106, -0.328104),
    vec2(0.789239, -0.419965),
    vec2(-0.545396, 0.538133),
    vec2(-0.178564, -0.596057)
);

float findBlockerDistance(sampler2DShadow shadowMap, vec3 coord, float lightSize, float bias) {
	int blockers = 0;
	float avgBlockerDistance = 0.0;
	float sw = lightSize * uNF.x / coord.z;

	float fd = coord.z - bias;
	for (int i = 0; i < 64; i++) {
		float z = texture(shadowMap, vec3(coord.xy + PoissonDisk[i % 64] * sw, 0.0));
		if (z < fd) {
			blockers++;
			avgBlockerDistance += z;
		}
	}
	if (blockers < 1) return -1.0;
	return avgBlockerDistance / float(blockers);
}

float PCF(in sampler2DShadow sbuffer, vec3 coord, float bias, float radius) {
	if (coord.z > 1.0) {
		return 0.0;
	}

	float fd = coord.z;
	float sum = 0.0;
	for (int i = 0; i < 64; i++) {
		vec3 uvc = vec3(coord.xy + PoissonDisk[i % 64] * radius, fd);
		float z = texture(sbuffer, uvc);
		sum += z < fd ? 1.0 : 0.0;
	}
	return sum / 64.0;
}

float PCSS(sampler2DShadow shadowMap, vec3 coord, float bias, float lightSize) {
	// blocker search
	float dist = findBlockerDistance(shadowMap, coord, lightSize, bias);
	if (dist <= -1.0) return 1.0;

	// penumbra estimation
	float pr = (coord.z - dist) / dist;

	// percentage-close filtering
	float radius = pr * lightSize * uNF.x / coord.z;
	return 1.0 - PCF(shadowMap, coord, bias, radius);
}

void main() {
	vec3 N = decodeNormals(texture(tNormals, oScreenPosition));
	float D = texture(tDepth, oScreenPosition).r;
	
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
		vec2 envBRDF = texture(tBRDFLUT, vec2(NoV, R)).rg;
		
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
			float vis = 1.0;

			vec3 Lp = uLight.position;

			if (uLight.type == 0) {
				L = -uLight.direction;
				att = 1.0;

				if (uShadowEnabled) {
					vec4 sc = uLightViewProj * vec4(wP, 1.0);
					vec3 coord = (sc.xyz / sc.w) * 0.5 + 0.5;

					float nl = dot(N, L);
					float bias = 0.0001 * tan(acos(nl));
					bias = clamp(bias, 0.0, 0.01);

					vis = PCSS(tShadowMap, coord, bias, uLight.size);
//					vis = PCF(tShadowMap, coord, bias, 0.01);
				}
			} else if (uLight.type == 1) {
				L = Lp - wP;
				float dist = length(L); L = normalize(L);

				att = lightAttenuation(uLight, L, dist);
			} else if (uLight.type == 2) {
				L = Lp - wP;
				float dist = length(L); L = normalize(L);

				att = lightAttenuation(uLight, L, dist);

				float S = dot(L, normalize(-uLight.direction));
				if (S > uLight.spotCutoff) {
					att *= (1.0 - (1.0 - S) * 1.0 / (1.0 - uLight.spotCutoff));
				} else {
					att *= 0.0;
				}
			}

			float NoL = max(dot(N, L), 0.0);
			float fact = NoL * att * vis;

			vec3 b = principledBRDF(mat, L, V, N) * fact;

			fragColor = vec4(uLight.color * uLight.intensity * b, 1.0);
//			fragColor = vec4(wP, 1.0);
		}
	}
}
)"