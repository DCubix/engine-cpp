#ifndef BUILDER_H
#define BUILDER_H

#include "types.h"

template <typename T>
class Builder {
public:
	static T build() {}
	static void clean() {}
	static void destroy(T object) {}
};

#endif /* BUILDER_H */

