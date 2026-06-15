#include "MockPositionerDriver.h"
#include "Logger.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace AMAS {

MockPositionerDriver::MockPositionerDriver()
    : m_isConnected(false)
    , m_currentAngle(0.0f)
    , m_targetAngle(0.0f)
    , m_isMoving(false) {
}

MockPositionerDriver::~MockPositionerDriver() {
    disconnect();
}

bool MockPositionerDriver::connect(const std::string& port) {
    disconnect();
    Log::info("MockPositionerDriver: Connecting to simulated serial port: " + port);
    
    // Simulate connection delay
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    m_isConnected = true;
    m_currentAngle = 0.0f;
    m_targetAngle = 0.0f;
    m_isMoving = false;
    
    Log::info("MockPositionerDriver: Successfully connected on " + port);
    return true;
}

void MockPositionerDriver::disconnect() {
    if (m_isConnected) {
        m_isConnected = false;
        m_isMoving = false;
        Log::info("MockPositionerDriver: Disconnected.");
    }
}

bool MockPositionerDriver::moveTo(float angle_deg) {
    if (!m_isConnected) {
        Log::error("MockPositionerDriver: Cannot move positioner, not connected.");
        return false;
    }

    // Keep angle normalized between -360 and 360
    m_targetAngle = angle_deg;
    m_isMoving = (std::abs(m_targetAngle - m_currentAngle) > 0.01f);
    
    Log::info("MockPositionerDriver: Initiating move to angle: " + std::to_string(angle_deg) + " deg.");
    return true;
}

float MockPositionerDriver::getCurrentAngle() {
    return m_currentAngle;
}

bool MockPositionerDriver::waitForSettle(int timeout_ms) {
    if (!m_isConnected) {
        Log::error("MockPositionerDriver: Cannot wait for settle, not connected.");
        return false;
    }

    if (!m_isMoving) {
        return true;
    }

    // Calculate rotation time based on a simulated speed of 30 degrees/second
    float delta = std::abs(m_targetAngle - m_currentAngle);
    float rotationSpeed = 30.0f; // deg/sec
    int simulatedTimeMs = static_cast<int>((delta / rotationSpeed) * 1000.0f) + 50; // add 50ms settling cushion

    Log::info("MockPositionerDriver: Turntable rotating... Estimated time: " + std::to_string(simulatedTimeMs) + " ms");

    if (simulatedTimeMs > timeout_ms) {
        // Simulating timeout condition
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        
        // Approximate position at timeout
        float direction = (m_targetAngle > m_currentAngle) ? 1.0f : -1.0f;
        m_currentAngle += direction * (rotationSpeed * (timeout_ms / 1000.0f));
        m_isMoving = false;
        
        Log::error("MockPositionerDriver: Timeout occurred before turntable settled.");
        return false;
    }

    // Normal movement completion
    std::this_thread::sleep_for(std::chrono::milliseconds(simulatedTimeMs));
    m_currentAngle = m_targetAngle;
    m_isMoving = false;
    
    Log::info("MockPositionerDriver: Rotation complete. Settled at angle: " + std::to_string(m_currentAngle) + " deg.");
    return true;
}

void MockPositionerDriver::home() {
    if (!m_isConnected) {
        Log::error("MockPositionerDriver: Cannot home, not connected.");
        return;
    }
    Log::info("MockPositionerDriver: Homing positioner to 0.0 deg.");
    moveTo(0.0f);
    waitForSettle(15000); // 15 seconds max home time
}

void MockPositionerDriver::emergencyStop() {
    if (!m_isConnected) return;
    Log::warn("MockPositionerDriver: EMERGENCY STOP TRIGGERED.");
    m_targetAngle = m_currentAngle;
    m_isMoving = false;
}

bool MockPositionerDriver::isConnected() const {
    return m_isConnected;
}

} // namespace AMAS
