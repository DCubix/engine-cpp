#include "types.h"

#include "SDL.h"

const String Util::currentDateTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%m/%d/%Y %X", &tstruct);

	return buf;
}

const String Util::currentDateTimeNoFormat() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%m_%d_%Y_%H_%M_%S", &tstruct);

	return buf;
}

i32 Util::random(i32 min, i32 max) {
	return i32(random((float) min, (float) max));
}

float Util::random(float min, float max) {
	float r = (float) rand() / (float) RAND_MAX;
	return (min + r * (max - min));
}

double Util::getTime() {
	return double(SDL_GetTicks()) / 1000.0;
}
