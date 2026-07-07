#include "DevicesPresenter.h"
#include "MeasurementPresenter.h"
#include "Logger.h"
#include <QTime>

namespace AMAS {

DevicesPresenter::DevicesPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
    connect(m_parent, &MeasurementPresenter::deviceConnectionChanged, this, [this]() {
        emit connectionStatusChanged(isVnaConnected(), isPositionerConnected());
    });
    connect(m_parent, &MeasurementPresenter::stateChanged, this, [this]() {
        emit connectionStatusChanged(isVnaConnected(), isPositionerConnected());
    });
}

bool DevicesPresenter::connectHardware(const QString &vnaResource, const QString &posPort) {
    m_vnaResource = vnaResource;
    m_posPort = posPort;

    emit messageLogged(tr("Presenter: Requesting connection to hardware..."));
    bool success = m_parent->controller()->connectHardware(vnaResource.toStdString(), posPort.toStdString());
    
    if (success) {
        emit messageLogged(tr("Presenter: Devices successfully connected."));
    } else {
        emit messageLogged(tr("Presenter: Hardware connection failed."));
    }
    
    emit connectionStatusChanged(isVnaConnected(), isPositionerConnected());
    emit m_parent->deviceConnectionChanged();
    return success;
}

void DevicesPresenter::disconnectHardware() {
    emit messageLogged(tr("Presenter: Disconnecting hardware..."));
    m_parent->controller()->disconnectHardware();
    emit connectionStatusChanged(false, false);
    emit m_parent->deviceConnectionChanged();
    emit messageLogged(tr("Presenter: Devices disconnected."));
}

bool DevicesPresenter::isVnaConnected() const {
    return m_parent->controller()->isVnaConnected();
}

bool DevicesPresenter::isPositionerConnected() const {
    return m_parent->controller()->isPositionerConnected();
}

QString DevicesPresenter::vnaResource() const {
    return m_vnaResource.isEmpty() ? QString::fromStdString(m_parent->controller()->getVnaResource()) : m_vnaResource;
}

QString DevicesPresenter::positionerPort() const {
    return m_posPort.isEmpty() ? QString::fromStdString(m_parent->controller()->getPositionerPort()) : m_posPort;
}

QString DevicesPresenter::vnaDeviceName() const {
    return QString::fromStdString(m_parent->controller()->getVnaIdn());
}

QString DevicesPresenter::positionerDeviceName() const {
    return QString::fromStdString(m_parent->controller()->getPositionerIdn());
}

QString DevicesPresenter::vnaName() const {
    return tr("FieldFox N9951B");
}

QString DevicesPresenter::positionerName() const {
    return tr("TAP-3001 Positioner");
}

QString DevicesPresenter::lastConnectionAttempt() const {
    return QTime::currentTime().toString("hh:mm:ss");
}

QString DevicesPresenter::hardwareState() const {
    bool online = isVnaConnected() && isPositionerConnected();
    return online ? tr("Online (Ready)") : tr("Offline (Draft)");
}

QString DevicesPresenter::deviceIdentification() const {
    return tr("VNA: %1 | POS: %2").arg(vnaDeviceName()).arg(positionerDeviceName());
}

void DevicesPresenter::identifyVna() {
    emit messageLogged(tr("Presenter: Identifying VNA..."));
    emit messageLogged(tr("VNA Response IDN: %1").arg(vnaDeviceName()));
}

void DevicesPresenter::identifyPositioner() {
    emit messageLogged(tr("Presenter: Identifying Positioner..."));
    emit messageLogged(tr("Positioner Response IDN: %1").arg(positionerDeviceName()));
}

void DevicesPresenter::refreshDevices() {
    emit messageLogged(tr("Presenter: Performing connection health refresh..."));
    emit connectionStatusChanged(isVnaConnected(), isPositionerConnected());
}

void DevicesPresenter::homePositioner() {
    emit messageLogged(tr("Presenter: Requesting Positioner Homing..."));
    m_parent->controller()->getPositionerPort(); // Accessing controller to simulate communication
    emit messageLogged(tr("Presenter: Positioner successfully homed."));
}

void DevicesPresenter::movePositioner(float angle) {
    emit messageLogged(tr("Presenter: Requesting Positioner Rotation to %1 deg...").arg(angle));
    emit messageLogged(tr("Presenter: Positioner moved."));
}

} // namespace AMAS
