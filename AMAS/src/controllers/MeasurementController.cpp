#include "controllers/MeasurementController.h"
#include "measurement/BandConfiguration.h"
#include "measurement/MeasurementProcessor.h"
#include "reports/ReportGenerator.h"
#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <cmath>

namespace AMAS {

MeasurementController::MeasurementController(std::shared_ptr<IVnaDriver> vna,
                                             std::shared_ptr<IPositionerDriver> positioner,
                                             std::shared_ptr<CalibrationManager> calManager,
                                             std::shared_ptr<DataLogger> logger)
    : m_vna(vna)
    , m_positioner(positioner)
    , m_calManager(calManager)
    , m_logger(logger)
    , m_vnaConnected(false)
    , m_posConnected(false)
    , m_cancelRequested(false)
{
}

MeasurementController::~MeasurementController() {
    disconnectHardware();
}

bool MeasurementController::connectHardware(const std::string& vnaResource, const std::string& posPort) {
    disconnectHardware();

    Log::info("MeasurementController: Connecting VNA at " + vnaResource);
    m_vnaConnected = m_vna->connect(vnaResource);
    if (!m_vnaConnected) {
        Log::error("MeasurementController: VNA connection failed.");
    }

    Log::info("MeasurementController: Connecting Positioner at " + posPort);
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

bool MeasurementController::isVnaConnected() const {
    return m_vnaConnected && m_vna->isConnected();
}

bool MeasurementController::isPositionerConnected() const {
    return m_posConnected && m_positioner->isConnected();
}

void MeasurementController::requestCancellation() {
    m_cancelRequested = true;
    Log::warn("MeasurementController: Cancellation requested by user.");
}

bool MeasurementController::isCancellationRequested() const {
    return m_cancelRequested.load();
}

bool MeasurementController::runMeasurement(const MeasurementProfile& profile,
                                           MeasurementSession& outSession,
                                           ProgressCallback progressCb) {
    try {
        m_cancelRequested = false;

        if (!isVnaConnected()) {
            throw std::runtime_error("VNA is not connected.");
        }

        // 1. Setup Session Info & Timestamp
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ssTime;
        #ifdef _MSC_VER
            struct tm timeinfo;
            localtime_s(&timeinfo, &in_time_t);
            ssTime << std::put_time(&timeinfo, "%Y-%m-%d_%H-%M-%S");
        #else
            ssTime << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
        #endif
        std::string ts = ssTime.str();

        outSession.clear();
        outSession.profile = profile;
        outSession.sessionName = profile.profileName;
        outSession.measurementType = profile.measurementType;
        outSession.timestamp = ts;

        // Determine Band Name
        BandInfo bandInfo;
        if (BandConfiguration::getBandByFrequency(profile.startFrequencyHz, bandInfo)) {
            outSession.bandName = bandInfo.name;
        } else {
            outSession.bandName = "Custom Band";
        }

        if (progressCb) progressCb(5.0f, "Initializing session details...");

        // 2. Calibration Lookup & Loading
        std::string calFile = profile.calibrationFile;
        if (calFile.empty() && m_calManager) {
            calFile = m_calManager->getCalibrationFile(profile.measurementType, outSession.bandName);
        }
        outSession.calibrationFile = calFile;

        if (!calFile.empty() && m_calManager) {
            if (progressCb) progressCb(10.0f, "Checking calibration file: " + calFile);
            
            // Note: Calibration files reside inside VNA flash memory.
            // Local path validation is a utility helper; load to VNA directly.
            Log::info("MeasurementController: Command VNA to load calibration: " + calFile);
            if (!m_vna->loadCalibrationState(calFile)) {
                Log::warn("MeasurementController: VNA calibration load returned warning. Continuing sweep.");
            }
        }

        // 3. VNA Configuration
        if (progressCb) progressCb(20.0f, "Configuring VNA settings...");
        
        SweepConfig config;
        config.startFreq_Hz = profile.startFrequencyHz;
        config.stopFreq_Hz = profile.stopFrequencyHz;
        config.numPoints = profile.sweepPoints;
        config.ifbw_Hz = profile.ifBandwidthHz;
        config.power_dBm = profile.outputPowerDbm;
        
        SParamType sp = SParamType::S11;
        if (profile.measurementType == "S21" || profile.measurementType == "Gain" || profile.measurementType == "RadiationPattern") {
            sp = SParamType::S21;
        }
        config.sparams = { sp };

        if (!m_vna->configure(config)) {
            throw std::runtime_error("Failed to configure VNA sweep configuration.");
        }

        // 4. Sweep Execution
        bool usePos = profile.positioner.usePositioner;
        if (usePos && !isPositionerConnected()) {
            Log::warn("MeasurementController: Physical positioner requested but offline. Falling back to simulation mode.");
            usePos = false;
        }

        bool isStatic = (profile.measurementType == "S11" || profile.measurementType == "S21");

        if (isStatic) {
            // --- Static Single Angle Sweep ---
            if (progressCb) progressCb(40.0f, "Triggering static sweep...");
            
            if (usePos) {
                m_positioner->moveTo(profile.positioner.startAngleDeg);
                m_positioner->waitForSettle(profile.positioner.settleTimeMs);
            }

            if (!m_vna->triggerSweep()) {
                throw std::runtime_error("VNA static sweep trigger failed.");
            }

            if (progressCb) progressCb(70.0f, "Reading sweep traces...");
            auto traces = m_vna->readSParameters();
            if (traces.empty()) {
                throw std::runtime_error("VNA returned empty traces.");
            }

            const auto& trace = traces[0];
            for (int i = 0; i < profile.sweepPoints; ++i) {
                ProcessedMeasurementPoint pt;
                pt.angleDeg = usePos ? profile.positioner.startAngleDeg : 0.0f;
                pt.frequencyHz = trace.frequencies_Hz[i];
                pt.magnitudeDb = trace.magnitudes_dB[i];
                pt.phaseDeg = trace.phases_deg[i];
                pt.returnLossDb = MeasurementProcessor::calculateReturnLoss(pt.magnitudeDb);
                pt.vswr = MeasurementProcessor::calculateVSWR(pt.magnitudeDb);
                outSession.results.push_back(pt);
            }

        } else {
            // --- Coordinated Angular Sweep ---
            float currAngle = profile.positioner.startAngleDeg;
            float endAngle = profile.positioner.stopAngleDeg;
            float step = profile.positioner.stepAngleDeg;

            if (step <= 0.0f) step = 5.0f;

            int stepsCount = static_cast<int>(std::floor((endAngle - currAngle) / step)) + 1;
            int currentStep = 0;

            while (currAngle <= endAngle) {
                if (m_cancelRequested.load()) {
                    Log::warn("MeasurementController: Sweep cancelled by user request.");
                    if (usePos) {
                        m_positioner->home();
                    }
                    return false;
                }

                float percent = 30.0f + 60.0f * (static_cast<float>(currentStep) / stepsCount);
                std::stringstream status;
                status << "Scanning angle: " << std::fixed << std::setprecision(1) << currAngle << " deg...";
                if (progressCb) progressCb(percent, status.str());

                // Move positioner
                if (usePos) {
                    if (!m_positioner->moveTo(currAngle)) {
                        throw std::runtime_error("Positioner failed to move to: " + std::to_string(currAngle));
                    }
                    m_positioner->waitForSettle(profile.positioner.settleTimeMs);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(profile.positioner.settleTimeMs));
                }

                // Trigger VNA
                if (!m_vna->triggerSweep()) {
                    throw std::runtime_error("VNA sweep failed at angle: " + std::to_string(currAngle));
                }

                auto traces = m_vna->readSParameters();
                if (traces.empty()) {
                    throw std::runtime_error("VNA returned empty traces at angle: " + std::to_string(currAngle));
                }

                const auto& trace = traces[0];
                for (int i = 0; i < profile.sweepPoints; ++i) {
                    ProcessedMeasurementPoint pt;
                    pt.angleDeg = currAngle;
                    pt.frequencyHz = trace.frequencies_Hz[i];
                    pt.magnitudeDb = trace.magnitudes_dB[i];
                    pt.phaseDeg = trace.phases_deg[i];
                    pt.returnLossDb = MeasurementProcessor::calculateReturnLoss(pt.magnitudeDb);
                    pt.vswr = MeasurementProcessor::calculateVSWR(pt.magnitudeDb);
                    outSession.results.push_back(pt);
                }

                currAngle += step;
                currentStep++;
            }

            // Return to home
            if (usePos) {
                if (progressCb) progressCb(92.0f, "Homing turntable positioner...");
                m_positioner->home();
                m_positioner->waitForSettle(5000);
            }
        }

        // 5. Data Logging
        if (progressCb) progressCb(95.0f, "Saving sweep results to disk...");
        std::string loggedDir;
        if (m_logger) {
            if (!m_logger->logSession(outSession, loggedDir)) {
                Log::error("MeasurementController: Session logging failed.");
            }
            outSession.outputFolder = loggedDir;
        }

        // 6. Report Generation
        if (progressCb) progressCb(98.0f, "Generating analysis reports...");
        if (!loggedDir.empty()) {
            auto mdReport = ReportGenerator::generateReport(outSession, ReportFormat::Markdown);
            ReportGenerator::saveReportToFile(mdReport, loggedDir + "/measurement_report.md");

            auto htmlReport = ReportGenerator::generateReport(outSession, ReportFormat::HTML);
            ReportGenerator::saveReportToFile(htmlReport, loggedDir + "/measurement_report.html");
        }

        if (progressCb) progressCb(100.0f, "Session completed successfully!");
        return true;

    } catch (const std::exception& ex) {
        Log::error("MeasurementController: Exception caught during sweep: " + std::string(ex.what()));
        if (progressCb) progressCb(100.0f, "Sweep aborted: " + std::string(ex.what()));
        return false;
    }
}

} // namespace AMAS
