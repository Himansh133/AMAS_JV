#include "MeasurementController.h"
#include "Logger.h"
#include "SweepConfig.h"
#include <iostream>
#include <cmath>
#include <windows.h>

namespace AMAS {

MeasurementController::MeasurementController(IVnaDriver* vna, IPositionerDriver* positioner)
    : m_vna(vna), m_positioner(positioner), m_vnaConnected(false), m_posConnected(false) {
}

bool MeasurementController::connectHardware(const std::string& vnaResource, const std::string& posPort) {
    disconnectHardware();

    Log::info("MeasurementController: Connecting to VNA at " + vnaResource);
    m_vnaConnected = m_vna->connect(vnaResource);
    if (!m_vnaConnected) {
        Log::error("MeasurementController: VNA connection failed.");
    }

    Log::info("MeasurementController: Connecting to Positioner at " + posPort);
    m_posConnected = m_positioner->connect(posPort);
    if (!m_posConnected) {
        Log::error("MeasurementController: Positioner connection failed.");
    }

    return m_vnaConnected;
}

void MeasurementController::disconnectHardware() {
    if (m_vnaConnected) {
        m_vna->disconnect();
        m_vnaConnected = false;
    }
    if (m_posConnected) {
        m_positioner->disconnect();
        m_posConnected = false;
    }
}

std::string MeasurementController::getSParamChoiceString(SParamType type) const {
    switch (type) {
        case SParamType::S11: return "S11";
        case SParamType::S21: return "S21";
        case SParamType::S12: return "S12";
        case SParamType::S22: return "S22";
        default: return "S21";
    }
}

bool MeasurementController::runMeasurement(const MeasurementProfile& profile, MeasurementSession& outSession) {
    if (!m_vnaConnected) {
        Log::error("MeasurementController: Cannot run sweep, VNA not connected.");
        return false;
    }

    outSession.profile = profile;
    outSession.clear();

    // 1. Load Calibration State
    if (!profile.band.calibrationFile.empty()) {
        Log::info("MeasurementController: Loading preset calibration...");
        if (!m_vna->loadCalibrationState(profile.band.calibrationFile)) {
            Log::error("MeasurementController: Calibration load failed. Aborting.");
            return false;
        }
    }

    // 2. Setup VNA Sweep Parameters
    SweepConfig config;
    config.startFreq_Hz = profile.band.startFreq_Hz;
    config.stopFreq_Hz = profile.band.stopFreq_Hz;
    config.numPoints = profile.band.sweepPoints;
    config.ifbw_Hz = profile.ifbw_Hz;
    config.power_dBm = profile.power_dBm;
    config.sparams = { profile.sparam };

    Log::info("MeasurementController: Configuring VNA...");
    if (!m_vna->configure(config)) {
        Log::error("MeasurementController: VNA configuration failed.");
        return false;
    }

    // Generate frequencies list for data storage
    std::vector<double> frequencies(profile.band.sweepPoints);
    double step = (profile.band.sweepPoints > 1) 
        ? (profile.band.stopFreq_Hz - profile.band.startFreq_Hz) / (profile.band.sweepPoints - 1) 
        : 0.0;
    for (int i = 0; i < profile.band.sweepPoints; ++i) {
        frequencies[i] = profile.band.startFreq_Hz + i * step;
    }

    // 3. Execution (Static Sweep vs Coordinated Sweep)
    bool isStatic = (profile.type == MeasurementType::S11 || profile.type == MeasurementType::S21);

    if (isStatic) {
        // --- Static sweep ---
        Log::info("MeasurementController: Starting static sweep...");
        if (!m_vna->triggerSweep()) {
            Log::error("MeasurementController: VNA sweep trigger failed.");
            return false;
        }

        auto traces = m_vna->readSParameters();
        if (traces.empty()) {
            Log::error("MeasurementController: Failed to read S-parameter results from VNA.");
            return false;
        }

        const auto& trace = traces[0];
        for (int i = 0; i < profile.band.sweepPoints; ++i) {
            MeasurementPoint pt;
            pt.angle_deg = 0.0f;
            pt.frequency_Hz = trace.frequencies_Hz[i];
            pt.magnitude_dB = trace.magnitudes_dB[i];
            pt.phase_deg = trace.phases_deg[i];
            outSession.points.push_back(pt);
        }

        Log::info("MeasurementController: Static sweep completed successfully.");
        return true;
    } else {
        // --- Coordinated sweep with turntable ---
        if (!m_posConnected) {
            Log::warn("MeasurementController: Turntable is offline. Running coordinated sweep with simulated positioner.");
        }

        Log::info("MeasurementController: Starting coordinated angular sweep...");
        float currentAngle = profile.startAngle_deg;
        while (currentAngle <= profile.stopAngle_deg) {
            Log::info("MeasurementController: Rotating turntable to " + std::to_string(currentAngle) + " deg...");
            
            if (m_posConnected) {
                if (!m_positioner->moveTo(currentAngle)) {
                    Log::error("MeasurementController: Positioner failed to move to " + std::to_string(currentAngle));
                    return false;
                }
                m_positioner->waitForSettle(profile.settleTime_ms);
            } else {
                // Mock delay
                Sleep(500);
            }

            Log::info("MeasurementController: Triggering VNA sweep at angle " + std::to_string(currentAngle) + "...");
            if (!m_vna->triggerSweep()) {
                Log::error("MeasurementController: VNA sweep failed at angle " + std::to_string(currentAngle));
                return false;
            }

            auto traces = m_vna->readSParameters();
            if (traces.empty()) {
                Log::error("MeasurementController: Trace read failed at angle " + std::to_string(currentAngle));
                return false;
            }

            const auto& trace = traces[0];
            for (int i = 0; i < profile.band.sweepPoints; ++i) {
                MeasurementPoint pt;
                pt.angle_deg = currentAngle;
                pt.frequency_Hz = trace.frequencies_Hz[i];
                pt.magnitude_dB = trace.magnitudes_dB[i];
                pt.phase_deg = trace.phases_deg[i];
                outSession.points.push_back(pt);
            }

            currentAngle += profile.stepAngle_deg;
        }

        // Home turntable
        Log::info("MeasurementController: Coordinated sweep finished. Homing turntable...");
        if (m_posConnected) {
            m_positioner->home();
            m_positioner->waitForSettle(10000);
        }

        return true;
    }
}

} // namespace AMAS
