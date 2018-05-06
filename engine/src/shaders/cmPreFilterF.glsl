R"(#version 330 core
const float PI = 3.14159265459;

out vec4 fragColor;
in vec3 oFragPos;

uniform samplerCube tCubeMap;
uniform float uRoughness;

float VanDerCorpus(uint n, uint base) {
	float invBase = 1.0 / float(base);
	float denom   = 1.0;
	float result  = 0.0;

	for(uint i = 0u; i < 32u; ++i) {
		if(n > 0u) {
			denom   = mod(float(n), 2.0);
			result += denom * invBase;
			invBase = invBase / 2.0;
			n       = uint(float(n) / 2.0);
		}
	}

	return result;
}

vec2 Hammersley(uint i, uint N) {
	return vec2(float(i) / float(N), VanDerCorpus(i, 2u));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main() {
	vec3 N = normalize(oFragPos);
	vec3 R = N;
	vec3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	float totalWeight = 0.0;
	vec3 prefilteredColor = vec3(0.0);
	for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		vec3 H  = ImportanceSampleGGX(Xi, N, uRoughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if(NdotL > 0.0) {
			prefilteredColor += texture(tCubeMap, L).rgb * NdotL;
			totalWeight      += NdotL;
		}
	}
	prefilteredColor = prefilteredColor / totalWeight;

	fragColor = vec4(prefilteredColor, 1.0);
}
)"