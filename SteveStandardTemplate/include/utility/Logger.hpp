#pragma once

#include <stdafx.hpp>
#if DEBUG
#include <utility/GameUtility.hpp>
#endif

namespace SteveBase::Utility {
#if DEBUG
	inline std::string GetTimeStamp() {
		time_t rawtime;
		char timebuffer[128];

		time(&rawtime);
		auto timeinfo = localtime(&rawtime);

		strftime(timebuffer, sizeof timebuffer, "%Y-%m-%d %H:%M:%S", timeinfo);

		return std::string(timebuffer);
	}

    inline void AppendLog(std::string location, std::string msg) {
#if 0
        std::ofstream ifs(location, std::ios_base::app);
        if (ifs) {
            ifs << msg;
            ifs.close();
        }
#endif
    }

#define LogLocation (textonce("c:/pubg_log.txt"))

#define LoggerDebug(logFormat, ...) \
    do { \
        auto msg = fmt::format(textonce("[{}] [DEBUG] {}: {}\n"), GetTimeStamp(), textonce(__FUNCTION__), fmt::format(textonce(logFormat), __VA_ARGS__)); \
        AppendLog(LogLocation, msg); \
        fmt::print(msg); \
    } while(0)

#define LoggerNotice(logFormat, ...) \
    do { \
        auto msg = fmt::format(textonce("[{}] [NOTICE] {}: {}\n"), GetTimeStamp(), textonce(__FUNCTION__), fmt::format(textonce(logFormat), __VA_ARGS__)); \
        AppendLog(LogLocation, msg); \
        fmt::print(msg); \
    } while(0)

#define LoggerWarning(logFormat, ...) \
    do { \
        auto msg = fmt::format(textonce("[{}] [WARNING] {}: {}\n"), GetTimeStamp(), textonce(__FUNCTION__), fmt::format(textonce(logFormat), __VA_ARGS__)); \
        AppendLog(LogLocation, msg); \
        fmt::print(msg); \
    } while(0)

#define LoggerError(logFormat, ...) \
    do { \
        auto msg = fmt::format(textonce("[{}] [ERROR] {}: {}\n"), GetTimeStamp(), textonce(__FUNCTION__), fmt::format(textonce(logFormat), __VA_ARGS__)); \
        AppendLog(LogLocation, msg); \
        fmt::print(msg); \
    } while(0)

#else

// nullify all Logger Calls
#define LoggerDebug(...) ((void) 0)
#define LoggerNotice(...) ((void) 0)
#define LoggerWarning(...) ((void) 0)
#define LoggerError(...) ((void) 0)

#endif
}