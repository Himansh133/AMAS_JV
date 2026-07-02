#ifndef AMAS_MEASUREMENTPRESENTER_H
#define AMAS_MEASUREMENTPRESENTER_H

#include <QObject>
#include <memory>
#include "controllers/MeasurementController.h"
#include "measurement/MeasurementProfile.h"

namespace AMAS {

class DashboardPresenter;
class DevicesPresenter;
class SetupPresenter;
class ProgressPresenter;
class ResultsPresenter;
class ProfilePresenter;

class MeasurementPresenter : public QObject {
    Q_OBJECT
public:
    explicit MeasurementPresenter(std::shared_ptr<MeasurementController> controller, QObject *parent = nullptr);
    ~MeasurementPresenter() override;

    // Access to controller
    std::shared_ptr<MeasurementController> controller() const { return m_controller; }

    // Sub-presenter instances (lazy initialized)
    DashboardPresenter* dashboard() const;
    DevicesPresenter*   devices() const;
    SetupPresenter*     setup() const;
    ProgressPresenter*  progress() const;
    ResultsPresenter*   results() const;
    ProfilePresenter*   profiles() const;

    // Shared UI state
    MeasurementProfile currentProfile() const;
    void setCurrentProfile(const MeasurementProfile &profile);

    QString activeCalibrationFile() const;
    void setActiveCalibrationFile(const QString &calFile);

    bool isSystemReady() const;
    QString systemState() const; // "Idle", "Running", "Paused"
    void setSystemState(const QString &state);

signals:
    void stateChanged();
    void currentProfileChanged(const QString &profileName);
    void deviceConnectionChanged();

private:
    std::shared_ptr<MeasurementController> m_controller;

    // Sub-presenters
    mutable DashboardPresenter* m_dashboardPresenter;
    mutable DevicesPresenter*   m_devicesPresenter;
    mutable SetupPresenter*     m_setupPresenter;
    mutable ProgressPresenter*  m_progressPresenter;
    mutable ResultsPresenter*   m_resultsPresenter;
    mutable ProfilePresenter*   m_profilePresenter;

    // Shared State
    MeasurementProfile m_currentProfile;
    QString m_activeCalibration;
    QString m_systemState; // "Idle", "Running", "Paused"
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTPRESENTER_H
