#include "log.h"

#include <assert.h>

#include "termcolor.hpp"

NS_BEGIN

//#ifdef ENG_DEBUG
//Logger Logger::logger = Logger();
//std::ofstream* Logger::logFile = nullptr;
//#else
std::ofstream* Logger::logFile = new std::ofstream("Log_" + Util::currentDateTimeNoFormat() + ".txt");
Logger Logger::logger = Logger(Logger::logFile);
//#endif

Logger::Logger() : m_output(&std::cout) {
}

Logger::Logger(std::ostream* output) : m_output(output) {
	if (m_output->bad()) {
		assert(false);
	}
}

Logger::~Logger() {
	m_output->flush();
	static std::stringstream closed_flag;
	m_output->rdbuf(closed_flag.rdbuf());
	if (logFile && *logFile) {
		logFile->close();
		delete logFile;
	}
}
	
static String replaceAll(const String& in, const String& what, const String& by) {
	String str = mov(in);
	size_t index = 0;
	while (true) {
		index = str.find(what, index);
		if (index == std::string::npos) break;

		str.replace(what.size(), 3, by);

		index += what.size();
	}
	return str;
}

void Logger::print(LogLevel level, const char* file, const char* function, int line, const String& msg) {
	// [12/12/2017 23:45] => [ERROR] [func@33] Test error!
	String prefx = "[" + Util::currentDateTime() + "] ";
	String filen = file;
	std::replace(filen.begin(), filen.end(), '\\', '/');
	filen = filen.substr(filen.find_last_of('/') + 1);

#ifdef ENG_DEBUG
	(*m_output) << termcolor::green << termcolor::dark;
#endif

	(*m_output) << prefx;

#ifdef ENG_DEBUG
	switch (level) {
		case LogLevel::Debug: (*m_output) << termcolor::cyan; break;
		case LogLevel::Info: (*m_output) << termcolor::blue; break;
		case LogLevel::Warning: (*m_output) << termcolor::yellow; break;
		case LogLevel::Error: (*m_output) << termcolor::red; break;
		case LogLevel::Fatal: (*m_output) << termcolor::magenta; break;
	}
#endif
	switch (level) {
		case LogLevel::Debug: (*m_output) << "[DBG]"; break;
		case LogLevel::Info: (*m_output) << "[INF]"; break;
		case LogLevel::Warning: (*m_output) << "[WRN]"; break;
		case LogLevel::Error: (*m_output) << "[ERR]"; break;
		case LogLevel::Fatal: (*m_output) << "[FTL]"; break;
	}

#ifdef ENG_DEBUG
	(*m_output) << termcolor::reset;
#endif

	(*m_output) << " [" << filen << "(" << function << " @ " << line << ")] " << msg << std::endl;
}

NS_END