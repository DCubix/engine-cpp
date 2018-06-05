R"(#version 330 core
uniform sampler2D tScreen;
uniform sampler2D tDepth;
uniform vec2 uResolution;
uniform vec2 uNF;

out vec4 fragColor;
in vec2 oScreenPosition;

uniform float VAR_FocalPoint = 6.0;
uniform float VAR_FocalScale = 5.0;

#define PI 3.1415926

const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 12.0;
const float RAD_SCALE = 1.5; // Smaller = nicer blur, larger = faster

float linearize(float depth) {
	float z = 2.0 * depth - 1.0;
	return (2.0 * uNF.x * uNF.y) / (uNF.y + uNF.x - z * (uNF.y - uNF.x));
}

float getBlurSize(float depth, float focusPoint, float focusScale) {
	float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
	return abs(coc) * MAX_BLUR_SIZE;
}

vec3 depthOfField(vec2 texCoord, float focusPoint, float focusScale) {
	float centerDepth = linearize(texture(tDepth, texCoord).r);
	float centerSize = getBlurSize(centerDepth, focusPoint, focusScale);
	vec3 color = texture(tScreen, texCoord).rgb;
	float tot = 1.0;

	vec2 pixSize = vec2(1.0) / uResolution;

	float radius = RAD_SCALE;
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE) {
		vec2 tc = texCoord + vec2(cos(ang), sin(ang)) * pixSize * radius;

		vec3 sampleColor = texture(tScreen, tc, 1.5).rgb;
		float sampleDepth = linearize(texture(tDepth, tc).r);
		float sampleSize = getBlurSize(sampleDepth, focusPoint, focusScale);

		if (sampleDepth > centerDepth)
			sampleSize = clamp(sampleSize, 0.0, centerSize*2.0);

		float m = smoothstep(radius-0.5, radius+0.5, sampleSize);
		color += mix(color/tot, sampleColor, m);
		tot += 1.0;
		radius += RAD_SCALE / radius;
	}
	return color /= tot;
}

void main(void) {
	fragColor = vec4(depthOfField(oScreenPosition, VAR_FocalPoint, VAR_FocalScale), 1.0);
}
)"
