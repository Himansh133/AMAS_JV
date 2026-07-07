#ifndef AMAS_DASHBOARDPRESENTER_H
#define AMAS_DASHBOARDPRESENTER_H

#include <QObject>

namespace AMAS {

class MeasurementPresenter;

class DashboardPresenter : public QObject {
    Q_OBJECT
public:
    explicit DashboardPresenter(MeasurementPresenter *parent);
    ~DashboardPresenter() override = default;

    QString currentProfileName() const;
    QString calibrationStatus() const;
    QString systemState() const;
    bool isSystemReady() const;
    bool isVnaConnected() const;
    bool isPositionerConnected() const;
    QString lastMeasurement() const;
    QString applicationVersion() const;

    QString activeSession() const;
    QString lastMeasurementTime() const;
    int connectedDeviceCount() const;
    QString vnaResource() const;
    QString vnaFirmware() const;
    QString positionerPort() const;
    QString positionerSlaveId() const;
    int currentProfilePoints() const;

signals:
    void dashboardUpdated();

private:
    MeasurementPresenter *m_parent;
};

} // namespace AMAS

#endif // AMAS_DASHBOARDPRESENTER_H
