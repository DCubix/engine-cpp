#include "texture.h"

#include "stb/stb_image.h"
#include "../core/filesys.h"

NS_BEGIN

Vector<GLuint> Builder<Texture>::g_textures;
Vector<GLuint> Builder<Sampler>::g_samplers;

Sampler& Sampler::setWrap(TextureWrap s, TextureWrap t, TextureWrap r) {
	glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, s);
	glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, t);
	if (r != TextureWrap::None)
		glSamplerParameteri(m_id, GL_TEXTURE_WRAP_R, r);
	return *this;
}

Sampler& Sampler::setFilter(TextureFilter min, TextureFilter mag) {
	glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, min);
	glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, mag);
	return *this;
}

Sampler& Sampler::setSeamlessCubemap(bool enable) {
	glSamplerParameteri(m_id, GL_TEXTURE_CUBE_MAP_SEAMLESS, enable ? 1 : 0);
	return *this;
}

static ImageData loadImage(const String& file) {
	VFS::get().openRead(file);
	fsize fileSize;
	u8* fileData = VFS::get().read(&fileSize);
	VFS::get().close();
	
	ImageData im;
	im.comp = 0;
	im.w = 0;
	im.h = 0;
	im.data = NULL;
	im.fdata = NULL;
	
	if (fileData) {
		int w, h, comp;
		im.hdr = stbi_is_hdr_from_memory(fileData, fileSize);
		if (!im.hdr) {
			im.data = stbi_load_from_memory(fileData, fileSize, &w, &h, &comp, 0);
		} else {
			im.fdata = stbi_loadf_from_memory(fileData, fileSize, &w, &h, &comp, 0);
		}
		im.w = w;
		im.h = h;
		im.comp = comp;
	}
	return im;
}

Sampler Texture::DEFAULT_SAMPLER;

Texture& Texture::setFromFile(const String& file, TextureTarget tgt) {
	ImageData im = loadImage(file);
	setFromData(im, tgt);
	return *this;
}

Texture& Texture::setFromFile(const String& file) {
	setFromFile(file, m_target);
	return *this;
}

Texture& Texture::setNull(int w, int h, TextureFormat format) {
	auto tfmt = getTextureFormat(format);
	glTexImage2D(m_target, 0, std::get<0>(tfmt), w, h, 0, std::get<1>(tfmt), std::get<2>(tfmt), NULL);
	m_width = w;
	m_height = h;
	return *this;
}

void Texture::setFromData(const ImageData& data, TextureTarget tgt) {
	GLint ifmt;
	GLenum fmt, type;
	
	switch (data.comp) {
		case 1: {
			if (data.hdr) {
				ifmt = GL_R16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_R8;
				type = GL_UNSIGNED_BYTE;
			}
			ifmt = GL_RED;
		} break;
		case 2: {
			if (data.hdr) {
				ifmt = GL_RG16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_RG8;
				type = GL_UNSIGNED_BYTE;
			}
			fmt = GL_RG;
		} break;
		case 3: {
			if (data.hdr) {
				ifmt = GL_RGB16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_RGB8;
				type = GL_UNSIGNED_BYTE;
			}
			fmt = GL_RGB;
		} break;
		case 4: {
			if (data.hdr) {
				ifmt = GL_RGBA16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_RGBA8;
				type = GL_UNSIGNED_BYTE;
			}
			fmt = GL_RGBA;
		} break;
		default: {
			if (data.hdr) free(data.fdata);
			else free(data.data);
			return;
		}
	}
	
	if (data.hdr) {
		glTexImage2D(tgt, 0, ifmt, data.w, data.h, 0, fmt, type, data.fdata);
		free(data.fdata);
	} else {
		glTexImage2D(tgt, 0, ifmt, data.w, data.h, 0, fmt, type, data.data);
		free(data.data);
	}
	
	m_width = data.w;
	m_height = data.h;
}

static ImageData subImage(const ImageData& data, int x, int y, int w, int h) {
	ImageData ndata;
	ndata.w = w;
	ndata.h = h;
	ndata.comp = data.comp;
	ndata.hdr = data.hdr;
	
	if (ndata.hdr) {
		ndata.fdata = new float[w * h * ndata.comp];
	} else {
		ndata.data = new u8[w * h * ndata.comp];
	}
	
	for (u32 iy = y; iy < y + h; iy++) {
		for (u32 ix = x; ix < x + w; ix++) {
			u32 i = (ix + iy * data.w) * data.comp;
			u32 j = ((ix-x) + (iy-y) * w) * ndata.comp; // ndata
			for (u32 c = 0; c < ndata.comp; c++) {
				if (ndata.hdr) {
					ndata.fdata[j + c] = data.fdata[i + c];
				} else {
					ndata.data[j + c] = data.data[i + c];
				}
			}
		}
	}
	
	return ndata;
}

enum CubeMapFace {
	CXP = 0,
	CXN,
	CYP,
	CYN,
	CZP,
	CZN
};

static ImageData cubemapSub(const ImageData& data, CubeMapFace face) {
	int w = data.w / 4;
	int h = data.h / 3;
	int x, y, i;
	switch (face) {
		case CubeMapFace::CXP: i = 6; break;
		case CubeMapFace::CXN: i = 4; break;
		case CubeMapFace::CYP: i = 1; break;
		case CubeMapFace::CYN: i = 9; break;
		case CubeMapFace::CZP: i = 5; break;
		case CubeMapFace::CZN: i = 7; break;
	}
	x = (i % 4) * w;
	y = (i / 4) * w;
	return subImage(data, x, y, w, h);
}

Texture& Texture::setCubemap(const String& file) {
	ImageData im = loadImage(file);
	setFromData(cubemapSub(im, CubeMapFace::CXP), TextureTarget::CubeMapPX);
	setFromData(cubemapSub(im, CubeMapFace::CXN), TextureTarget::CubeMapNX);
	setFromData(cubemapSub(im, CubeMapFace::CYP), TextureTarget::CubeMapPY);
	setFromData(cubemapSub(im, CubeMapFace::CYN), TextureTarget::CubeMapNY);
	setFromData(cubemapSub(im, CubeMapFace::CZP), TextureTarget::CubeMapPZ);
	setFromData(cubemapSub(im, CubeMapFace::CZN), TextureTarget::CubeMapNZ);
	if (im.hdr) free(im.fdata);
	else free(im.data);
	return *this;
}

Texture& Texture::generateMipmaps() {
	glGenerateMipmap(m_target);
	return *this;
}

Texture& Texture::bind(TextureTarget target) {
	m_target = target;
	glBindTexture(m_target, m_id);
	return *this;
}

void Texture::bind(const Sampler& sampler, u32 slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(m_target, m_id);
	sampler.bind(slot);
}

void Texture::unbind() {
	glBindTexture(m_target, 0);
}

NS_END