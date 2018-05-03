#include "material.h"

NS_BEGIN

Material& Material::setTexture(u32 index, const Texture& texture) {
	assert(index >= 0 && index < TextureSlotCount);
	m_textures[index].texture = texture;
	return *this;
}

Material& Material::setTextureEnabled(u32 index, bool enabled) {
	assert(index >= 0 && index < TextureSlotCount);
	m_textures[index].enabled = enabled;
	return *this;
}

Material& Material::setTextureType(u32 index, TextureSlotType type) {
	assert(index >= 0 && index < TextureSlotCount);
	m_textures[index].type = type;
	return *this;
}

Material& Material::setTextureUVTransform(u32 index, Vec4 uvt) {
	assert(index >= 0 && index < TextureSlotCount);
	m_textures[index].uvTransform = uvt;
	return *this;
}

NS_END