#ifndef LIGHT_H
#define LIGHT_H

#include "../math/vec.h"
#include "../math/consts.h"
#include "../core/types.h"
#include "../core/ecs.h"

NS_BEGIN

enum LightType {
	Directional = 0,
	Point,
	Spot
};

class LightBase : public Component {
public:
	LightBase() : color(Vec3(1.0f)), intensity(1.0f) {}
	
	Vec3 color;
	float intensity;
	
	virtual LightType getType() const = 0;
};

class DirectionalLight : public LightBase {
public:
	DirectionalLight() : LightBase(), shadows(false), shadowFrustumSize(10.0f), size(1.0f) {}

	bool shadows;
	float shadowFrustumSize, size;

	LightType getType() const { return LightType::Directional; }
};

class PointLight : public LightBase {
public:
	PointLight() : LightBase(), radius(5.0f), lightCutOff(0.003f) {}
	
	float radius, lightCutOff;
	
	LightType getType() const { return LightType::Point; }
};

class SpotLight : public PointLight {
public:
	SpotLight() : PointLight(), spotCutOff(Pi / 4.0f) {}

	float spotCutOff;
	
	LightType getType() const { return LightType::Spot; }
};

NS_END

#endif /* LIGHT_H */

