#include "MeasurementSetupPage.h"
#include "presentation/SetupPresenter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QStyle>
#include <QInputDialog>
#include <cmath>

namespace AMAS {

MeasurementSetupPage::MeasurementSetupPage(SetupPresenter *presenter, QWidget *parent)
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

    // 4. Header block inside the scroll layout
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);
    
    auto *lblTitle = new QLabel(tr("Measurement Setup"), scrollContent);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(lblTitle);

    auto *lblSubtitle = new QLabel(tr("Configure all frequency, calibration, angular sweep, and directory variables."), scrollContent);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #C8C8C8;");
    headerLayout->addWidget(lblSubtitle);
    scrollLayout->addLayout(headerLayout);

    // 5. Sections and Groupboxes (with strict min heights)
    auto *grpType = new QGroupBox(tr("Measurement Type"), scrollContent);
    grpType->setMinimumHeight(120);
    createMeasTypePanel(grpType);
    scrollLayout->addWidget(grpType);

    auto *grpFreq = new QGroupBox(tr("Frequency Configuration"), scrollContent);
    grpFreq->setMinimumHeight(180);
    createFreqPanel(grpFreq);
    scrollLayout->addWidget(grpFreq);

    auto *grpCal = new QGroupBox(tr("Calibration"), scrollContent);
    grpCal->setMinimumHeight(170);
    createCalPanel(grpCal);
    scrollLayout->addWidget(grpCal);

    auto *grpAngular = new QGroupBox(tr("Angular Sweep Configuration"), scrollContent);
    grpAngular->setMinimumHeight(260);
    createAngularPanel(grpAngular);
    scrollLayout->addWidget(grpAngular);

    auto *grpOutput = new QGroupBox(tr("Output Configuration"), scrollContent);
    grpOutput->setMinimumHeight(220);
    createOutputPanel(grpOutput);
    scrollLayout->addWidget(grpOutput);

    auto *grpSummary = new QGroupBox(tr("Configuration Summary"), scrollContent);
    grpSummary->setMinimumHeight(220);
    createSummaryPanel(grpSummary);
    scrollLayout->addWidget(grpSummary);

    // 6. Action buttons bar at bottom
    auto *actionsBarLayout = new QHBoxLayout();
    createBottomBar(actionsBarLayout);
    scrollLayout->addLayout(actionsBarLayout);

    scrollArea->setWidget(scrollContent);

    // Hook presenter slots
    connect(m_btnSave, &QPushButton::clicked, this, &MeasurementSetupPage::onSaveClicked);
    connect(m_btnStart, &QPushButton::clicked, this, &MeasurementSetupPage::startRequested);
    connect(m_presenter, &SetupPresenter::profileUpdated, this, &MeasurementSetupPage::onProfileLoaded);

    // Value changes trigger profile updates
    connect(m_spinStartFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::onSetupControlChanged);
    connect(m_spinStopFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::onSetupControlChanged);
    connect(m_spinAzStep, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::onSetupControlChanged);
    connect(m_comboBand, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeasurementSetupPage::onSetupControlChanged);
    connect(m_comboMeasType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeasurementSetupPage::onSetupControlChanged);
    connect(m_btnCalBrowse, &QPushButton::clicked, this, &MeasurementSetupPage::onCalBrowseClicked);

    // Initial state loading
    onProfileLoaded(m_presenter->getProfile());
}

void MeasurementSetupPage::createMeasTypePanel(QGroupBox *box) {
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    m_comboMeasType = new QComboBox(box);
    m_comboMeasType->setMinimumWidth(260);
    m_comboMeasType->addItems({
        tr("S11 Measurement"),
        tr("Gain Measurement"),
        tr("Radiation Pattern (2D)"),
        tr("Radiation Pattern (3D)"),
        tr("Axial Ratio"),
        tr("Near Field Scan"),
        tr("Far Field Scan")
    });
    layout->addWidget(m_comboMeasType);

    m_lblMeasDesc = new QLabel(box);
    m_lblMeasDesc->setWordWrap(true);
    m_lblMeasDesc->setStyleSheet("color: #C8C8C8; font-size: 12px; line-height: 16px;");
    layout->addWidget(m_lblMeasDesc);

    connect(m_comboMeasType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeasurementSetupPage::onMeasTypeChanged);
}

void MeasurementSetupPage::createFreqPanel(QGroupBox *box) {
    auto *layout = new QGridLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    // Row 0: Frequencies
    auto *lblStart = new QLabel(tr("Start Frequency:"), box);
    layout->addWidget(lblStart, 0, 0);
    m_spinStartFreq = new QDoubleSpinBox(box);
    m_spinStartFreq->setRange(0.1, 40.0);
    m_spinStartFreq->setSuffix(tr(" GHz"));
    m_spinStartFreq->setDecimals(3);
    m_spinStartFreq->setMinimumWidth(160);
    layout->addWidget(m_spinStartFreq, 0, 1);

    auto *lblStop = new QLabel(tr("Stop Frequency:"), box);
    layout->addWidget(lblStop, 0, 2);
    m_spinStopFreq = new QDoubleSpinBox(box);
    m_spinStopFreq->setRange(0.1, 40.0);
    m_spinStopFreq->setSuffix(tr(" GHz"));
    m_spinStopFreq->setDecimals(3);
    m_spinStopFreq->setMinimumWidth(160);
    layout->addWidget(m_spinStopFreq, 0, 3);

    // Row 1: Points & Band Preset
    auto *lblPoints = new QLabel(tr("Sweep Points:"), box);
    layout->addWidget(lblPoints, 1, 0);
    m_spinPoints = new QSpinBox(box);
    m_spinPoints->setRange(1, 10001);
    m_spinPoints->setValue(401);
    m_spinPoints->setMinimumWidth(160);
    layout->addWidget(m_spinPoints, 1, 1);

    auto *lblBand = new QLabel(tr("Frequency Band:"), box);
    layout->addWidget(lblBand, 1, 2);
    m_comboBand = new QComboBox(box);
    m_comboBand->setMinimumWidth(260);
    m_comboBand->addItems({
        tr("8–12 GHz"),
        tr("12–18 GHz"),
        tr("18–26.5 GHz"),
        tr("26.5–40 GHz")
    });
    layout->addWidget(m_comboBand, 1, 3);

    // Column stretches
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    // Connect triggers
    connect(m_comboBand, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeasurementSetupPage::onFreqBandChanged);
    connect(m_spinStartFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MeasurementSetupPage::updateSummary);
    connect(m_spinStopFreq, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MeasurementSetupPage::updateSummary);
    connect(m_spinPoints, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MeasurementSetupPage::updateSummary);
}

void MeasurementSetupPage::createCalPanel(QGroupBox *box) {
    auto *layout = new QGridLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(tr("Calibration File:"), box), 0, 0);
    m_lblCalFile = new QLabel(tr("calchamber8_12ghz.sta"), box);
    m_lblCalFile->setStyleSheet("color: #FFFFFF; font-weight: bold;");
    layout->addWidget(m_lblCalFile, 0, 1);

    m_btnCalBrowse = new QPushButton(tr("Browse..."), box);
    m_btnCalBrowse->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_btnCalBrowse->setMinimumWidth(140);
    layout->addWidget(m_btnCalBrowse, 0, 2);

    layout->addWidget(new QLabel(tr("Cal Status:"), box), 1, 0);
    m_lblCalStatus = new QLabel(tr("Loaded & Valid"), box);
    m_lblCalStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    layout->addWidget(m_lblCalStatus, 1, 1);

    layout->addWidget(new QLabel(tr("Cal Date:"), box), 1, 2);
    m_lblCalDate = new QLabel(tr("2026-06-30 08:30"), box);
    m_lblCalDate->setStyleSheet("color: #C8C8C8;");
    layout->addWidget(m_lblCalDate, 1, 3);
}

void MeasurementSetupPage::createAngularPanel(QGroupBox *box) {
    auto *layout = new QGridLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    // Columns: Axis | Start | Stop | Step | Units
    auto *lblHeaderAxis = new QLabel(tr("Axis"), box);
    lblHeaderAxis->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    layout->addWidget(lblHeaderAxis, 0, 0, Qt::AlignCenter);

    auto *lblHeaderStart = new QLabel(tr("Start"), box);
    lblHeaderStart->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    layout->addWidget(lblHeaderStart, 0, 1, Qt::AlignCenter);

    auto *lblHeaderStop = new QLabel(tr("Stop"), box);
    lblHeaderStop->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    layout->addWidget(lblHeaderStop, 0, 2, Qt::AlignCenter);

    auto *lblHeaderStep = new QLabel(tr("Step"), box);
    lblHeaderStep->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    layout->addWidget(lblHeaderStep, 0, 3, Qt::AlignCenter);

    auto *lblHeaderUnits = new QLabel(tr("Units"), box);
    lblHeaderUnits->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    layout->addWidget(lblHeaderUnits, 0, 4, Qt::AlignCenter);

    // Azimuth row
    layout->addWidget(new QLabel(tr("Azimuth"), box), 1, 0);
    m_spinAzStart = new QDoubleSpinBox(box);
    m_spinAzStart->setRange(0.0, 360.0);
    m_spinAzStart->setValue(0.0);
    m_spinAzStart->setMinimumWidth(160);
    layout->addWidget(m_spinAzStart, 1, 1);

    m_spinAzStop = new QDoubleSpinBox(box);
    m_spinAzStop->setRange(0.0, 360.0);
    m_spinAzStop->setValue(180.0);
    m_spinAzStop->setMinimumWidth(160);
    layout->addWidget(m_spinAzStop, 1, 2);

    m_spinAzStep = new QDoubleSpinBox(box);
    m_spinAzStep->setRange(0.1, 45.0);
    m_spinAzStep->setValue(10.0);
    m_spinAzStep->setMinimumWidth(160);
    layout->addWidget(m_spinAzStep, 1, 3);
    layout->addWidget(new QLabel(tr("degrees (\xc2\xb0)"), box), 1, 4, Qt::AlignCenter);

    // Elevation row
    layout->addWidget(new QLabel(tr("Elevation"), box), 2, 0);
    m_spinElStart = new QDoubleSpinBox(box);
    m_spinElStart->setRange(-90.0, 90.0);
    m_spinElStart->setValue(0.0);
    m_spinElStart->setMinimumWidth(160);
    layout->addWidget(m_spinElStart, 2, 1);

    m_spinElStop = new QDoubleSpinBox(box);
    m_spinElStop->setRange(-90.0, 90.0);
    m_spinElStop->setValue(0.0);
    m_spinElStop->setMinimumWidth(160);
    layout->addWidget(m_spinElStop, 2, 2);

    m_spinElStep = new QDoubleSpinBox(box);
    m_spinElStep->setRange(0.1, 45.0);
    m_spinElStep->setValue(1.0);
    m_spinElStep->setMinimumWidth(160);
    layout->addWidget(m_spinElStep, 2, 3);
    layout->addWidget(new QLabel(tr("degrees (\xc2\xb0)"), box), 2, 4, Qt::AlignCenter);

    // Polarization row
    layout->addWidget(new QLabel(tr("Polarization"), box), 3, 0);
    m_spinPlStart = new QDoubleSpinBox(box);
    m_spinPlStart->setRange(0.0, 360.0);
    m_spinPlStart->setValue(0.0);
    m_spinPlStart->setMinimumWidth(160);
    layout->addWidget(m_spinPlStart, 3, 1);

    m_spinPlStop = new QDoubleSpinBox(box);
    m_spinPlStop->setRange(0.0, 360.0);
    m_spinPlStop->setValue(0.0);
    m_spinPlStop->setMinimumWidth(160);
    layout->addWidget(m_spinPlStop, 3, 2);

    m_spinPlStep = new QDoubleSpinBox(box);
    m_spinPlStep->setRange(0.1, 45.0);
    m_spinPlStep->setValue(1.0);
    m_spinPlStep->setMinimumWidth(160);
    layout->addWidget(m_spinPlStep, 3, 3);
    layout->addWidget(new QLabel(tr("degrees (\xc2\xb0)"), box), 3, 4, Qt::AlignCenter);

    // Column stretches
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 1);
    layout->setColumnStretch(3, 1);

    // Hook signals
    connect(m_spinAzStart, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinAzStop, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinAzStep, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinElStart, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinElStop, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinElStep, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinPlStart, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinPlStop, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
    connect(m_spinPlStep, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeasurementSetupPage::updateSummary);
}

void MeasurementSetupPage::createOutputPanel(QGroupBox *box) {
    auto *layout = new QFormLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    // Form row: Output Folder (QLineEdit + Browse)
    auto *folderLayout = new QHBoxLayout();
    folderLayout->setSpacing(12);
    folderLayout->setContentsMargins(0, 0, 0, 0);

    m_txtOutputFolder = new QLineEdit(tr("measurements/active_session"), box);
    m_txtOutputFolder->setMinimumWidth(350);
    folderLayout->addWidget(m_txtOutputFolder);

    m_btnOutputBrowse = new QPushButton(tr("Browse..."), box);
    m_btnOutputBrowse->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_btnOutputBrowse->setMinimumWidth(140);
    folderLayout->addWidget(m_btnOutputBrowse);

    layout->addRow(tr("Output Folder:"), folderLayout);

    // Checkboxes
    m_chkExportCsv = new QCheckBox(tr("Export CSV Data"), box);
    m_chkExportCsv->setChecked(true);
    layout->addRow(QString(), m_chkExportCsv);

    m_chkGeneratePdf = new QCheckBox(tr("Generate PDF Report"), box);
    m_chkGeneratePdf->setChecked(true);
    layout->addRow(QString(), m_chkGeneratePdf);

    m_chkOverwrite = new QCheckBox(tr("Overwrite Existing Files"), box);
    m_chkOverwrite->setChecked(false);
    layout->addRow(QString(), m_chkOverwrite);

    connect(m_txtOutputFolder, &QLineEdit::textChanged, this, &MeasurementSetupPage::updateSummary);
}

void MeasurementSetupPage::createSummaryPanel(QGroupBox *box) {
    auto *layout = new QFormLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto addSummaryFormRow = [box, layout](const QString &tag, QLabel *&valLabel) {
        valLabel = new QLabel(tr("N/A"), box);
        valLabel->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        layout->addRow(tag, valLabel);
    };

    addSummaryFormRow(tr("Measurement Type:"), m_lblSumMeasType);
    addSummaryFormRow(tr("Frequency Band:"), m_lblSumBand);
    addSummaryFormRow(tr("Sweep Points:"), m_lblSumPoints);
    addSummaryFormRow(tr("Total Angular Steps:"), m_lblSumPositions);
    addSummaryFormRow(tr("Estimated Duration:"), m_lblSumDuration);
    addSummaryFormRow(tr("Storage Destination:"), m_lblSumStorage);
}

void MeasurementSetupPage::createBottomBar(QHBoxLayout *layout) {
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QWidget *parentWidget = layout->parentWidget();

    m_btnLoad = new QPushButton(tr("Load Profile"), parentWidget);
    m_btnLoad->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_btnLoad->setMinimumHeight(32);
    m_btnLoad->setMinimumWidth(140);

    m_btnSave = new QPushButton(tr("Save Profile"), parentWidget);
    m_btnSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnSave->setMinimumHeight(32);
    m_btnSave->setMinimumWidth(140);

    m_btnValidate = new QPushButton(tr("Validate Configuration"), parentWidget);
    m_btnValidate->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_btnValidate->setMinimumHeight(32);
    m_btnValidate->setMinimumWidth(140);

    m_btnStart = new QPushButton(tr("Start Measurement"), parentWidget);
    m_btnStart->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_btnStart->setMinimumHeight(32);
    m_btnStart->setMinimumWidth(140);
    m_btnStart->setStyleSheet(
        "QPushButton { "
        "  background-color: #007ACC; "
        "  border: 1px solid #3F3F46; "
        "  color: #FFFFFF; "
        "} "
        "QPushButton:hover { "
        "  background-color: #0098FF; "
        "} "
    );

    m_btnCancel = new QPushButton(tr("Cancel"), parentWidget);
    m_btnCancel->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    m_btnCancel->setMinimumHeight(32);
    m_btnCancel->setMinimumWidth(140);

    layout->addWidget(m_btnLoad);
    layout->addWidget(m_btnSave);
    layout->addStretch();
    layout->addWidget(m_btnValidate);
    layout->addWidget(m_btnStart);
    layout->addWidget(m_btnCancel);
}

void MeasurementSetupPage::onMeasTypeChanged(int index) {
    QStringList descriptions = {
        tr("Measures the reflection coefficient (S11) at the antenna port to analyze input impedance, return loss, and VSWR over the sweep points."),
        tr("Computes absolute gain (dBi) by performing a comparative transmission calibration (S21) against a standard gain reference horn."),
        tr("Rotates the turntable around the Azimuth axis to map radiation intensity and extract 3dB/10dB beamwidth metrics in 2D."),
        tr("Rotates both Azimuth and Elevation axes sequentially to construct a complete 3D radiation sphere of the antenna under test."),
        tr("Analyzes orthogonal circular polarization components over angles to calculate axial ratio values for circular polarization purity."),
        tr("Acquires field data inside the radiative near-field boundary zone for near-to-far-field mathematical conversion."),
        tr("Acquires pattern details outside the 2*D^2/lambda far-field path limit to determine direct radiation patterns.")
    };

    if (index >= 0 && index < descriptions.size()) {
        m_lblMeasDesc->setText(descriptions[index]);
    }
    updateSummary();
}

void MeasurementSetupPage::onFreqBandChanged(int index) {
    struct BandLimits {
        double start;
        double stop;
        int points;
        QString name;
    } bands[] = {
        { 8.0, 12.0, 401, tr("8-12 GHz") },
        { 12.0, 18.0, 601, tr("12-18 GHz") },
        { 18.0, 26.5, 851, tr("18-26.5 GHz") },
        { 26.5, 40.0, 136, tr("26.5-40 GHz") }
    };

    if (index >= 0 && index < 4) {
        m_spinStartFreq->setValue(bands[index].start);
        m_spinStopFreq->setValue(bands[index].stop);
        m_spinPoints->setValue(bands[index].points);
    }
    updateSummary();
}

void MeasurementSetupPage::updateSummary() {
    if (!m_lblSumMeasType) return;

    m_lblSumMeasType->setText(m_comboMeasType->currentText());
    m_lblSumBand->setText(QString("%1 GHz to %2 GHz")
                          .arg(m_spinStartFreq->value(), 0, 'f', 3)
                          .arg(m_spinStopFreq->value(), 0, 'f', 3));
    m_lblSumPoints->setText(QString::number(m_spinPoints->value()));

    // Angular positions steps calculation
    auto calcSteps = [](QDoubleSpinBox *start, QDoubleSpinBox *stop, QDoubleSpinBox *step) -> int {
        double range = std::abs(stop->value() - start->value());
        if (range < 0.01) return 1;
        if (step->value() < 0.01) return 1;
        return static_cast<int>(range / step->value()) + 1;
    };

    int azPos = calcSteps(m_spinAzStart, m_spinAzStop, m_spinAzStep);
    int elPos = calcSteps(m_spinElStart, m_spinElStop, m_spinElStep);
    int plPos = calcSteps(m_spinPlStart, m_spinPlStop, m_spinPlStep);
    int totalPositions = azPos * elPos * plPos;

    m_lblSumPositions->setText(QString::number(totalPositions));

    // Estimated duration calculation
    double totalSeconds = totalPositions * (0.4 + 0.0012 * m_spinPoints->value());
    
    int h = static_cast<int>(totalSeconds) / 3600;
    int m = (static_cast<int>(totalSeconds) % 3600) / 60;
    int s = static_cast<int>(totalSeconds) % 60;

    m_lblSumDuration->setText(QString("%1:%2:%3")
                              .arg(h, 2, 10, QChar('0'))
                              .arg(m, 2, 10, QChar('0'))
                              .arg(s, 2, 10, QChar('0')));

    m_lblSumStorage->setText(m_txtOutputFolder->text());
}

void MeasurementSetupPage::onSaveClicked() {
    MeasurementProfile profile;
    profile.profileName = m_comboMeasType->currentText().toStdString();
    profile.measurementType = m_comboMeasType->currentText().toStdString();
    profile.startFrequencyHz = m_spinStartFreq->value() * 1e9;
    profile.stopFrequencyHz = m_spinStopFreq->value() * 1e9;
    profile.sweepPoints = m_spinPoints->value();
    profile.calibrationFile = m_lblCalFile->text().toStdString();

    profile.positioner.usePositioner = true;
    profile.positioner.startAngleDeg = m_spinAzStart->value();
    profile.positioner.stopAngleDeg = m_spinAzStop->value();
    profile.positioner.stepAngleDeg = m_spinAzStep->value();
    profile.positioner.settleTimeMs = 150;

    m_presenter->updateProfile(profile);
}

void MeasurementSetupPage::onProfileLoaded(const MeasurementProfile &profile) {
    // Block signals to prevent infinite update loop
    blockSignals(true);
    m_spinStartFreq->setValue(profile.startFrequencyHz / 1e9);
    m_spinStopFreq->setValue(profile.stopFrequencyHz / 1e9);
    m_spinPoints->setValue(profile.sweepPoints);
    m_lblCalFile->setText(QString::fromStdString(profile.calibrationFile));

    m_spinAzStart->setValue(profile.positioner.startAngleDeg);
    m_spinAzStop->setValue(profile.positioner.stopAngleDeg);
    m_spinAzStep->setValue(profile.positioner.stepAngleDeg);
    blockSignals(false);

    updateSummary();
}

void MeasurementSetupPage::onCalBrowseClicked() {
    bool ok = false;
    QString item = QInputDialog::getItem(this, tr("Select Calibration File"),
                                         tr("Available Calibrations:"),
                                         m_presenter->getAvailableCalibrations(),
                                         0, false, &ok);
    if (ok && !item.isEmpty()) {
        m_lblCalFile->setText(item);
        onSetupControlChanged();
    }
}

void MeasurementSetupPage::onSetupControlChanged() {
    MeasurementProfile profile;
    profile.profileName = m_comboMeasType->currentText().toStdString();
    profile.measurementType = m_comboMeasType->currentText().toStdString();
    profile.startFrequencyHz = m_spinStartFreq->value() * 1e9;
    profile.stopFrequencyHz = m_spinStopFreq->value() * 1e9;
    profile.sweepPoints = m_spinPoints->value();
    profile.calibrationFile = m_lblCalFile->text().toStdString();

    profile.positioner.usePositioner = true;
    profile.positioner.startAngleDeg = m_spinAzStart->value();
    profile.positioner.stopAngleDeg = m_spinAzStop->value();
    profile.positioner.stepAngleDeg = m_spinAzStep->value();
    profile.positioner.settleTimeMs = 150;

    m_presenter->updateProfile(profile);
    updateSummary();
}

} // namespace AMAS
