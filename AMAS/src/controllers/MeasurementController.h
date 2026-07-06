#ifndef AMAS_MEASUREMENTCONTROLLER_H
#define AMAS_MEASUREMENTCONTROLLER_H

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <vector>
#include <QObject>
#include "IVnaDriver.h"
#include "IPositionerDriver.h"
#include "measurement/CalibrationManager.h"
#include "logger/DataLogger.h"
#include "measurement/MeasurementSession.h"
#include "measurement/MeasurementProfile.h"

namespace AMAS {

using ProgressCallback = std::function<void(float progressPercent, const std::string& statusMessage)>;

class MeasurementController : public QObject {
    Q_OBJECT
public:
    MeasurementController(std::shared_ptr<IVnaDriver> vna,
                          std::shared_ptr<IPositionerDriver> positioner,
                          std::shared_ptr<CalibrationManager> calManager,
                          std::shared_ptr<DataLogger> logger,
                          QObject* parent = nullptr);
    ~MeasurementController();

    MeasurementController(const MeasurementController&) = delete;
    MeasurementController& operator=(const MeasurementController&) = delete;

    // Connect both instruments
    bool connectHardware(const std::string& vnaResource, const std::string& posPort);

    // Disconnect both instruments
    void disconnectHardware();

    // Connection checks
    bool isVnaConnected() const;
    bool isPositionerConnected() const;

    // Device information placeholders
    std::string getVnaIdn() const;
    std::string getPositionerIdn() const;
    std::string getVnaResource() const;
    std::string getPositionerPort() const;

    // Orchestrate the measurement session
    bool runMeasurement(const MeasurementProfile& profile,
                        MeasurementSession& outSession,
                        ProgressCallback progressCb = nullptr);

    // Cancel coordinated sweeps
    void requestCancellation();
    bool isCancellationRequested() const;

    // Profile Management
    std::vector<MeasurementProfile> getProfiles() const;
    MeasurementProfile getActiveProfile() const;
    void setActiveProfile(const MeasurementProfile& profile);
    bool saveProfile(const MeasurementProfile& profile);
    bool deleteProfile(const std::string& profileName);
    MeasurementSession getLatestSession() const;

    // Live progress state getters
    std::string getMeasurementStatus() const;
    int getMeasurementProgress() const;
    double getCurrentFrequency() const;
    float getCurrentAngle() const;
    double getEstimatedRemainingTime() const;

signals:
    void measurementStarted();
    void measurementFinished();
    void measurementProgressUpdated(int progress);
    void measurementCancelled();
    void deviceConnected();
    void deviceDisconnected();
    void profileLoaded();
    void profileSaved();
    void profileDeleted();
    void measurementSessionChanged();
    void systemStatusChanged();

private:
    std::shared_ptr<IVnaDriver> m_vna;
    std::shared_ptr<IPositionerDriver> m_positioner;
    std::shared_ptr<CalibrationManager> m_calManager;
    std::shared_ptr<DataLogger> m_logger;

    bool m_vnaConnected;
    bool m_posConnected;
    std::atomic<bool> m_cancelRequested;

    // Profile and Session Data Cache
    std::vector<MeasurementProfile> m_profiles;
    MeasurementProfile m_activeProfile;
    MeasurementSession m_latestSession;

    // Live progress state
    std::string m_measurementStatus;
    int m_measurementProgress;
    double m_currentFrequency;
    float m_currentAngle;
    double m_estimatedRemainingTime;
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTCONTROLLER_H
