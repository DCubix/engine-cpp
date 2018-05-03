#include "material.h"

NS_BEGIN

Material& Material::setTexture(u32 index, const Texture& texture) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].texture = texture;
	return *this;
}

Material& Material::setTextureEnabled(u32 index, bool enabled) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].enabled = enabled;
	return *this;
}

Material& Material::setTextureType(u32 index, TextureSlotType type) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].type = type;
	return *this;
}

Material& Material::setTextureUVTransform(u32 index, Vec4 uvt) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].uvTransform = uvt;
	return *this;
}

Material& Material::setTextureSampler(u32 index, Sampler sampler) {
	assert(index >= 0 && index < TextureSlotCount);
	textures[index].sampler = sampler;
	return *this;
}

NS_END