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

const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float W = 11.2;

vec3 Uncharted2Tonemap(vec3 x) {
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec4 Uncharted2Tonemap(vec4 x) {
   return vec4(Uncharted2Tonemap(x.rgb), x.a);
}

)"