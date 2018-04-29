#ifndef BUILDER_H
#define BUILDER_H

#include "types.h"

template <typename T>
class Builder {
public:
	static T create() {}
	static void clean() {}
};

#endif /* BUILDER_H */

