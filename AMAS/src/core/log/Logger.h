#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace AMAS {
namespace Log {

enum class Level {
    Debug,
    Info,
    Warning,
    Error
};

void init();
void log(Level level, const std::string& message);

void debug(const std::string& message);
void info(const std::string& message);
void warn(const std::string& message);
void error(const std::string& message);

} // namespace Log
} // namespace AMAS

#endif // LOGGER_H
