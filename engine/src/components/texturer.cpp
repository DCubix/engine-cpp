#include "texturer.h"

NS_BEGIN

Texturer& Texturer::setTexture(u32 index, const Texture& texture) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].texture = texture;
	return *this;
}

Texturer& Texturer::setTextureEnabled(u32 index, bool enabled) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].enabled = enabled;
	return *this;
}

Texturer& Texturer::setTextureType(u32 index, TextureSlotType type) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].type = type;
	return *this;
}

Texturer& Texturer::setTextureUVTransform(u32 index, Vec4 uvt) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].uvTransform = uvt;
	return *this;
}

Texturer& Texturer::setTextureSampler(u32 index, Sampler sampler) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].sampler = sampler;
	return *this;
}

NS_END
