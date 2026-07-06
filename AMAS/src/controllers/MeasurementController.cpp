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
#include <algorithm>

namespace AMAS {

MeasurementController::MeasurementController(std::shared_ptr<IVnaDriver> vna,
                                             std::shared_ptr<IPositionerDriver> positioner,
                                             std::shared_ptr<CalibrationManager> calManager,
                                             std::shared_ptr<DataLogger> logger,
                                             QObject* parent)
    : QObject(parent)
    , m_vna(vna)
    , m_positioner(positioner)
    , m_calManager(calManager)
    , m_logger(logger)
    , m_vnaConnected(false)
    , m_posConnected(false)
    , m_cancelRequested(false)
    , m_measurementStatus("Idle")
    , m_measurementProgress(0)
    , m_currentFrequency(0.0)
    , m_currentAngle(0.0f)
    , m_estimatedRemainingTime(0.0)
{
    // Initialize default mock profiles
    MeasurementProfile p1;
    p1.profileName = "Horn 8-12 GHz";
    p1.measurementType = "Gain";
    p1.startFrequencyHz = 8.0e9;
    p1.stopFrequencyHz = 12.0e9;
    p1.sweepPoints = 401;
    p1.ifBandwidthHz = 1000.0;
    p1.outputPowerDbm = -10.0;
    p1.calibrationFile = "calchamber8_12ghz.sta";
    p1.positioner.usePositioner = true;
    p1.positioner.startAngleDeg = 0.0f;
    p1.positioner.stopAngleDeg = 180.0f;
    p1.positioner.stepAngleDeg = 10.0f;
    p1.positioner.settleTimeMs = 150;
    m_profiles.push_back(p1);

    MeasurementProfile p2;
    p2.profileName = "Patch 12-18 GHz";
    p2.measurementType = "S11";
    p2.startFrequencyHz = 12.0e9;
    p2.stopFrequencyHz = 18.0e9;
    p2.sweepPoints = 601;
    p2.ifBandwidthHz = 1000.0;
    p2.outputPowerDbm = -10.0;
    p2.calibrationFile = "cal_patch_12_18.sta";
    p2.positioner.usePositioner = false;
    m_profiles.push_back(p2);

    m_activeProfile = p1;
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

    if (m_vnaConnected && m_posConnected) {
        emit deviceConnected();
    }
    emit systemStatusChanged();
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
    emit deviceDisconnected();
    emit systemStatusChanged();
}

bool MeasurementController::isVnaConnected() const {
    return m_vnaConnected && m_vna->isConnected();
}

bool MeasurementController::isPositionerConnected() const {
    return m_posConnected && m_positioner->isConnected();
}

std::string MeasurementController::getVnaIdn() const {
    if (m_vnaConnected && m_vna) {
        return m_vna->getIdnString();
    }
    return "Keysight FieldFox N9951B (Offline)";
}

std::string MeasurementController::getPositionerIdn() const {
    return "TAP-3001 Turntable Positioner";
}

std::string MeasurementController::getVnaResource() const {
    return "TCPIP0::192.168.113.206::inst0::INSTR";
}

std::string MeasurementController::getPositionerPort() const {
    return "COM3";
}

void MeasurementController::requestCancellation() {
    m_cancelRequested = true;
    Log::warn("MeasurementController: Cancellation requested by user.");
}

bool MeasurementController::isCancellationRequested() const {
    return m_cancelRequested.load();
}

std::string MeasurementController::getMeasurementStatus() const {
    return m_measurementStatus;
}

int MeasurementController::getMeasurementProgress() const {
    return m_measurementProgress;
}

double MeasurementController::getCurrentFrequency() const {
    return m_currentFrequency;
}

float MeasurementController::getCurrentAngle() const {
    return m_currentAngle;
}

double MeasurementController::getEstimatedRemainingTime() const {
    return m_estimatedRemainingTime;
}

std::vector<MeasurementProfile> MeasurementController::getProfiles() const {
    return m_profiles;
}

MeasurementProfile MeasurementController::getActiveProfile() const {
    return m_activeProfile;
}

void MeasurementController::setActiveProfile(const MeasurementProfile& profile) {
    m_activeProfile = profile;
    emit profileLoaded();
    emit systemStatusChanged();
}

bool MeasurementController::saveProfile(const MeasurementProfile& profile) {
    auto it = std::find_if(m_profiles.begin(), m_profiles.end(), [&](const MeasurementProfile& p) {
        return p.profileName == profile.profileName;
    });
    if (it != m_profiles.end()) {
        *it = profile;
    } else {
        m_profiles.push_back(profile);
    }
    emit profileSaved();
    emit systemStatusChanged();
    return true;
}

bool MeasurementController::deleteProfile(const std::string& profileName) {
    auto it = std::remove_if(m_profiles.begin(), m_profiles.end(), [&](const MeasurementProfile& p) {
        return p.profileName == profileName;
    });
    if (it != m_profiles.end()) {
        m_profiles.erase(it, m_profiles.end());
        emit profileDeleted();
        emit systemStatusChanged();
        return true;
    }
    return false;
}

MeasurementSession MeasurementController::getLatestSession() const {
    return m_latestSession;
}

bool MeasurementController::runMeasurement(const MeasurementProfile& profile,
                                           MeasurementSession& outSession,
                                           ProgressCallback progressCb) {
    m_measurementStatus = "Starting...";
    m_measurementProgress = 0;
    m_currentFrequency = profile.startFrequencyHz;
    m_currentAngle = profile.positioner.startAngleDeg;
    m_estimatedRemainingTime = 0.0;

    emit measurementStarted();
    emit systemStatusChanged();

    auto wrappedCb = [this, progressCb](float percent, const std::string& msg) {
        m_measurementProgress = static_cast<int>(percent);
        m_measurementStatus = msg;
        emit measurementProgressUpdated(m_measurementProgress);
        emit systemStatusChanged();
        if (progressCb) {
            progressCb(percent, msg);
        }
    };

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

        wrappedCb(5.0f, "Initializing session details...");

        // 2. Calibration Lookup & Loading
        std::string calFile = profile.calibrationFile;
        if (calFile.empty() && m_calManager) {
            calFile = m_calManager->getCalibrationFile(profile.measurementType, outSession.bandName);
        }
        outSession.calibrationFile = calFile;

        if (!calFile.empty() && m_calManager) {
            wrappedCb(10.0f, "Checking calibration file: " + calFile);
            
            // Note: Calibration files reside inside VNA flash memory.
            // Local path validation is a utility helper; load to VNA directly.
            Log::info("MeasurementController: Command VNA to load calibration: " + calFile);
            if (!m_vna->loadCalibrationState(calFile)) {
                Log::warn("MeasurementController: VNA calibration load returned warning. Continuing sweep.");
            }
        }

        // 3. VNA Configuration
        wrappedCb(20.0f, "Configuring VNA settings...");
        
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
            m_currentAngle = usePos ? profile.positioner.startAngleDeg : 0.0f;
            m_currentFrequency = profile.startFrequencyHz;
            wrappedCb(40.0f, "Triggering static sweep...");
            
            if (usePos) {
                m_positioner->moveTo(profile.positioner.startAngleDeg);
                m_positioner->waitForSettle(profile.positioner.settleTimeMs);
            }

            if (!m_vna->triggerSweep()) {
                throw std::runtime_error("VNA static sweep trigger failed.");
            }

            wrappedCb(70.0f, "Reading sweep traces...");
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
                    emit measurementCancelled();
                    emit systemStatusChanged();
                    return false;
                }

                m_currentAngle = currAngle;
                m_currentFrequency = profile.startFrequencyHz;
                m_estimatedRemainingTime = (stepsCount - currentStep) * (profile.positioner.settleTimeMs / 1000.0);

                float percent = 30.0f + 60.0f * (static_cast<float>(currentStep) / stepsCount);
                std::stringstream status;
                status << "Scanning angle: " << std::fixed << std::setprecision(1) << currAngle << " deg...";
                wrappedCb(percent, status.str());

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
                wrappedCb(92.0f, "Homing turntable positioner...");
                m_positioner->home();
                m_positioner->waitForSettle(5000);
            }
        }

        // 5. Data Logging
        wrappedCb(95.0f, "Saving sweep results to disk...");
        std::string loggedDir;
        if (m_logger) {
            if (!m_logger->logSession(outSession, loggedDir)) {
                Log::error("MeasurementController: Session logging failed.");
            }
            outSession.outputFolder = loggedDir;
        }

        // 6. Report Generation
        wrappedCb(98.0f, "Generating analysis reports...");
        if (!loggedDir.empty()) {
            auto mdReport = ReportGenerator::generateReport(outSession, ReportFormat::Markdown);
            ReportGenerator::saveReportToFile(mdReport, loggedDir + "/measurement_report.md");

            auto htmlReport = ReportGenerator::generateReport(outSession, ReportFormat::HTML);
            ReportGenerator::saveReportToFile(htmlReport, loggedDir + "/measurement_report.html");
        }

        wrappedCb(100.0f, "Session completed successfully!");
        m_latestSession = outSession;
        emit measurementFinished();
        emit measurementSessionChanged();
        emit systemStatusChanged();
        return true;

    } catch (const std::exception& ex) {
        Log::error("MeasurementController: Exception caught during sweep: " + std::string(ex.what()));
        wrappedCb(100.0f, "Sweep aborted: " + std::string(ex.what()));
        emit measurementCancelled();
        emit systemStatusChanged();
        return false;
    }
}

} // namespace AMAS
