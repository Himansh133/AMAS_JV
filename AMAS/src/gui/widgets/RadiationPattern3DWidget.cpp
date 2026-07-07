#include "RadiationPattern3DWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QStyle>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AMAS {

using namespace QtDataVisualization;

RadiationPattern3DWidget::RadiationPattern3DWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_surface(nullptr)
    , m_proxy(nullptr)
    , m_series(nullptr)
    , m_surfaceContainer(nullptr)
{
    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(30);

    setupUI(title);
    setup3DChart();

    connect(m_animationTimer, &QTimer::timeout, this, &RadiationPattern3DWidget::onAnimationStep);
    
    // Initial generation
    generatePatternData();
}

RadiationPattern3DWidget::~RadiationPattern3DWidget() {
    if (m_animationTimer) m_animationTimer->stop();
}

void RadiationPattern3DWidget::setupUI(const QString &title) {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    // Toolbar Header
    auto *headerLayout = new QHBoxLayout();

    m_lblTitle = new QLabel(title, this);
    m_lblTitle->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 13px;");
    headerLayout->addWidget(m_lblTitle);

    headerLayout->addSpacing(16);

    // Pattern type selector
    m_comboPattern = new QComboBox(this);
    m_comboPattern->addItems({
        tr("Dipole Pattern"),
        tr("Patch Pattern"),
        tr("Horn Pattern"),
        tr("Isotropic Pattern"),
        tr("Gaussian Beam")
    });
    m_comboPattern->setStyleSheet(
        "QComboBox { background-color: #2D2D30; border: 1px solid #3F3F46; color: #FFFFFF; font-size: 11px; padding: 2px 4px; }"
    );
    headerLayout->addWidget(m_comboPattern);

    // Display mode
    m_comboDisplayMode = new QComboBox(this);
    m_comboDisplayMode->addItems({
        tr("Surface Mode"),
        tr("Wireframe Mode"),
        tr("Surface + Wireframe")
    });
    m_comboDisplayMode->setStyleSheet(
        "QComboBox { background-color: #2D2D30; border: 1px solid #3F3F46; color: #FFFFFF; font-size: 11px; padding: 2px 4px; }"
    );
    headerLayout->addWidget(m_comboDisplayMode);

    // Projection toggle
    m_btnProjection = new QPushButton(tr("Perspective"), this);
    m_btnProjection->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #DCDCDC; padding: 2px 8px; font-size: 11px; } "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnProjection);

    headerLayout->addStretch();

    // Readout label
    m_lblTracking = new QLabel("", this);
    m_lblTracking->setStyleSheet("color: #4CAF50; font-family: monospace; font-size: 11px; font-weight: bold; margin-right: 12px;");
    headerLayout->addWidget(m_lblTracking);

    // Reset view
    m_btnReset = new QPushButton(tr("Reset Camera"), this);
    m_btnReset->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_btnReset->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #DCDCDC; padding: 2px 8px; font-size: 11px; } "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnReset);

    // Exports
    m_btnExportPng = new QPushButton(tr("PNG"), this);
    m_btnExportPng->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnExportPng->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #DCDCDC; padding: 2px 8px; font-size: 11px; } "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnExportPng);

    m_btnExportMeta = new QPushButton(tr("Metadata"), this);
    m_btnExportMeta->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    m_btnExportMeta->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #DCDCDC; padding: 2px 8px; font-size: 11px; } "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnExportMeta);

    mainLayout->addLayout(headerLayout);

    // Connect Header controls
    connect(m_comboPattern, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RadiationPattern3DWidget::onPatternTypeChanged);
    connect(m_comboDisplayMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RadiationPattern3DWidget::onDisplayModeChanged);
    connect(m_btnProjection, &QPushButton::clicked, this, &RadiationPattern3DWidget::toggleProjection);
    connect(m_btnReset, &QPushButton::clicked, this, &RadiationPattern3DWidget::onResetCamera);
    connect(m_btnExportPng, &QPushButton::clicked, this, &RadiationPattern3DWidget::onExportPng);
    connect(m_btnExportMeta, &QPushButton::clicked, this, &RadiationPattern3DWidget::onExportMetadata);
}

void RadiationPattern3DWidget::setup3DChart() {
    m_surface = new Q3DSurface();
    m_surface->setAntialiasing(true);

    // Apply dark theme aesthetics
    Q3DTheme *theme = m_surface->activeTheme();
    theme->setBackgroundEnabled(false);
    theme->setGridEnabled(true);
    theme->setLabelBackgroundEnabled(false);
    theme->setLabelTextColor(QColor("#C8C8C8"));
    theme->setGridLineColor(QColor("#3F3F46"));
    theme->setLabelFont(QFont("Segoe UI", 9));

    // Axis configs
    m_surface->axisX()->setTitle(tr("X (Theta Direction)"));
    m_surface->axisY()->setTitle(tr("Y (Polar Height)"));
    m_surface->axisZ()->setTitle(tr("Z (Phi Direction)"));
    m_surface->axisX()->setTitleVisible(true);
    m_surface->axisY()->setTitleVisible(true);
    m_surface->axisZ()->setTitleVisible(true);

    m_proxy = new QSurfaceDataProxy();
    m_series = new QSurface3DSeries(m_proxy);
    m_surface->addSeries(m_series);

    // Engineering Gain Color Map (Blue -> Green -> Yellow -> Orange -> Red)
    QLinearGradient gr;
    gr.setColorAt(0.0, QColor("#0000FF"));  // Blue (Low Gain)
    gr.setColorAt(0.25, QColor("#00FF00")); // Green
    gr.setColorAt(0.5, QColor("#FFFF00"));  // Yellow
    gr.setColorAt(0.75, QColor("#FF7F00")); // Orange
    gr.setColorAt(1.0, QColor("#FF0000"));  // Red (High Gain)
    m_series->setBaseGradient(gr);
    m_series->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
    m_series->setDrawMode(QSurface3DSeries::DrawSurface);

    // Embed in Layout
    m_surfaceContainer = QWidget::createWindowContainer(m_surface, this);
    m_surfaceContainer->setMinimumHeight(350);
    m_surfaceContainer->setStyleSheet("border: 1px solid #3F3F46; background-color: #1E1E1E;");
    
    auto *layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (layout) {
        layout->addWidget(m_surfaceContainer, 1);
    }

    // Add Side & Animation Control panel at the bottom
    auto *controlsBox = new QGroupBox(tr("Visualization Workspace Controls"), this);
    controlsBox->setStyleSheet(
        "QGroupBox { color: #FFFFFF; font-weight: bold; border: 1px solid #3F3F46; margin-top: 6px; padding-top: 10px; } "
        "QLabel { color: #C8C8C8; font-size: 11px; } "
    );
    auto *ctrlLayout = new QGridLayout(controlsBox);
    ctrlLayout->setSpacing(8);

    // Slices
    ctrlLayout->addWidget(new QLabel(tr("Theta Slice:")), 0, 0);
    m_sliderTheta = new QSlider(Qt::Horizontal, this);
    m_sliderTheta->setRange(0, 180);
    m_sliderTheta->setValue(90);
    m_sliderTheta->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #3F3F46; height: 4px; background: #2D2D30; } QSlider::handle:horizontal { background: #007ACC; width: 10px; margin: -3px 0; }");
    ctrlLayout->addWidget(m_sliderTheta, 0, 1);

    ctrlLayout->addWidget(new QLabel(tr("Phi Slice:")), 1, 0);
    m_sliderPhi = new QSlider(Qt::Horizontal, this);
    m_sliderPhi->setRange(0, 360);
    m_sliderPhi->setValue(180);
    m_sliderPhi->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #3F3F46; height: 4px; background: #2D2D30; } QSlider::handle:horizontal { background: #007ACC; width: 10px; margin: -3px 0; }");
    ctrlLayout->addWidget(m_sliderPhi, 1, 1);

    // Axes toggle
    m_chkShowAxes = new QCheckBox(tr("Show Grid Axes"), this);
    m_chkShowAxes->setChecked(true);
    m_chkShowAxes->setStyleSheet("QCheckBox { color: #C8C8C8; font-size: 11px; }");
    ctrlLayout->addWidget(m_chkShowAxes, 0, 2);

    // Animation controls
    auto *animLayout = new QHBoxLayout();
    m_btnPlay = new QPushButton(tr("Play Rotation"), this);
    m_btnPlay->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #FFFFFF; font-size: 11px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #3F3F46; }"
    );
    animLayout->addWidget(m_btnPlay);

    m_sliderSpeed = new QSlider(Qt::Horizontal, this);
    m_sliderSpeed->setRange(1, 10);
    m_sliderSpeed->setValue(2);
    m_sliderSpeed->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #3F3F46; height: 4px; background: #2D2D30; } QSlider::handle:horizontal { background: #007ACC; width: 10px; margin: -3px 0; }");
    animLayout->addWidget(new QLabel(tr("Speed:")));
    animLayout->addWidget(m_sliderSpeed);

    ctrlLayout->addLayout(animLayout, 1, 2);

    if (layout) {
        layout->addWidget(controlsBox);
    }

    // Connect controls
    connect(m_sliderTheta, &QSlider::valueChanged, this, &RadiationPattern3DWidget::onSliceChanged);
    connect(m_sliderPhi, &QSlider::valueChanged, this, &RadiationPattern3DWidget::onSliceChanged);
    connect(m_chkShowAxes, &QCheckBox::toggled, this, &RadiationPattern3DWidget::onShowAxesToggled);
    connect(m_btnPlay, &QPushButton::clicked, this, &RadiationPattern3DWidget::toggleAnimation);
    connect(m_sliderSpeed, &QSlider::valueChanged, this, &RadiationPattern3DWidget::onSpeedChanged);

    onResetCamera();
    onSliceChanged();
}

void RadiationPattern3DWidget::setSessionData(const MeasurementSession &session) {
    m_currentSession = session;
    generatePatternData();
}

void RadiationPattern3DWidget::clearData() {
    m_currentSession = MeasurementSession();
    generatePatternData();
}

void RadiationPattern3DWidget::onPatternTypeChanged(int index) {
    m_selectedPattern = static_cast<AntennaPatternType>(index);
    generatePatternData();
}

void RadiationPattern3DWidget::onDisplayModeChanged(int index) {
    if (!m_series) return;
    if (index == 0) {
        m_series->setDrawMode(QSurface3DSeries::DrawSurface);
    } else if (index == 1) {
        m_series->setDrawMode(QSurface3DSeries::DrawWireframe);
    } else {
        m_series->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
    }
}

void RadiationPattern3DWidget::onSpeedChanged(int value) {
    m_rotationSpeed = value;
}

void RadiationPattern3DWidget::onSliceChanged() {
    double thetaVal = m_sliderTheta->value();
    double phiVal = m_sliderPhi->value();

    // Map slice positions to gain value depending on model
    double R = 0.0;
    double thetaRad = thetaVal * M_PI / 180.0;
    double phiRad = phiVal * M_PI / 180.0;

    switch (m_selectedPattern) {
        case AntennaPatternType::Dipole:
            R = 2.15 + 10.0 * std::log10(std::max(1e-4, std::pow(std::sin(thetaRad), 2)));
            break;
        case AntennaPatternType::Patch:
            R = 7.0 + 10.0 * std::log10(std::max(1e-4, std::pow(std::cos(thetaRad), 2) * (1.0 + 0.15 * std::cos(2.0 * phiRad))));
            break;
        case AntennaPatternType::Horn:
            R = 15.0 - 40.0 * std::pow(thetaRad / 0.5, 2);
            break;
        case AntennaPatternType::Isotropic:
            R = 0.0;
            break;
        case AntennaPatternType::GaussianBeam:
            R = 20.0 - 50.0 * std::pow(thetaRad / 0.2, 2);
            break;
    }

    if (R < -40.0) R = -40.0;

    m_lblTracking->setText(QString("Slice: θ = %1° | φ = %2° | Gain = %3 dBi")
                           .arg(thetaVal)
                           .arg(phiVal)
                           .arg(R, 0, 'f', 1));
}

void RadiationPattern3DWidget::onShowAxesToggled(bool checked) {
    if (m_surface) {
        m_surface->axisX()->setVisible(checked);
        m_surface->axisY()->setVisible(checked);
        m_surface->axisZ()->setVisible(checked);
    }
}

void RadiationPattern3DWidget::generatePatternData() {
    if (!m_proxy) return;

    int thetaSteps = 37; // 0 to 180 degrees in 5 deg steps
    int phiSteps = 73;   // 0 to 360 degrees in 5 deg steps

    QSurfaceDataArray *dataArray = new QSurfaceDataArray();
    dataArray->reserve(thetaSteps);

    for (int i = 0; i < thetaSteps; ++i) {
        double thetaDeg = i * 5.0;
        double thetaRad = thetaDeg * M_PI / 180.0;
        QSurfaceDataRow *newRow = new QSurfaceDataRow();
        newRow->reserve(phiSteps);

        for (int j = 0; j < phiSteps; ++j) {
            double phiDeg = j * 5.0;
            double phiRad = phiDeg * M_PI / 180.0;

            // Generate Gain factors R (linear radius) between 0.1 and 1.0
            double R = 0.1;
            switch (m_selectedPattern) {
                case AntennaPatternType::Dipole:
                    R = 0.1 + 0.9 * std::pow(std::sin(thetaRad), 2);
                    break;
                case AntennaPatternType::Patch:
                    if (thetaRad < M_PI / 2.0) {
                        R = 0.15 + 0.85 * std::pow(std::cos(thetaRad), 2) * (1.0 + 0.15 * std::cos(2.0 * phiRad));
                    } else {
                        R = 0.15; // Small backlobe
                    }
                    break;
                case AntennaPatternType::Horn:
                    R = 0.1 + 0.9 * std::exp(-std::pow(thetaRad / 0.5, 2)) * (1.0 + 0.1 * std::cos(4.0 * phiRad));
                    break;
                case AntennaPatternType::Isotropic:
                    R = 0.7; // Perfect sphere
                    break;
                case AntennaPatternType::GaussianBeam:
                    R = 0.1 + 0.9 * std::exp(-std::pow(thetaRad / 0.2, 2));
                    break;
            }

            // Map to Cartesian coordinates for 3D Balloon visualization
            float x = R * std::sin(thetaRad) * std::cos(phiRad);
            float y = R * std::cos(thetaRad); // Height axis
            float z = R * std::sin(thetaRad) * std::sin(phiRad);

            newRow->append(QSurfaceDataItem(QVector3D(x, y, z)));
        }
        dataArray->append(newRow);
    }

    m_proxy->resetArray(dataArray);
}

void RadiationPattern3DWidget::onResetCamera() {
    if (m_surface) {
        m_surface->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricLeftHigh);
        m_surface->scene()->activeCamera()->setZoomLevel(100.0f);
    }
}

void RadiationPattern3DWidget::toggleProjection() {
    if (!m_surface) return;
    auto *camera = m_surface->scene()->activeCamera();
    
    // Toggle camera presets or orthographic mode (preset toggle is standard in Qt3D/DataVis)
    if (m_btnProjection->text() == tr("Perspective")) {
        camera->setCameraPreset(Q3DCamera::CameraPresetDirectlyAbove);
        m_btnProjection->setText(tr("Orthographic"));
    } else {
        camera->setCameraPreset(Q3DCamera::CameraPresetIsometricLeftHigh);
        m_btnProjection->setText(tr("Perspective"));
    }
}

void RadiationPattern3DWidget::toggleAnimation() {
    m_isAnimating = !m_isAnimating;
    if (m_isAnimating) {
        m_animationTimer->start();
        m_btnPlay->setText(tr("Pause Rotation"));
    } else {
        m_animationTimer->stop();
        m_btnPlay->setText(tr("Play Rotation"));
    }
}

void RadiationPattern3DWidget::onAnimationStep() {
    if (m_surface) {
        m_rotationAngle += m_rotationSpeed;
        if (m_rotationAngle >= 360.0f) m_rotationAngle -= 360.0f;
        m_surface->scene()->activeCamera()->setXRotation(m_rotationAngle);
    }
}

void RadiationPattern3DWidget::onExportPng() {
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export 3D Pattern as PNG"), "", tr("PNG Image (*.png)"));
    
    if (fileName.isEmpty()) return;

    // Grab surface container frame
    QPixmap pixmap = m_surfaceContainer->grab();
    if (pixmap.save(fileName, "PNG")) {
        QMessageBox::information(this, tr("Export Success"), 
            tr("3D Pattern snapshot saved successfully."));
    } else {
        QMessageBox::warning(this, tr("Export Failure"), 
            tr("Failed to save image file."));
    }
}

void RadiationPattern3DWidget::onExportMetadata() {
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Viewport Metadata as TXT"), "", tr("Text Files (*.txt)"));
    
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        auto *camera = m_surface->scene()->activeCamera();
        out << "==================================================\n";
        out << "        AMAS 3D PATTERN VIEWPORT METADATA        \n";
        out << "==================================================\n";
        out << "Pattern Model:  " << m_comboPattern->currentText() << "\n";
        out << "Display Mode:   " << m_comboDisplayMode->currentText() << "\n";
        out << "Camera Zoom:    " << camera->zoomLevel() << "%\n";
        out << "Camera Yaw (X): " << camera->xRotation() << " deg\n";
        out << "Camera Pitch(Y):" << camera->yRotation() << " deg\n";
        out << "Grid Settings:  " << (m_chkShowAxes->isChecked() ? "Visible" : "Hidden") << "\n";
        out << "Slice Theta:    " << m_sliderTheta->value() << " deg\n";
        out << "Slice Phi:      " << m_sliderPhi->value() << " deg\n";
        out << "==================================================\n";
        file.close();
        QMessageBox::information(this, tr("Export Success"), 
            tr("Viewport metadata successfully saved to '%1'").arg(QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::warning(this, tr("Export Failure"), tr("Failed to write to file."));
    }
}

} // namespace AMAS
