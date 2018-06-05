#ifndef MATERIAL_H
#define MATERIAL_H

#include "../math/vec.h"
#include "../core/types.h"
#include "texture.h"

NS_BEGIN

class Material {
	friend class RendererSystem;
private:
	Material(u32 id)
		: roughness(0.5f), metallic(0.0f), emission(0.0f), baseColor(Vec3(1.0f)), heightScale(1.0f),
			discardParallaxEdges(false), instanced(false), m_id(id), castsShadow(true)
	{}

public:
	Material() : Material(-1) {}

	Vec3 baseColor;
	float metallic;
	float roughness;
	float emission;
	float heightScale;
	bool discardParallaxEdges, instanced, castsShadow;

	u32 id() const { return m_id; }
protected:
	u32 m_id;
};

NS_END

#endif /* MATERIAL_H */

