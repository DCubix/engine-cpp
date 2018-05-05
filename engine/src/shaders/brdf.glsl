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

vec3 F_Schlick(vec3 SpecularColor, float VoH) {
	return SpecularColor + (vec3(1.0) - SpecularColor) * pow(1.0 - VoH, 5.0);
}

)"