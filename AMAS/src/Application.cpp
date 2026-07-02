#include "Application.h"
#include "Logger.h"
#include "controllers/MeasurementController.h"
#include "core/vna/MockVnaDriver.h"
#include "core/positioner/MockPositionerDriver.h"
#include "measurement/CalibrationManager.h"
#include "logger/DataLogger.h"
#include "gui/MainWindow.h"

namespace AMAS {

Application::Application(QObject *parent)
    : QObject(parent) {
}

Application::~Application() = default;

bool Application::initialize() {
    Log::init();
    Log::info("AMAS Application layer initialized successfully.");

    // Instantiate simulated backend drivers & logger
    m_vna = std::make_shared<MockVnaDriver>();
    m_positioner = std::make_shared<MockPositionerDriver>();
    m_calManager = std::make_shared<CalibrationManager>();
    m_logger = std::make_shared<DataLogger>();
    m_logger->setBaseDirectory("simulated_runs");

    // Coordinated Controller
    m_controller = std::make_shared<MeasurementController>(m_vna, m_positioner, m_calManager, m_logger);
    
    // Pass to MainWindow (Dependency Injection)
    m_mainWindow = std::make_unique<MainWindow>(m_controller);
    return true;
}

void Application::run() {
    Log::info("AMAS Application event loop running.");
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}

} // namespace AMAS
