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

vec4 Uncharted2Tonemap(vec4 x) {
   return vec4(Uncharted2Tonemap(x.rgb), x.a);
}

void main() {
	fragColor = Uncharted2Tonemap(texture(tScreen, oScreenPosition));
}
)"