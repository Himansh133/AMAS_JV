#include "ProgressPresenter.h"
#include "MeasurementPresenter.h"
#include <QTime>

namespace AMAS {

ProgressPresenter::ProgressPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
    , m_workerThread(nullptr)
{
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

} // namespace AMAS
