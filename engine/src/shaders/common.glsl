R"(const float PI = 3.141592654;
const float Epsilon = 0.00001;

float saturate(float x) {
	return max(min(x, 1.0), 0.0);
}

vec2 saturate(vec2 x) {
	return vec2(saturate(x.x), saturate(x.y));
}

vec3 saturate(vec3 x) {
	return vec3(saturate(x.xy), saturate(x.z));
}

vec4 saturate(vec4 x) {
	return vec4(saturate(x.xyz), saturate(x.w));
}

#ifdef VERTEX_SHADER_COMMON

#endif

#ifdef FRAGMENT_SHADER_COMMON
struct TextureSlotOptions {
	bool enabled;
	vec4 uv_transform; // xy = position, zw = scale
};

struct TextureSlot2D {
	TextureSlotOptions opt;
	sampler2D img;
};

struct TextureSlotCube {
	TextureSlotOptions opt;
	samplerCube img;
};

vec2 transformUV(TextureSlotOptions opt, vec2 uv) {
	return opt.uv_transform.xy + uv * opt.uv_transform.zw;
}

struct Material {
	float roughness;
	float metallic;
	float emission;
	vec3 albedo;
};

struct Light {
	vec3 color;
	float intensity;
	
	float radius;

	vec3 position;
	vec3 direction;

	float lightCutoff;
	float spotCutoff;

	int type;
};

#define TexSlot2D(name) uniform TextureSlot2D t##name;
#define TexSlotCube(name) uniform TextureSlotCube t##name;

#define TexSlotEnabled(name) t##name.opt.enabled
#define TexSlotGet(name) t##name

vec3 normalMap(mat3 tbn, vec3 N) {
	N.y = 1.0 - N.y;
	return normalize(tbn * (N * 2.0 - 1.0));
}

vec3 normalMap(mat3 tbn, vec4 N) {
	return normalMap(tbn, N.xyz);
}

float lambert(vec3 n, vec3 l) {
	return saturate(dot(n, l));
}

vec3 worldPosition(mat4 projection, mat4 view, vec2 uv, float z) {
    vec4 wp = inverse(projection * view) * vec4(uv * 2.0 - 1.0, z, 1.0);
    return (wp.xyz / wp.w);
}

float lightAttenuation(Light light, vec3 L, float dist) {
	float r = light.radius;
	float d = max(dist - r, 0.0);

	// calculate basic attenuation
	float denom = d / r + 1.0;
	float attenuation = 1.0 / (denom * denom);

	// scale and bias attenuation such that:
	//   attenuation == 0 at extent of max influence
	//   attenuation == 1 when d == 0
	attenuation = (attenuation - light.lightCutoff) / (1.0 - light.lightCutoff);
	attenuation = max(attenuation, 0.0);

	return attenuation;
}

vec2 encodeNormals(vec3 n) {
	float scale = 1.7777;
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;
	return enc;
}

vec3 decodeNormals(vec2 enc) {
	float scale = 1.7777;
	vec3 nn = vec3(enc, 0.0) * vec3(2.0 * scale, 2.0 * scale, 0.0) + vec3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);
	vec3 n;
	n.xy = g * nn.xy;
	n.z = g - 1.0;
	return n;
}

#endif

)"