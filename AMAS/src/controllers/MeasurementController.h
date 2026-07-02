#ifndef AMAS_MEASUREMENTCONTROLLER_H
#define AMAS_MEASUREMENTCONTROLLER_H

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include "IVnaDriver.h"
#include "IPositionerDriver.h"
#include "measurement/CalibrationManager.h"
#include "logger/DataLogger.h"
#include "measurement/MeasurementSession.h"
#include "measurement/MeasurementProfile.h"

namespace AMAS {

// Callback signature for reporting sweep progress to the UI
using ProgressCallback = std::function<void(float progressPercent, const std::string& statusMessage)>;

class MeasurementController {
public:
    MeasurementController(std::shared_ptr<IVnaDriver> vna,
                          std::shared_ptr<IPositionerDriver> positioner,
                          std::shared_ptr<CalibrationManager> calManager,
                          std::shared_ptr<DataLogger> logger);
    ~MeasurementController();

    // Disable copy constructors to avoid duplicate references to open hardware ports
    MeasurementController(const MeasurementController&) = delete;
    MeasurementController& operator=(const MeasurementController&) = delete;

    // Connect both instruments. Returns true if successful.
    bool connectHardware(const std::string& vnaResource, const std::string& posPort);

    // Disconnect both instruments
    void disconnectHardware();

    // Connection checks
    bool isVnaConnected() const;
    bool isPositionerConnected() const;

    // Orchestrate the measurement session
    bool runMeasurement(const MeasurementProfile& profile,
                        MeasurementSession& outSession,
                        ProgressCallback progressCb = nullptr);

    // Cancel an in-progress coordinated sweep
    void requestCancellation();
    bool isCancellationRequested() const;

private:
    std::shared_ptr<IVnaDriver> m_vna;
    std::shared_ptr<IPositionerDriver> m_positioner;
    std::shared_ptr<CalibrationManager> m_calManager;
    std::shared_ptr<DataLogger> m_logger;

    bool m_vnaConnected;
    bool m_posConnected;
    std::atomic<bool> m_cancelRequested;
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTCONTROLLER_H
