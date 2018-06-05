R"(
float sqr(float x) { return x*x; }
vec3 sqr(vec3 x) { return vec3(sqr(x.x), sqr(x.y), sqr(x.z)); }

float Pow5(float x) {
	float xx = x*x;
	return xx * xx * x;
}

vec3 Diffuse_Lambert(vec3 DiffuseColor) {
	return DiffuseColor * (1.0 / PI);
}

vec3 Diffuse_Burley(vec3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH) {
	float FD90 = 0.5 + 2 * VoH * VoH * Roughness;
	float FdV = 1 + (FD90 - 1) * Pow5( 1 - NoV );
	float FdL = 1 + (FD90 - 1) * Pow5( 1 - NoL );
	return DiffuseColor * ( (1 / PI) * FdV * FdL );
}

float D_GGX(float Roughness, float NoH) {
	float alpha   = Roughness * Roughness;
	float alphaSq = alpha * alpha;

	float denom = (NoH * NoH) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

float Vis_SmithJointApprox(float Roughness, float NoV, float NoL) {
	float a = sqr(Roughness);
	float Vis_SmithV = NoL * (NoV * (1.0 - a) + a);
	float Vis_SmithL = NoV * (NoL * (1.0 - a) + a);
	return 0.5 * (1.0 / (Vis_SmithV + Vis_SmithL));
}

float G_SchlickG1(float cosTheta, float k) {
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

float G_Schlick(float Roughness, float NoV, float NoL) {
	float r = Roughness + 1.0;
	float k = (r * r) / 8.0;
	return G_SchlickG1(NoL, k) * G_SchlickG1(NoV, k);
}

vec3 F_Schlick(vec3 SpecularColor, float VoH) {
	float Fc = Pow5(1.0 - VoH);
	return saturate(50.0 * SpecularColor.g) * Fc + (1.0 - Fc) * SpecularColor;
}

vec3 F_Fresnel(vec3 SpecularColor, float VoH) {
	vec3 SpecularColorSqrt = sqrt(clamp(vec3(0, 0, 0), vec3(0.99, 0.99, 0.99), SpecularColor));
	vec3 n = (1.0 + SpecularColorSqrt) / (1.0 - SpecularColorSqrt);
	vec3 g = sqrt(n*n + VoH*VoH - 1.0);
	return 0.5 * sqr((g - VoH) / (g + VoH)) * (1.0 + sqr(((g + VoH) * VoH - 1) / ((g-VoH)*VoH + 1)));
}

vec3 mon2lin(vec3 x) {
	return vec3(pow(x.r, 2.2), pow(x.g, 2.2), pow(x.b, 2.2));
}

vec3 BRDF(Material params, vec3 specColor, vec3 L, vec3 V, vec3 N, float energy) {
	float NoL = dot(N, L);
	float NoV = dot(N, V);
	float LoV = dot(L, V);
	float InvLenH = inversesqrt(2.0 + 2.0 * LoV);
	float NoH = saturate((NoL + NoV) * InvLenH);
	float VoH = saturate(InvLenH + InvLenH * LoV);
	NoL = saturate(NoL);
	NoV = saturate(abs(NoV) + 1e-5);

	// Generalized microfacet specular
	float D = D_GGX(params.roughness, NoH) * energy;
	float Vis = Vis_SmithJointApprox(params.roughness, NoV, NoL);
//	vec3 F = F_Schlick(specColor, VoH);
	vec3 F = F_Fresnel(specColor, VoH);

//	vec3 Diffuse = Diffuse_Lambert(params.baseColor);
	vec3 Diffuse = Diffuse_Burley(mon2lin(params.baseColor), params.roughness, NoV, NoL, VoH);

	return Diffuse * energy + (D * Vis) * F;
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
