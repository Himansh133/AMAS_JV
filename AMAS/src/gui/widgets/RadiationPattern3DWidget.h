#ifndef AMAS_RADIATIONPATTERN3DVIEWER_H
#define AMAS_RADIATIONPATTERN3DVIEWER_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QTimer>
#include <QLabel>
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>
#include <vector>
#include "measurement/MeasurementSession.h"

namespace AMAS {

using namespace QtDataVisualization;

enum class AntennaPatternType {
    Dipole,
    Patch,
    Horn,
    Isotropic,
    GaussianBeam
};

class RadiationPattern3DWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadiationPattern3DWidget(const QString &title, QWidget *parent = nullptr);
    ~RadiationPattern3DWidget() override;

    // Propagate active session
    void setSessionData(const MeasurementSession &session);
    void clearData();

public slots:
    void onResetCamera();
    void toggleProjection();
    void toggleAnimation();
    void onExportPng();
    void onExportMetadata();

private slots:
    void onPatternTypeChanged(int index);
    void onDisplayModeChanged(int index);
    void onSpeedChanged(int value);
    void onSliceChanged();
    void onShowAxesToggled(bool checked);
    void onAnimationStep();

private:
    void setupUI(const QString &title);
    void setup3DChart();
    void generatePatternData();

    QLabel *m_lblTitle;
    QLabel *m_lblTracking;
    QComboBox *m_comboPattern;
    QComboBox *m_comboDisplayMode;
    QPushButton *m_btnProjection;
    QPushButton *m_btnReset;
    QPushButton *m_btnExportPng;
    QPushButton *m_btnExportMeta;

    // Animation widgets
    QPushButton *m_btnPlay;
    QSlider *m_sliderSpeed;

    // Slice widgets
    QSlider *m_sliderTheta;
    QSlider *m_sliderPhi;
    QCheckBox *m_chkShowAxes;

    // Qt 3D Surface
    Q3DSurface *m_surface;
    QSurfaceDataProxy *m_proxy;
    QSurface3DSeries *m_series;
    QWidget *m_surfaceContainer;

    QTimer *m_animationTimer;
    float m_rotationAngle = 0.0f;
    int m_rotationSpeed = 2; // Degrees per step
    bool m_isAnimating = false;

    // Data Cache
    MeasurementSession m_currentSession;
    AntennaPatternType m_selectedPattern = AntennaPatternType::Dipole;
};

} // namespace AMAS

#endif // AMAS_RADIATIONPATTERN3DVIEWER_H
