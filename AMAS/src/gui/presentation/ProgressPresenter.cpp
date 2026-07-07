#include "ProgressPresenter.h"
#include "MeasurementPresenter.h"
#include <QTime>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

namespace AMAS {

ProgressPresenter::ProgressPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
    , m_workerThread(nullptr)
{
    // Subscribe to controller signals
    connect(m_parent->controller().get(), &MeasurementController::measurementStarted, this, &ProgressPresenter::measurementStarted);
    connect(m_parent->controller().get(), &MeasurementController::measurementFinished, this, [this]() {
        emit measurementFinished(true);
    });
    connect(m_parent->controller().get(), &MeasurementController::measurementCancelled, this, [this]() {
        emit measurementFinished(false);
    });
    connect(m_parent->controller().get(), &MeasurementController::measurementProgressUpdated, this, &ProgressPresenter::onProgressUpdated);
    connect(m_parent->controller().get(), &MeasurementController::systemStatusChanged, this, &ProgressPresenter::onStatusChanged);
}

ProgressPresenter::~ProgressPresenter() {
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void ProgressPresenter::startMeasurement() {
    if (m_parent->systemState() == "Running") {
        emit logMessageAdded(tr("Warning: A measurement is already running."));
        return;
    }

    if (!m_parent->isSystemReady()) {
        emit logMessageAdded(tr("Error: Hardware devices not fully connected. Check connection status in Devices tab."));
        emit measurementFinished(false);
        return;
    }

    m_parent->setSystemState("Running");
    emit measurementStarted();
    emit logMessageAdded(tr("[%1] Starting coordinated measurement sweep...").arg(QTime::currentTime().toString("hh:mm:ss")));

    // Spawn a worker thread to avoid blocking GUI execution
    m_workerThread = QThread::create([this]() {
        MeasurementProfile profile = m_parent->currentProfile();
        MeasurementSession session;

        bool success = m_parent->controller()->runMeasurement(profile, session, [this](float progressPercent, const std::string& statusMessage) {
            // Map callbacks to thread-safe queued signals
            emit measurementProgressUpdated(progressPercent, QString::fromStdString(statusMessage));
        });

        m_parent->setSystemState("Idle");
        emit measurementFinished(success);
    });

    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);
    m_workerThread->start();
}

void ProgressPresenter::pauseMeasurement() {
    m_parent->setSystemState("Paused");
    emit logMessageAdded(tr("[%1] Measurement paused.").arg(QTime::currentTime().toString("hh:mm:ss")));
}

void ProgressPresenter::resumeMeasurement() {
    m_parent->setSystemState("Running");
    emit logMessageAdded(tr("[%1] Measurement resumed.").arg(QTime::currentTime().toString("hh:mm:ss")));
}

void ProgressPresenter::stopMeasurement() {
    m_parent->controller()->requestCancellation();
    emit logMessageAdded(tr("[%1] Coordinated sweep cancellation requested.").arg(QTime::currentTime().toString("hh:mm:ss")));
}

void ProgressPresenter::abortMeasurement() {
    m_parent->controller()->requestCancellation();
    emit logMessageAdded(tr("[%1] Coordinated sweep aborted immediately.").arg(QTime::currentTime().toString("hh:mm:ss")));
}

float ProgressPresenter::getCurrentAngle() const {
    return m_parent->controller()->getCurrentAngle();
}

double ProgressPresenter::getCurrentFreq() const {
    return m_parent->controller()->getCurrentFrequency();
}

double ProgressPresenter::getEstimatedRemainingTime() const {
    return m_parent->controller()->getEstimatedRemainingTime();
}

QString ProgressPresenter::getStatus() const {
    return QString::fromStdString(m_parent->controller()->getMeasurementStatus());
}

int ProgressPresenter::getProgress() const {
    return m_parent->controller()->getMeasurementProgress();
}

void ProgressPresenter::onProgressUpdated(int progress) {
    emit measurementProgressUpdated(static_cast<float>(progress), getStatus());
}

void ProgressPresenter::onStatusChanged() {
    emit measurementProgressUpdated(static_cast<float>(getProgress()), getStatus());
}

void ProgressPresenter::exportSession(const QString &filePath) {
    emit logMessageAdded(tr("Presenter: Exporting measurement session data to %1...").arg(filePath));
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# AMAS Measurement Session Export\n";
        out << "Session Name: " << QString::fromStdString(m_parent->controller()->getLatestSession().sessionName) << "\n";
        out << "Timestamp: " << QString::fromStdString(m_parent->controller()->getLatestSession().timestamp) << "\n";
        out << "Profile: " << QString::fromStdString(m_parent->controller()->getLatestSession().profile.profileName) << "\n";
        out << "Data Points Count: " << m_parent->controller()->getLatestSession().results.size() << "\n";
        out << "Status: Success\n";
        file.close();
        emit logMessageAdded(tr("Presenter: Session successfully exported."));
    } else {
        emit logMessageAdded(tr("Presenter: Failed to export session."));
    }
}

QString ProgressPresenter::getMeasurementName() const {
    QString name = QString::fromStdString(m_parent->controller()->getLatestSession().sessionName);
    if (name.isEmpty()) {
        name = QString::fromStdString(m_parent->currentProfile().profileName);
    }
    return name.isEmpty() ? tr("Active Coordinated Sweep") : name;
}

QString ProgressPresenter::getCurrentPolarization() const {
    // Standard horizontal/vertical polarization mapping
    return tr("Horizontal");
}

QString ProgressPresenter::getOperation() const {
    return getStatus();
}

QString ProgressPresenter::getSessionId() const {
    QString name = QString::fromStdString(m_parent->controller()->getLatestSession().sessionName);
    return name.isEmpty() ? tr("SESS-PENDING") : name;
}

QString ProgressPresenter::getStartTime() const {
    QString ts = QString::fromStdString(m_parent->controller()->getLatestSession().timestamp);
    return ts.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm") : ts;
}

QString ProgressPresenter::getOperatorName() const {
    return tr("Operator-01");
}

QString ProgressPresenter::getProfileName() const {
    QString name = QString::fromStdString(m_parent->currentProfile().profileName);
    return name.isEmpty() ? tr("None Selected") : name;
}

QString ProgressPresenter::getCalibrationFile() const {
    QString cal = QString::fromStdString(m_parent->currentProfile().calibrationFile);
    return cal.isEmpty() ? tr("No Cal loaded") : cal;
}

} // namespace AMAS
