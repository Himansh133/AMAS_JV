#include "MeasurementPresenter.h"
#include "DashboardPresenter.h"
#include "DevicesPresenter.h"
#include "SetupPresenter.h"
#include "ProgressPresenter.h"
#include "ResultsPresenter.h"
#include "ProfilePresenter.h"

namespace AMAS {

MeasurementPresenter::MeasurementPresenter(std::shared_ptr<MeasurementController> controller, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_dashboardPresenter(nullptr)
    , m_devicesPresenter(nullptr)
    , m_setupPresenter(nullptr)
    , m_progressPresenter(nullptr)
    , m_resultsPresenter(nullptr)
    , m_profilePresenter(nullptr)
    , m_systemState("Idle")
    , m_undoStack(new QUndoStack(this))
{
    // Establish Controller Signal connections
    connect(m_controller.get(), &MeasurementController::deviceConnected, this, &MeasurementPresenter::deviceConnectionChanged);
    connect(m_controller.get(), &MeasurementController::deviceDisconnected, this, &MeasurementPresenter::deviceConnectionChanged);
    connect(m_controller.get(), &MeasurementController::systemStatusChanged, this, &MeasurementPresenter::stateChanged);
    connect(m_controller.get(), &MeasurementController::profileLoaded, this, [this]() {
        m_currentProfile = m_controller->getActiveProfile();
        m_activeCalibration = QString::fromStdString(m_currentProfile.calibrationFile);
        emit currentProfileChanged(QString::fromStdString(m_currentProfile.profileName));
        emit stateChanged();
    });
    connect(m_controller.get(), &MeasurementController::profileSaved, this, &MeasurementPresenter::stateChanged);
    connect(m_controller.get(), &MeasurementController::profileDeleted, this, &MeasurementPresenter::stateChanged);
    connect(m_controller.get(), &MeasurementController::measurementSessionChanged, this, &MeasurementPresenter::stateChanged);
}

MeasurementPresenter::~MeasurementPresenter() {
    // Parent QObject deletes all child sub-presenter allocations automatically
}

DashboardPresenter* MeasurementPresenter::dashboard() const {
    if (!m_dashboardPresenter) {
        m_dashboardPresenter = new DashboardPresenter(const_cast<MeasurementPresenter*>(this));
    }
    return m_dashboardPresenter;
}

DevicesPresenter* MeasurementPresenter::devices() const {
    if (!m_devicesPresenter) {
        m_devicesPresenter = new DevicesPresenter(const_cast<MeasurementPresenter*>(this));
    }
    return m_devicesPresenter;
}

SetupPresenter* MeasurementPresenter::setup() const {
    if (!m_setupPresenter) {
        m_setupPresenter = new SetupPresenter(const_cast<MeasurementPresenter*>(this));
    }
    return m_setupPresenter;
}

ProgressPresenter* MeasurementPresenter::progress() const {
    if (!m_progressPresenter) {
        m_progressPresenter = new ProgressPresenter(const_cast<MeasurementPresenter*>(this));
    }
    return m_progressPresenter;
}

ResultsPresenter* MeasurementPresenter::results() const {
    if (!m_resultsPresenter) {
        m_resultsPresenter = new ResultsPresenter(const_cast<MeasurementPresenter*>(this));
    }
    return m_resultsPresenter;
}

ProfilePresenter* MeasurementPresenter::profiles() const {
    if (!m_profilePresenter) {
        m_profilePresenter = new ProfilePresenter(const_cast<MeasurementPresenter*>(this));
    }
    return m_profilePresenter;
}

MeasurementProfile MeasurementPresenter::currentProfile() const {
    return m_currentProfile;
}

void MeasurementPresenter::setCurrentProfile(const MeasurementProfile &profile) {
    m_currentProfile = profile;
    emit currentProfileChanged(QString::fromStdString(profile.profileName));
}

QString MeasurementPresenter::activeCalibrationFile() const {
    return m_activeCalibration;
}

void MeasurementPresenter::setActiveCalibrationFile(const QString &calFile) {
    m_activeCalibration = calFile;
    emit stateChanged();
}

bool MeasurementPresenter::isSystemReady() const {
    return m_controller->isVnaConnected() && m_controller->isPositionerConnected();
}

QString MeasurementPresenter::systemState() const {
    return m_systemState;
}

void MeasurementPresenter::setSystemState(const QString &state) {
    m_systemState = state;
    emit stateChanged();
}

QUndoStack* MeasurementPresenter::undoStack() const {
    return m_undoStack;
}

} // namespace AMAS
