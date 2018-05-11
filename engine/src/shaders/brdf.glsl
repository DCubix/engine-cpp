R"(
float sqr(float x) { return x*x; }

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

/// Principled BRDF (https://github.com/wdas/brdf/blob/master/src/brdfs/disney.brdf)
float fresnelSchlick(float u) {
	float m = saturate(1.0 - u);
	float m2 = m * m;
	return m2 * m2 * m; // pow(m, 5)
}

vec3 mon2lin(vec3 x) {
	return vec3(pow(x.r, 2.2), pow(x.g, 2.2), pow(x.b, 2.2));
}

// X is tangent, Y is biTangent
vec3 principledBRDF(Material params, vec3 L, vec3 V, vec3 N) {
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);

	vec3 H = normalize(L + V);
	float NoH = max(dot(N, H), 0.0);
	float LoH = max(dot(L, H), 0.0);

	vec3 Cdlin = mon2lin(params.baseColor);
	vec3 Cspec0 = mix(vec3(0.08), Cdlin, params.metallic);

	float Ds = D_GGX(params.roughness, NoH);
	float FH = fresnelSchlick(LoH);
	vec3 Fs = mix(Cspec0, vec3(1.0), FH);
	float Gs = G_Schlick(params.roughness, NoV, NoL);

	return ((1.0 / PI) * Cdlin)
			* (1.0 - params.metallic)
			+ Gs * Fs * Ds;
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