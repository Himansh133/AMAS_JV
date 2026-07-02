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

} // namespace AMAS
