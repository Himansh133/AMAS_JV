#ifndef AMAS_MEASUREMENTPLOTWIDGET_H
#define AMAS_MEASUREMENTPLOTWIDGET_H

#include <QWidget>
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QValueAxis>
#include <QScatterSeries>
#include <QLabel>
#include <QPushButton>
#include <vector>

namespace AMAS {

// A custom QChartView subclass to implement panning, zooming, and tracking
class CustomChartView : public QChartView {
    Q_OBJECT
public:
    explicit CustomChartView(QChart *chart, QWidget *parent = nullptr);
    ~CustomChartView() override = default;

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

// Reusable Cartesian plotting widget
class MeasurementPlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit MeasurementPlotWidget(const QString &title, QWidget *parent = nullptr);
    ~MeasurementPlotWidget() override = default;

    // Set dataset and refresh scales
    void setData(const std::vector<double> &xData, const std::vector<double> &yData, 
                 const QString &xTitle, const QString &yTitle);
    
    // Set active markers on plot
    void setMarkers(const std::vector<double> &markerFreqs);

    // Clear any existing series
    void clearData();

public slots:
    void onMouseMoved(const QPoint &pos);
    void onResetViewClicked();
    void onExportClicked();

private:
    void updateTrackingLabel(const QPoint &pos);

    QLabel *m_lblTitle;
    QLabel *m_lblTracking;
    QPushButton *m_btnReset;
    QPushButton *m_btnExport;

    QChart *m_chart;
    CustomChartView *m_chartView;
    QLineSeries *m_series;
    QScatterSeries *m_markerSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    std::vector<double> m_xData;
    std::vector<double> m_yData;
    QString m_xUnit;
    QString m_yUnit;
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTPLOTWIDGET_H
