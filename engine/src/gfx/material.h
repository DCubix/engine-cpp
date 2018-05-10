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
	HeightMap,
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
		: roughness(0.5f), metallic(0.0f), emission(0.0f), baseColor(Vec3(1.0f)), heightScale(1.0f),
			discardParallaxEdges(false), instanced(false), m_id(++g_matID)
	{}
	
	Vec3 baseColor;
	float metallic;
	float roughness;
	float emission;
	float heightScale;
	bool discardParallaxEdges, instanced;
	
	Material& setTextureEnabled(u32 index, bool enabled);
	Material& setTextureUVTransform(u32 index, Vec4 uvt);
	Material& setTexture(u32 index, const Texture& texture);
	Material& setTextureType(u32 index, TextureSlotType type);
	Material& setTextureSampler(u32 index, Sampler sampler);
	
	Array<TextureSlot, TextureSlotCount> textures;
	
	u32 id() const { return m_id; }
private:
	static u32 g_matID;
	u32 m_id;
};

NS_END

#endif /* MATERIAL_H */

