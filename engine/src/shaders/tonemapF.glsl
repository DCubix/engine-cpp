R"(#version 330 core
out vec4 fragColor;
in vec2 oScreenPosition;

uniform sampler2D tScreen;

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

void main() {
	vec4 color = texture(tScreen, oScreenPosition);
//	color.rgb *= 16.0;

	float ExposureBias = 3.0;
	vec3 curr = Uncharted2Tonemap(ExposureBias * color.rgb);

	vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
	vec3 ncolor = curr * whiteScale;

	vec3 gcorrect = pow(ncolor, vec3(1.0 / 2.2));

	fragColor = vec4(gcorrect, color.a);
}
)"
