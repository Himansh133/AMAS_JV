#include "DashboardPresenter.h"
#include "MeasurementPresenter.h"

namespace AMAS {

DashboardPresenter::DashboardPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
    connect(m_parent, &MeasurementPresenter::stateChanged, this, &DashboardPresenter::dashboardUpdated);
    connect(m_parent, &MeasurementPresenter::currentProfileChanged, this, &DashboardPresenter::dashboardUpdated);
    connect(m_parent, &MeasurementPresenter::deviceConnectionChanged, this, &DashboardPresenter::dashboardUpdated);
}

QString DashboardPresenter::currentProfileName() const {
    QString name = QString::fromStdString(m_parent->currentProfile().profileName);
    return name.isEmpty() ? tr("None Selected") : name;
}

QString DashboardPresenter::calibrationStatus() const {
    QString cal = m_parent->activeCalibrationFile();
    return cal.isEmpty() ? tr("No Calibration Loaded") : cal;
}

QString DashboardPresenter::systemState() const {
    return m_parent->systemState();
}

bool DashboardPresenter::isSystemReady() const {
    return m_parent->isSystemReady();
}

bool DashboardPresenter::isVnaConnected() const {
    return m_parent->controller()->isVnaConnected();
}

bool DashboardPresenter::isPositionerConnected() const {
    return m_parent->controller()->isPositionerConnected();
}

QString DashboardPresenter::lastMeasurement() const {
    QString name = QString::fromStdString(m_parent->controller()->getLatestSession().sessionName);
    return name.isEmpty() ? tr("None") : name;
}

QString DashboardPresenter::applicationVersion() const {
    return "1.0.0";
}

QString DashboardPresenter::activeSession() const {
    QString name = QString::fromStdString(m_parent->controller()->getLatestSession().sessionName);
    return name.isEmpty() ? tr("No Active Session") : name;
}

QString DashboardPresenter::lastMeasurementTime() const {
    QString ts = QString::fromStdString(m_parent->controller()->getLatestSession().timestamp);
    return ts.isEmpty() ? tr("N/A") : ts;
}

int DashboardPresenter::connectedDeviceCount() const {
    int count = 0;
    if (m_parent->controller()->isVnaConnected()) count++;
    if (m_parent->controller()->isPositionerConnected()) count++;
    return count;
}

QString DashboardPresenter::vnaResource() const {
    std::string res = m_parent->controller()->getVnaResource();
    return res.empty() ? tr("Offline") : QString::fromStdString(res);
}

QString DashboardPresenter::vnaFirmware() const {
    if (m_parent->controller()->isVnaConnected()) {
        return "A.10.15";
    }
    return tr("N/A");
}

QString DashboardPresenter::positionerPort() const {
    std::string port = m_parent->controller()->getPositionerPort();
    return port.empty() ? tr("Offline") : QString::fromStdString(port);
}

QString DashboardPresenter::positionerSlaveId() const {
    if (m_parent->controller()->isPositionerConnected()) {
        return "1";
    }
    return tr("N/A");
}

int DashboardPresenter::currentProfilePoints() const {
    return m_parent->currentProfile().sweepPoints;
}

} // namespace AMAS
