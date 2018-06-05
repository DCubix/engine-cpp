#ifndef TEXTURER_H
#define TEXTURER_H

#include "../core/ecs.h"

#include "../core/types.h"
#include "../math/vec.h"

#include "../gfx/texture.h"

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
	{
		texture.invalidate();
	}
};

class Texturer : public Component {
public:
	Texturer& setTextureEnabled(u32 index, bool enabled);
	Texturer& setTextureUVTransform(u32 index, Vec4 uvt);
	Texturer& setTexture(u32 index, const Texture& texture);
	Texturer& setTextureType(u32 index, TextureSlotType type);
	Texturer& setTextureSampler(u32 index, Sampler sampler);

	Array<TextureSlot, TextureSlotCount> textures;
};

NS_END

#endif // TEXTURER_H
