#include "MeasurementProgressPage.h"
#include "presentation/ProgressPresenter.h"
#include <QTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QStyle>
#include <QScrollBar>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>

namespace AMAS {

MeasurementProgressPage::MeasurementProgressPage(ProgressPresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_presenter(presenter)
{
    // 1. Root main layout (holds scroll area)
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 2. Create the Scroll Area
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");
    mainLayout->addWidget(scrollArea);

    // 3. Scrollable content widget
    auto *scrollContent = new QWidget(scrollArea);
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(24, 24, 24, 24);
    scrollLayout->setSpacing(20);

    // 4. Header block
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);

    auto *lblTitle = new QLabel(tr("Measurement Progress"), scrollContent);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(lblTitle);

    auto *lblSubtitle = new QLabel(tr("Monitor instrument channels, angular positions, and status of the sweep in real-time."), scrollContent);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #C8C8C8;");
    headerLayout->addWidget(lblSubtitle);
    scrollLayout->addLayout(headerLayout);

    // 5. Top Metadata Bar (Custom container widget)
    auto *infoBar = new QWidget(scrollContent);
    infoBar->setStyleSheet("background-color: #2D2D30; border: 1px solid #3F3F46; border-radius: 6px;");
    createTopInfoBar(infoBar);
    scrollLayout->addWidget(infoBar);

    // 6. Progress and Status Panels
    auto *grpProgress = new QGroupBox(tr("Active Coordinated Progress"), scrollContent);
    createProgressPanel(grpProgress);
    scrollLayout->addWidget(grpProgress);

    auto *grpInstrument = new QGroupBox(tr("Instrument Tracking Channels"), scrollContent);
    createInstrumentStatusPanel(grpInstrument);
    scrollLayout->addWidget(grpInstrument);

    // Two-Column Split: Stats (Left) and Log (Right)
    auto *splitLayout = new QHBoxLayout();
    splitLayout->setSpacing(20);

    auto *grpStats = new QGroupBox(tr("Session Accumulation Statistics"), scrollContent);
    createStatisticsPanel(grpStats);
    splitLayout->addWidget(grpStats, 2);

    auto *grpLog = new QGroupBox(tr("Live Diagnostics Log"), scrollContent);
    createLiveLogPanel(grpLog);
    splitLayout->addWidget(grpLog, 3);

    scrollLayout->addLayout(splitLayout);

    // 7. Emergency Controls layout
    auto *bottomWidget = new QWidget(scrollContent);
    bottomWidget->setMinimumHeight(70);
    auto *bottomLayout = new QHBoxLayout(bottomWidget);
    createEmergencyControls(bottomLayout);
    scrollLayout->addWidget(bottomWidget);

    scrollArea->setWidget(scrollContent);

    // Bind presenter slots
    connect(m_btnPause, &QPushButton::clicked, this, &MeasurementProgressPage::onPauseClicked);
    connect(m_btnResume, &QPushButton::clicked, this, &MeasurementProgressPage::onResumeClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &MeasurementProgressPage::onStopClicked);
    connect(m_btnAbort, &QPushButton::clicked, this, &MeasurementProgressPage::onAbortClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &MeasurementProgressPage::onExportClicked);

    connect(m_presenter, &ProgressPresenter::measurementStarted, this, &MeasurementProgressPage::onStartClicked);
    connect(m_presenter, &ProgressPresenter::measurementProgressUpdated, this, &MeasurementProgressPage::updateProgress);
    connect(m_presenter, &ProgressPresenter::measurementFinished, this, &MeasurementProgressPage::handleMeasurementFinished);
    connect(m_presenter, &ProgressPresenter::logMessageAdded, this, &MeasurementProgressPage::appendLogMessage);
}

void MeasurementProgressPage::createTopInfoBar(QWidget *container) {
    auto *layout = new QGridLayout(container);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    auto addMetadataField = [container, layout](int row, int col, const QString &label, QLabel *&valLabel, const QString &defaultVal) {
        auto *lblTag = new QLabel(label, container);
        lblTag->setStyleSheet("color: #C8C8C8; font-weight: 500; border: none; background: transparent;");
        valLabel = new QLabel(defaultVal, container);
        valLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; border: none; background: transparent;");
        
        layout->addWidget(lblTag, row, col);
        layout->addWidget(valLabel, row, col + 1);
    };

    addMetadataField(0, 0, tr("Measurement Type:"), m_lblMeasType, tr("Radiation Pattern (2D)"));
    addMetadataField(0, 2, tr("Operator:"), m_lblOperator, tr("Operator-01"));
    addMetadataField(0, 4, tr("Session ID:"), m_lblSessionId, tr("SESS-2026-0701"));

    addMetadataField(1, 0, tr("Profile Name:"), m_lblProfile, tr("8-12 GHz Sweep"));
    addMetadataField(1, 2, tr("Calibration:"), m_lblCal, tr("Loaded"));
    addMetadataField(1, 4, tr("Start Time:"), m_lblStartTime, tr("2026-07-01 14:00"));

    // Set grid stretching
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);
    layout->setColumnStretch(5, 1);
}

void MeasurementProgressPage::createProgressPanel(QGroupBox *box) {
    auto *layout = new QFormLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_progressBar = new QProgressBar(box);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(68);
    m_progressBar->setTextVisible(true);
    m_progressBar->setStyleSheet(
        "QProgressBar { "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 4px; "
        "  text-align: center; "
        "  color: #FFFFFF; "
        "  font-weight: bold; "
        "  background-color: #1E1E1E; "
        "} "
        "QProgressBar::chunk { "
        "  background-color: #007ACC; "
        "  width: 1px; "
        "} "
    );
    layout->addRow(tr("Overall Progress:"), m_progressBar);

    auto *gridStats = new QGridLayout();
    gridStats->setSpacing(12);

    auto addProgressLabel = [box, gridStats](int r, int c, const QString &tag, QLabel *&lblRef, const QString &initText) {
        gridStats->addWidget(new QLabel(tag, box), r, c);
        lblRef = new QLabel(initText, box);
        lblRef->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        gridStats->addWidget(lblRef, r, c + 1);
    };

    addProgressLabel(0, 0, tr("Current Angle:"), m_lblCurrAngle, tr("90.0 \xc2\xb0"));
    addProgressLabel(0, 2, tr("Current Frequency:"), m_lblCurrFreq, tr("10.250 GHz"));
    addProgressLabel(1, 0, tr("Current Sweep:"), m_lblCurrSweep, tr("18 / 25"));
    addProgressLabel(1, 2, tr("Est. Remaining:"), m_lblEstRemaining, tr("00:04:15"));

    layout->addRow(QString(), gridStats);

    // Running Status line
    auto *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(8);
    auto *ind = new QLabel(box);
    ind->setFixedSize(10, 10);
    ind->setStyleSheet("border-radius: 5px; background-color: #4CAF50;"); // Green
    m_lblStatus = new QLabel(tr("Running"), box);
    m_lblStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    statusLayout->addWidget(ind);
    statusLayout->addWidget(m_lblStatus);
    statusLayout->addStretch();
    layout->addRow(tr("Status:"), statusLayout);
}

void MeasurementProgressPage::createInstrumentStatusPanel(QGroupBox *box) {
    auto *layout = new QGridLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // Left Column: VNA
    auto *vnaBox = new QWidget(box);
    auto *vnaLayout = new QFormLayout(vnaBox);
    vnaLayout->setSpacing(10);
    vnaLayout->setContentsMargins(0, 0, 0, 0);

    auto *vnaStatusRow = new QHBoxLayout();
    vnaStatusRow->setSpacing(6);
    auto *indVna = new QLabel(vnaBox);
    indVna->setFixedSize(10, 10);
    indVna->setStyleSheet("border-radius: 5px; background-color: #4CAF50;");
    m_lblVnaConn = new QLabel(tr("Connected"), vnaBox);
    m_lblVnaConn->setStyleSheet("color: #FFFFFF; font-weight: bold;");
    vnaStatusRow->addWidget(indVna);
    vnaStatusRow->addWidget(m_lblVnaConn);
    vnaStatusRow->addStretch();
    vnaLayout->addRow(tr("FieldFox VNA:"), vnaStatusRow);

    m_lblVnaFreq = new QLabel(tr("10.250 GHz"), vnaBox);
    m_lblVnaFreq->setStyleSheet("color: #FFFFFF;");
    vnaLayout->addRow(tr("Current Frequency:"), m_lblVnaFreq);

    m_lblVnaTrigger = new QLabel(tr("Ready"), vnaBox);
    m_lblVnaTrigger->setStyleSheet("color: #4CAF50; font-weight: bold;");
    vnaLayout->addRow(tr("Trigger Status:"), m_lblVnaTrigger);

    layout->addWidget(vnaBox, 0, 0);

    // Right Column: Positioner
    auto *posBox = new QWidget(box);
    auto *posLayout = new QFormLayout(posBox);
    posLayout->setSpacing(10);
    posLayout->setContentsMargins(0, 0, 0, 0);

    auto *posStatusRow = new QHBoxLayout();
    posStatusRow->setSpacing(6);
    auto *indPos = new QLabel(posBox);
    indPos->setFixedSize(10, 10);
    indPos->setStyleSheet("border-radius: 5px; background-color: #4CAF50;");
    m_lblPosConn = new QLabel(tr("Connected"), posBox);
    m_lblPosConn->setStyleSheet("color: #FFFFFF; font-weight: bold;");
    posStatusRow->addWidget(indPos);
    posStatusRow->addWidget(m_lblPosConn);
    posStatusRow->addStretch();
    posLayout->addRow(tr("TAP-3001:"), posStatusRow);

    auto *anglesLayout = new QHBoxLayout();
    anglesLayout->setSpacing(12);
    m_lblPosAz = new QLabel(tr("Az: 90.0 \xc2\xb0"), posBox);
    m_lblPosAz->setStyleSheet("color: #FFFFFF;");
    m_lblPosEl = new QLabel(tr("El: 0.0 \xc2\xb0"), posBox);
    m_lblPosEl->setStyleSheet("color: #FFFFFF;");
    m_lblPosPl = new QLabel(tr("Pl: 0.0 \xc2\xb0"), posBox);
    m_lblPosPl->setStyleSheet("color: #FFFFFF;");
    anglesLayout->addWidget(m_lblPosAz);
    anglesLayout->addWidget(m_lblPosEl);
    anglesLayout->addWidget(m_lblPosPl);
    anglesLayout->addStretch();
    posLayout->addRow(tr("Current Position:"), anglesLayout);

    m_lblPosMotion = new QLabel(tr("Idle"), posBox);
    m_lblPosMotion->setStyleSheet("color: #C8C8C8;");
    posLayout->addRow(tr("Motion Status:"), m_lblPosMotion);

    layout->addWidget(posBox, 0, 1);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
}

void MeasurementProgressPage::createStatisticsPanel(QGroupBox *box) {
    auto *layout = new QFormLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto addStatRow = [box, layout](const QString &tag, QLabel *&ref, const QString &val) {
        ref = new QLabel(val, box);
        ref->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        layout->addRow(tag, ref);
    };

    addStatRow(tr("Completed Points:"), m_lblStatCompleted, tr("7,218"));
    addStatRow(tr("Remaining Points:"), m_lblStatRemaining, tr("3,382"));
    addStatRow(tr("Total Measurements:"), m_lblStatTotal, tr("10,600"));
    addStatRow(tr("Avg. Measure Time:"), m_lblStatAvgTime, tr("1.25 seconds"));
    addStatRow(tr("Current Band:"), m_lblStatBand, tr("8-12 GHz"));
    addStatRow(tr("Active Data Size:"), m_lblStatSize, tr("248 KB"));
}

void MeasurementProgressPage::createLiveLogPanel(QGroupBox *box) {
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(12, 16, 12, 12);

    m_logEdit = new QTextEdit(box);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(150);
    m_logEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1E1E1E; "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 4px; "
        "  font-family: Consolas, 'Courier New', monospace; "
        "  font-size: 12px; "
        "  color: #C8C8C8; "
        "  line-height: 18px; "
        "} "
    );

    // Placeholders
    m_logEdit->append(tr("[14:00:00] [INFO] Coordinated Measurement started."));
    m_logEdit->append(tr("[14:00:01] [INFO] Loading calibration: calchamber8_12ghz.sta"));
    m_logEdit->append(tr("[14:00:05] [INFO] Moving turntable to start position: Az=0.0 deg."));
    m_logEdit->append(tr("[14:00:06] [INFO] Triggering VNA sweep channel."));
    m_logEdit->append(tr("[14:00:07] [INFO] Reading S-parameters. S11 data logged."));
    m_logEdit->append(tr("[14:00:10] [INFO] Moving to next position: Az=10.0 deg."));
    m_logEdit->append(tr("[14:00:11] [INFO] Triggering VNA sweep channel."));
    m_logEdit->append(tr("[14:00:12] [INFO] Reading S-parameters. S11 data logged."));

    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
    layout->addWidget(m_logEdit);
}

void MeasurementProgressPage::createEmergencyControls(QHBoxLayout *layout) {
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QWidget *parentWidget = layout->parentWidget();

    m_btnPause = new QPushButton(tr("Pause"), parentWidget);
    m_btnPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    m_btnPause->setMinimumHeight(32);
    m_btnPause->setMinimumWidth(140);

    m_btnResume = new QPushButton(tr("Resume"), parentWidget);
    m_btnResume->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_btnResume->setMinimumHeight(32);
    m_btnResume->setMinimumWidth(140);

    m_btnStop = new QPushButton(tr("Stop"), parentWidget);
    m_btnStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_btnStop->setMinimumHeight(32);
    m_btnStop->setMinimumWidth(140);
    m_btnStop->setStyleSheet(
        "QPushButton { "
        "  background-color: #A13838; " // Engineering stop red highlight
        "  border: 1px solid #3F3F46; "
        "  color: #FFFFFF; "
        "} "
        "QPushButton:hover { "
        "  background-color: #C03E3E; "
        "} "
    );

    m_btnAbort = new QPushButton(tr("Abort"), parentWidget);
    m_btnAbort->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    m_btnAbort->setMinimumHeight(32);
    m_btnAbort->setMinimumWidth(140);

    m_btnExport = new QPushButton(tr("Export Data"), parentWidget);
    m_btnExport->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnExport->setMinimumHeight(32);
    m_btnExport->setMinimumWidth(140);

    layout->addWidget(m_btnPause);
    layout->addWidget(m_btnResume);
    layout->addStretch();
    layout->addWidget(m_btnStop);
    layout->addWidget(m_btnAbort);
    layout->addWidget(m_btnExport);
}

void MeasurementProgressPage::onStartClicked() {
    m_progressBar->setValue(0);
    appendLogMessage(tr("Measurement sweep process initialized by presenter..."));
    m_btnPause->setEnabled(true);
    m_btnResume->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_btnAbort->setEnabled(true);

    // Update info bar labels with dynamic values
    m_lblMeasType->setText(m_presenter->getMeasurementName());
    m_lblOperator->setText(m_presenter->getOperatorName());
    m_lblSessionId->setText(m_presenter->getSessionId());
    m_lblProfile->setText(m_presenter->getProfileName());
    m_lblCal->setText(m_presenter->getCalibrationFile());
    m_lblStartTime->setText(m_presenter->getStartTime());
}

void MeasurementProgressPage::onPauseClicked() {
    m_presenter->pauseMeasurement();
    m_btnPause->setEnabled(false);
    m_btnResume->setEnabled(true);
}

void MeasurementProgressPage::onResumeClicked() {
    m_presenter->resumeMeasurement();
    m_btnPause->setEnabled(true);
    m_btnResume->setEnabled(false);
}

void MeasurementProgressPage::onStopClicked() {
    m_presenter->stopMeasurement();
}

void MeasurementProgressPage::onAbortClicked() {
    m_presenter->abortMeasurement();
}

void MeasurementProgressPage::updateProgress(float progressPercent, const QString &statusMessage) {
    m_progressBar->setValue(static_cast<int>(progressPercent));
    m_lblStatus->setText(statusMessage);
    
    // Auto update labels
    m_lblCurrAngle->setText(tr("%1°").arg(static_cast<double>(m_presenter->getCurrentAngle()), 0, 'f', 1));
    m_lblCurrFreq->setText(tr("%1 GHz").arg(m_presenter->getCurrentFreq() / 1e9, 0, 'f', 3));
    m_lblEstRemaining->setText(tr("%1 s").arg(m_presenter->getEstimatedRemainingTime(), 0, 'f', 1));
}

void MeasurementProgressPage::handleMeasurementFinished(bool success) {
    m_btnPause->setEnabled(false);
    m_btnResume->setEnabled(false);
    m_btnStop->setEnabled(false);
    m_btnAbort->setEnabled(false);
    if (success) {
        appendLogMessage(tr("[SUCCESS] Coordinated measurement finished successfully."));
    } else {
        appendLogMessage(tr("[ERROR] Coordinated measurement sweep stopped or aborted."));
    }
}

void MeasurementProgressPage::onExportClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Measurement Session"),
                                                    "", tr("Markdown (*.md);;HTML (*.html);;CSV (*.csv);;All Files (*)"));
    if (!fileName.isEmpty()) {
        m_presenter->exportSession(fileName);
        QMessageBox::information(this, tr("Export Data"), tr("Data exported successfully to %1").arg(fileName));
    }
}

void MeasurementProgressPage::appendLogMessage(const QString &msg) {
    m_logEdit->append(tr("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg));
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
}

} // namespace AMAS
