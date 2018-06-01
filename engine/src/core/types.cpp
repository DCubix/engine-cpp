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

String Util::replace(const String& str, const String& what, const String& by) {
	String rep(str);
	size_t pos = rep.find(what);

	while( pos != String::npos) {
		rep.replace(pos, what.size(), by);
		pos = rep.find(what, pos + what.size());
	}

	return rep;
}

Vector<String> Util::split(const String& text, const String& delimiter) {
	Vector<String> arr;
	String::size_type start = 0;
	String::size_type end = text.find(delimiter.c_str(), start);
	while (end != String::npos) 	{
		String token = text.substr(start, end - start);
		if (token != "")
			arr.push_back(token);
		start = end + 1;
		end = text.find(delimiter.c_str(), start);
	}
	arr.push_back(text.substr(start));
	return arr;
}
