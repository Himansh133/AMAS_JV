#ifndef AMAS_POLARPLOTWIDGET_H
#define AMAS_POLARPLOTWIDGET_H

#include <QWidget>
#include <QChartView>
#include <QPolarChart>
#include <QLineSeries>
#include <QScatterSeries>
#include <QValueAxis>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <vector>
#include "measurement/MeasurementSession.h"

namespace AMAS {

// Custom polar view to enable zoom interactions and hover coordinate tracking
class CustomPolarView : public QChartView {
    Q_OBJECT
public:
    explicit CustomPolarView(QChart *chart, QWidget *parent = nullptr);
    ~CustomPolarView() override = default;

signals:
    void mouseMoved(const QPoint &pos);
    void doubleClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QPoint m_lastMousePos;
    bool m_isPanning = false;
};

// Reusable Polar Plot Widget supporting multiple traces, frequency selection, and markers
class PolarPlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit PolarPlotWidget(const QString &title, QWidget *parent = nullptr);
    ~PolarPlotWidget() override = default;

    // Load full session dataset
    void setSessionData(const std::vector<ProcessedMeasurementPoint> &points);
    
    // Set highlight markers
    void setMarkers(const std::vector<QPointF> &markerPoints); // angle, magnitude

    // Clear data
    void clearData();

public slots:
    void onResetViewClicked();
    void onExportClicked();

private slots:
    void onTraceTypeChanged(int index);
    void onFrequencyChanged(int index);
    void onMouseMoved(const QPoint &pos);

private:
    void updatePlot();
    void updateTrackingLabel(const QPoint &pos);

    QLabel *m_lblTitle;
    QLabel *m_lblTracking;
    QComboBox *m_comboTrace;
    QComboBox *m_comboFreq;
    QPushButton *m_btnReset;
    QPushButton *m_btnExport;

    QPolarChart *m_chart;
    CustomPolarView *m_chartView;
    QLineSeries *m_series;
    QScatterSeries *m_markerSeries;
    QValueAxis *m_axisAngular;
    QValueAxis *m_axisRadial;

    std::vector<ProcessedMeasurementPoint> m_points;
    std::vector<double> m_uniqueFrequencies;
    double m_selectedFreqHz = 0.0;
    bool m_showMagnitude = true;
};

} // namespace AMAS

#endif // AMAS_POLARPLOTWIDGET_H
