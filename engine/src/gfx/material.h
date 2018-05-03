#ifndef MATERIAL_H
#define MATERIAL_H

#include "../math/vec.h"
#include "../core/types.h"
#include "texture.h"

NS_BEGIN

enum TextureSlotType {
	Albedo0 = 0,
	Albedo1,
	NormalMap,
	RougnessMetallicEmission,
	TextureSlotCount
};

struct TextureSlot {
	bool enabled;
	Vec4 uvTransform;
	Texture texture;
	TextureSlotType type;
	Sampler sampler;
	
	TextureSlot()
		: enabled(false), uvTransform(Vec4(0, 0, 1, 1)), type(TextureSlotType::Albedo0),
		sampler(Texture::DEFAULT_SAMPLER)
	{}
};

class Material {
public:
	Material() 
		: roughness(0.01f), metallic(0.0f), emission(0.0f), albedo(Vec4(1.0f))
	{}
	
	float roughness, metallic, emission;
	Vec4 albedo;
	
	Material& setTextureEnabled(u32 index, bool enabled);
	Material& setTextureUVTransform(u32 index, Vec4 uvt);
	Material& setTexture(u32 index, const Texture& texture);
	Material& setTextureType(u32 index, TextureSlotType type);
	Material& setTextureSampler(u32 index, Sampler sampler);
	
	Array<TextureSlot, TextureSlotCount> textures;
};

NS_END

#endif /* MATERIAL_H */

