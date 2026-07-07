#ifndef AMAS_DEVICESPRESENTER_H
#define AMAS_DEVICESPRESENTER_H

#include <QObject>

namespace AMAS {

class MeasurementPresenter;

class DevicesPresenter : public QObject {
    Q_OBJECT
public:
    explicit DevicesPresenter(MeasurementPresenter *parent);
    ~DevicesPresenter() override = default;

    // Actions
    bool connectHardware(const QString &vnaResource, const QString &posPort);
    void disconnectHardware();

    // Query states
    bool isVnaConnected() const;
    bool isPositionerConnected() const;
    QString vnaResource() const;
    QString positionerPort() const;
    QString vnaDeviceName() const;
    QString positionerDeviceName() const;

    QString vnaName() const;
    QString positionerName() const;
    QString lastConnectionAttempt() const;
    QString hardwareState() const;
    QString deviceIdentification() const;

    void identifyVna();
    void identifyPositioner();
    void refreshDevices();
    void homePositioner();
    void movePositioner(float angle);

signals:
    void connectionStatusChanged(bool vnaConnected, bool posConnected);
    void messageLogged(const QString &message);

private:
    MeasurementPresenter *m_parent;
    QString m_vnaResource;
    QString m_posPort;
};

} // namespace AMAS

#endif // AMAS_DEVICESPRESENTER_H
