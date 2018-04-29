#include "texture.h"

#include "stb/stb_image.h"
#include "../core/filesys.h"

NS_BEGIN

Vector<GLuint> Builder<Texture>::g_textures;

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

Texture& Texture::setFromFile(const String& file, TextureTarget tgt) {
	ImageData im = loadImage(file);
	GLint ifmt;
	GLenum fmt, type;
	
	switch (im.comp) {
		case 1: {
			if (im.hdr) {
				ifmt = GL_R16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_R8;
				type = GL_UNSIGNED_BYTE;
			}
			ifmt = GL_RED;
		} break;
		case 2: {
			if (im.hdr) {
				ifmt = GL_RG16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_RG8;
				type = GL_UNSIGNED_BYTE;
			}
			fmt = GL_RG;
		} break;
		case 3: {
			if (im.hdr) {
				ifmt = GL_RGB16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_RGB8;
				type = GL_UNSIGNED_BYTE;
			}
			fmt = GL_RGB;
		} break;
		case 4: {
			if (im.hdr) {
				ifmt = GL_RGBA16F;
				type = GL_FLOAT;
			} else {
				ifmt = GL_RGBA8;
				type = GL_UNSIGNED_BYTE;
			}
			fmt = GL_RGBA;
		} break;
		default: return *this;
	}
	
	if (im.hdr) {
		glTexImage2D(tgt, 0, ifmt, im.w, im.h, 0, fmt, type, im.fdata);
		free(im.fdata);
	} else {
		glTexImage2D(tgt, 0, ifmt, im.w, im.h, 0, fmt, type, im.data);
		free(im.data);
	}
	
	return *this;
}

Texture& Texture::setFromFile(const String& file) {
	setFromFile(file, m_target);
	return *this;
}

Texture& Texture::setNull(int w, int h, GLint ifmt, GLenum fmt, DataType data) {
	glTexImage2D(m_target, 0, ifmt, w, h, 0, fmt, data, NULL);
	return *this;
}

Texture& Texture::setWrap(TextureWrap s, TextureWrap t, TextureWrap r) {
	glTexParameteri(m_target, GL_TEXTURE_WRAP_S, s);
	glTexParameteri(m_target, GL_TEXTURE_WRAP_T, t);
	if (r != TextureWrap::None)
		glTexParameteri(m_target, GL_TEXTURE_WRAP_R, r);
	return *this;
}

Texture& Texture::setFilter(TextureFilter min, TextureFilter mag) {
	glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, mag);
	if (min == TextureFilter::LinearMipLinear ||
		min == TextureFilter::LinearMipNearest ||
		min == TextureFilter::NearestMipLinear ||
		min == TextureFilter::NearestMipNearest) {
		glGenerateMipmap(m_target);
	}
	return *this;
}

Texture& Texture::generateMipmaps() {
	glGenerateMipmap(m_target);
}

Texture& Texture::bind(TextureTarget target) {
	m_target = target;
	glBindTexture(m_target, m_id);
	return *this;
}

void Texture::bind(u32 slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(m_target, m_id);
}

void Texture::unbind() {
	glBindTexture(m_target, 0);
}

NS_END