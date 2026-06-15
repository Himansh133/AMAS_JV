#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace AMAS {
namespace Log {

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    std::stringstream ss;
    struct tm buf;
#if defined(_WIN32)
    localtime_s(&buf, &time_t_now);
#else
    localtime_r(&time_t_now, &buf);
#endif
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "." 
       << std::setfill('0') << std::setw(3) << milliseconds;
    return ss.str();
}

static std::string levelToString(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO ";
        case Level::Warning: return "WARN ";
        case Level::Error:   return "ERROR";
        default:             return "UNKN ";
    }
}

void init() {
    // For now, simple initialization message.
    info("AMAS Logger initialized.");
}

void log(Level level, const std::string& message) {
    std::string ts = getTimestamp();
    std::string lvl = levelToString(level);
    
    if (level == Level::Error) {
        std::cerr << "[" << ts << "] [" << lvl << "] " << message << std::endl;
    } else {
        std::cout << "[" << ts << "] [" << lvl << "] " << message << std::endl;
    }
}

void debug(const std::string& message) {
    log(Level::Debug, message);
}

void info(const std::string& message) {
    log(Level::Info, message);
}

void warn(const std::string& message) {
    log(Level::Warning, message);
}

void error(const std::string& message) {
    log(Level::Error, message);
}

} // namespace Log
} // namespace AMAS
