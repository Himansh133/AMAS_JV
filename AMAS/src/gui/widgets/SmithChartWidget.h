#ifndef AMAS_SMITHCHARTWIDGET_H
#define AMAS_SMITHCHARTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include "measurement/MeasurementSession.h"

namespace AMAS {

// Reusable Custom Smith Chart Widget with mouse zoom, pan, hover coordinate tracking, and markers
class SmithChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit SmithChartWidget(const QString &title, QWidget *parent = nullptr);
    QSize sizeHint() const override;

    // Set active session points
    void setSessionData(const std::vector<ProcessedMeasurementPoint> &points);
    
    // Set frequency markers
    void setMarkers(const std::vector<double> &markerFreqs);

    // Clear data
    void clearData();

    // Save plot image to a file
    bool savePng(const QString &filePath);

    // Toggle grid/markers visibility
    void setGridVisible(bool visible);
    void setMarkersVisible(bool visible);

public slots:
    void onResetViewClicked();
    void onExportClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QRect getChartRect() const;
    QPointF mapToPixel(double re, double im, const QRect &chartRect) const;
    QPointF mapToValue(const QPoint &pixelPos, const QRect &chartRect) const;
    
    void updateTrackingLabel(const QPoint &pos);

    QLabel *m_lblTitle;
    QLabel *m_lblTracking;
    QPushButton *m_btnReset;
    QPushButton *m_btnExport;

    std::vector<ProcessedMeasurementPoint> m_points;
    std::vector<double> m_markerFreqs;

    double m_zoomFactor = 1.0;
    QPointF m_panOffset = QPointF(0, 0);
    QPoint m_lastMousePos;
    bool m_isPanning = false;
    QPoint m_hoverPos;
    bool m_gridVisible = true;
    bool m_markersVisible = true;
};

} // namespace AMAS

#endif // AMAS_SMITHCHARTWIDGET_H
