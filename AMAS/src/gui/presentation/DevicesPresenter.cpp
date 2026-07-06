#include "DevicesPresenter.h"
#include "MeasurementPresenter.h"
#include "Logger.h"

namespace AMAS {

DevicesPresenter::DevicesPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
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

} // namespace AMAS
