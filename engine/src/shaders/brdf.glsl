R"(
float D_GGX(float Roughness, float NoH) {
	float alpha   = Roughness * Roughness;
	float alphaSq = alpha * alpha;

	float denom = (NoH * NoH) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

float G_SchlickG1(float cosTheta, float k) {
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

float G_Schlick(float Roughness, float NoV, float NoL) {
	float r = Roughness + 1.0;
	float k = (r * r) / 8.0;
	return G_SchlickG1(NoL, k) * G_SchlickG1(NoV, k);
}

vec3 F_Schlick(vec3 F0, float VoH, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - VoH, 5.0);
}

float mipFromRoughness(float roughness) {
	return (roughness * 5.0 - pow(roughness, 6.0) * 1.5);
}

vec3 specularDominantDir(vec3 normal, vec3 reflection, float roughness) {
	float smoothness = 1.0 - roughness;
	float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
	return mix(normal, reflection, lerpFactor);
}

vec3 envBRDFApprox(vec3 SpecularColor, float Roughness, float NoV) {
	vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
	vec4 c1 = vec4(1, 0.0425, 1.0, -0.04);
	vec4 r = Roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	return SpecularColor * AB.x + AB.y;
}

)"