#ifndef AABB_H
#define AABB_H

#include "vec.h"
#include "../core/types.h"

#include <cmath>
#include <cfloat>

NS_BEGIN

class AABB {
public:
	AABB() : m_min(FLT_MAX), m_max(FLT_MIN) {}
	AABB(const Vec3& min, const Vec3& max) : m_min(min), m_max(max) {}
	
	Vec3 min() const { return m_min; }
	Vec3 max() const { return m_max; }
	
private:
	Vec3 m_min, m_max;
};
		
NS_END

#endif /* AABB_H */

