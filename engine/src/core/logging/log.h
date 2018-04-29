#ifndef TFX_CORE_LOG_H
#define TFX_CORE_LOG_H

#include "../types.h"
#include <fstream>
#include <iostream>

NS_BEGIN
enum LogLevel {
	Debug = 0,
	Info,
	Warning,
	Error,
	Fatal
};

class Logger {
public:
	Logger();
	Logger(std::ostream* output);
	~Logger();

	void print(LogLevel level, const char* file, const char* function, int line, const String& msg);

	static Logger& getSingleton() { return logger; }
private:
	std::ostream* m_output;

	static Logger logger;
	static std::ofstream* logFile;
};

NS_END

#ifdef USE_NAMESPACE
#define LOGGER eng::Logger::getSingleton()
#else
#define LOGGER Logger::getSingleton()
#endif

template<typename T>
String Str(const T& value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

template<typename T, typename ... Args >
String Str(const T& value, const Args& ... args) {
	return Str(value) + Str(args...);
}

#define Print(l, ...) LOGGER.print(l, __FILE__, __FUNCTION__, __LINE__, Str(__VA_ARGS__))
#define Log(...) Print(LogLevel::Debug, __VA_ARGS__)
#define LogInfo(...) Print(LogLevel::Info, __VA_ARGS__)
#define LogWarning(...) Print(LogLevel::Warning, __VA_ARGS__)
#define LogError(...) Print(LogLevel::Error, __VA_ARGS__)
#define LogFatal(...) Print(LogLevel::Fatal, __VA_ARGS__)

#endif // TFX_CORE_LOG_H