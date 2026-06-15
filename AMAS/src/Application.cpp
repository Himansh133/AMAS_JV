#include "Application.h"
#include "Logger.h"

namespace AMAS {

Application::Application(QObject *parent)
    : QObject(parent) {
}

bool Application::initialize() {
    Log::init();
    Log::info("AMAS Application layer initialized successfully.");
    return true;
}

void Application::run() {
    Log::info("AMAS Application event loop running.");
}

} // namespace AMAS
