#include "MockVnaDriver.h"
#include "Logger.h"
#include <thread>
#include <chrono>
#include <cmath>

namespace AMAS {

MockVnaDriver::MockVnaDriver()
    : m_isConnected(false)
    , m_idnString("Keysight Technologies,N9951B,MOCK-N9951B-0001,A.10.15") {
}

MockVnaDriver::~MockVnaDriver() {
    disconnect();
}

bool MockVnaDriver::connect(const std::string& resource) {
    disconnect();
    Log::info("MockVnaDriver: Connecting to simulated resource: " + resource);
    
    // Simulate connection latency
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_isConnected = true;
    
    Log::info("MockVnaDriver: Successfully connected. IDN: " + m_idnString);
    return true;
}

void MockVnaDriver::disconnect() {
    if (m_isConnected) {
        m_isConnected = false;
        Log::info("MockVnaDriver: Disconnected.");
    }
}

bool MockVnaDriver::configure(const SweepConfig& config) {
    if (!m_isConnected) {
        Log::error("MockVnaDriver: Cannot configure VNA, not connected.");
        return false;
    }

    if (!config.validate()) {
        Log::error("MockVnaDriver: Configuration validation failed.");
        return false;
    }

    m_config = config;
    Log::info("MockVnaDriver: VNA sweep configured with " + std::to_string(config.numPoints) + " points.");
    return true;
}

bool MockVnaDriver::triggerSweep() {
    if (!m_isConnected) {
        Log::error("MockVnaDriver: Cannot trigger sweep, not connected.");
        return false;
    }

    Log::info("MockVnaDriver: Triggering simulated sweep...");
    
    // Simulate measurement time (depends on IF bandwidth and points)
    // E.g., 201 points at 1 kHz IFBW is quick, let's simulate ~100 ms of sweep time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Log::info("MockVnaDriver: Simulated sweep finished.");
    return true;
}

std::vector<SParamData> MockVnaDriver::readSParameters() {
    std::vector<SParamData> results;

    if (!m_isConnected) {
        Log::error("MockVnaDriver: Cannot read S-parameters, not connected.");
        return results;
    }

    int nPoints = m_config.numPoints;
    results.reserve(m_config.sparams.size());

    // Frequencies vector
    std::vector<double> frequencies(nPoints);
    double stepFreq = 0.0;
    if (nPoints > 1) {
        stepFreq = (m_config.stopFreq_Hz - m_config.startFreq_Hz) / (nPoints - 1);
    }
    for (int i = 0; i < nPoints; ++i) {
        frequencies[i] = m_config.startFreq_Hz + i * stepFreq;
    }

    // Midpoint of sweep as the resonance frequency f0
    double f0 = 0.5 * (m_config.startFreq_Hz + m_config.stopFreq_Hz);
    double Q = 50.0; // Quality factor

    for (auto sparam : m_config.sparams) {
        SParamData traceData;
        traceData.type = sparam;
        traceData.frequencies_Hz = frequencies;
        traceData.magnitudes_dB.resize(nPoints);
        traceData.phases_deg.resize(nPoints);

        for (int i = 0; i < nPoints; ++i) {
            double f = frequencies[i];
            
            // Normalized frequency difference factor
            double ratio = (f / f0) - (f0 / f);
            double resonanceFactor = 1.0 + Q * Q * ratio * ratio;

            if (sparam == SParamType::S11) {
                // S11 Reflection / Return Loss Dip (Lorentzian shape)
                // Dip goes down to -30 dB at f0, baseline is -5 dB
                traceData.magnitudes_dB[i] = -5.0 - (25.0 / resonanceFactor);
                
                // Phase behavior around resonance (steep transition)
                double phaseOffset = -std::atan(Q * ratio) * 180.0 / 3.14159265358979323846;
                double linearPhase = -180.0 * (f - m_config.startFreq_Hz) / (m_config.stopFreq_Hz - m_config.startFreq_Hz);
                traceData.phases_deg[i] = linearPhase + phaseOffset;

            } else if (sparam == SParamType::S21) {
                // S21 Transmission / Gain Peak (Lorentzian shape)
                // Peak is at -5 dB at f0, baseline is -45 dB
                traceData.magnitudes_dB[i] = -45.0 + (40.0 / resonanceFactor);
                
                // Phase ramp
                double phaseOffset = std::atan(Q * ratio) * 180.0 / 3.14159265358979323846;
                double linearPhase = -360.0 * (f - m_config.startFreq_Hz) / (m_config.stopFreq_Hz - m_config.startFreq_Hz);
                traceData.phases_deg[i] = linearPhase + phaseOffset;

            } else {
                // Default flat profile for S12/S22
                traceData.magnitudes_dB[i] = -15.0;
                traceData.phases_deg[i] = 0.0;
            }
        }

        results.push_back(traceData);
    }

    return results;
}

bool MockVnaDriver::isConnected() const {
    return m_isConnected;
}

std::string MockVnaDriver::getIdnString() {
    return m_idnString;
}

} // namespace AMAS
