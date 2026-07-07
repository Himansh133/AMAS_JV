#include "MeasurementPlotWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFileDialog>
#include <QStyle>
#include <QMessageBox>
#include <cmath>
#include <algorithm>

namespace AMAS {

// ==========================================
// CustomChartView Implementation
// ==========================================

CustomChartView::CustomChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setRenderHint(QPainter::Antialiasing);
}

void CustomChartView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        m_lastMousePos = event->pos();
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void CustomChartView::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        chart()->scroll(-delta.x(), delta.y());
        m_lastMousePos = event->pos();
        event->accept();
        return;
    }
    QChartView::mouseMoveEvent(event);
    emit mouseMoved(event->pos());
}

void CustomChartView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QChartView::mouseReleaseEvent(event);
}

void CustomChartView::wheelEvent(QWheelEvent *event) {
    qreal factor = (event->angleDelta().y() > 0) ? 0.9 : 1.1;
    chart()->zoom(factor);
    event->accept();
}

void CustomChartView::mouseDoubleClickEvent(QMouseEvent *event) {
    emit doubleClicked();
    QChartView::mouseDoubleClickEvent(event);
}


// ==========================================
// MeasurementPlotWidget Implementation
// ==========================================

MeasurementPlotWidget::MeasurementPlotWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    // Main layout
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Header Toolbar Layout
    auto *headerLayout = new QHBoxLayout();
    
    m_lblTitle = new QLabel(title, this);
    m_lblTitle->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 13px;");
    headerLayout->addWidget(m_lblTitle);

    headerLayout->addStretch();

    m_lblTracking = new QLabel("", this);
    m_lblTracking->setStyleSheet("color: #4CAF50; font-family: monospace; font-size: 11px; font-weight: bold; margin-right: 12px;");
    headerLayout->addWidget(m_lblTracking);

    m_btnReset = new QPushButton(tr("Reset"), this);
    m_btnReset->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_btnReset->setStyleSheet(
        "QPushButton { "
        "  background-color: #2D2D30; "
        "  border: 1px solid #3F3F46; "
        "  color: #DCDCDC; "
        "  padding: 2px 8px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnReset);

    m_btnExport = new QPushButton(tr("PNG"), this);
    m_btnExport->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnExport->setStyleSheet(
        "QPushButton { "
        "  background-color: #2D2D30; "
        "  border: 1px solid #3F3F46; "
        "  color: #DCDCDC; "
        "  padding: 2px 8px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnExport);

    layout->addLayout(headerLayout);

    // Setup Chart
    m_chart = new QChart();
    m_chart->legend()->hide();
    
    // Sleek dark theme palette matching the interface
    m_chart->setBackgroundBrush(QBrush(QColor("#1E1E1E")));
    m_chart->setPlotAreaBackgroundBrush(QBrush(QColor("#252526")));
    m_chart->setPlotAreaBackgroundVisible(true);
    m_chart->setMargins(QMargins(8, 8, 8, 8));

    m_series = new QLineSeries(m_chart);
    // Use green lines for magnitude, orange for phase
    if (title.contains(tr("Magnitude"), Qt::CaseInsensitive)) {
        m_series->setColor(QColor("#4CAF50")); // Green
    } else {
        m_series->setColor(QColor("#FF9800")); // Orange
    }
    m_chart->addSeries(m_series);

    m_markerSeries = new QScatterSeries(m_chart);
    m_markerSeries->setColor(QColor("#FF4C4C")); // Red
    m_markerSeries->setMarkerSize(8.0);
    m_chart->addSeries(m_markerSeries);

    // Create Axis
    m_axisX = new QValueAxis(m_chart);
    m_axisX->setGridLineColor(QColor("#3F3F46"));
    m_axisX->setLabelsColor(QColor("#C8C8C8"));
    m_axisX->setTitleBrush(QBrush(QColor("#FFFFFF")));
    
    m_axisY = new QValueAxis(m_chart);
    m_axisY->setGridLineColor(QColor("#3F3F46"));
    m_axisY->setLabelsColor(QColor("#C8C8C8"));
    m_axisY->setTitleBrush(QBrush(QColor("#FFFFFF")));

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    m_markerSeries->attachAxis(m_axisX);
    m_markerSeries->attachAxis(m_axisY);

    // Setup Chart View
    m_chartView = new CustomChartView(m_chart, this);
    m_chartView->setRubberBand(QChartView::RectangleRubberBand);
    m_chartView->setStyleSheet("background-color: #1E1E1E; border: 1px solid #3F3F46;");
    
    layout->addWidget(m_chartView, 1);

    // Bind event signals
    connect(m_chartView, &CustomChartView::mouseMoved, this, &MeasurementPlotWidget::onMouseMoved);
    connect(m_chartView, &CustomChartView::doubleClicked, this, &MeasurementPlotWidget::onResetViewClicked);
    connect(m_btnReset, &QPushButton::clicked, this, &MeasurementPlotWidget::onResetViewClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &MeasurementPlotWidget::onExportClicked);
}

void MeasurementPlotWidget::setData(const std::vector<double> &xData, const std::vector<double> &yData, 
                                    const QString &xTitle, const QString &yTitle)
{
    m_xData = xData;
    m_yData = yData;

    // Cache suffix / units for tracking
    int xSpaceIdx = xTitle.lastIndexOf('(');
    int ySpaceIdx = yTitle.lastIndexOf('(');
    m_xUnit = (xSpaceIdx != -1) ? xTitle.mid(xSpaceIdx) : "";
    m_yUnit = (ySpaceIdx != -1) ? yTitle.mid(ySpaceIdx) : "";

    m_series->clear();
    
    if (xData.empty() || yData.empty() || xData.size() != yData.size()) {
        m_lblTracking->setText(tr("No Data"));
        return;
    }

    QList<QPointF> points;
    points.reserve(static_cast<qsizetype>(xData.size()));
    for (size_t i = 0; i < xData.size(); ++i) {
        points.append(QPointF(xData[i], yData[i]));
    }
    m_series->replace(points);

    // Calculate Bounds
    double minX = *std::min_element(xData.begin(), xData.end());
    double maxX = *std::max_element(xData.begin(), xData.end());
    double minY = *std::min_element(yData.begin(), yData.end());
    double maxY = *std::max_element(yData.begin(), yData.end());

    double yMargin = std::abs(maxY - minY) * 0.1;
    if (yMargin == 0.0) yMargin = 1.0;

    m_axisX->setRange(minX, maxX);
    m_axisY->setRange(minY - yMargin, maxY + yMargin);

    m_axisX->setTitleText(xTitle);
    m_axisY->setTitleText(yTitle);

    m_lblTracking->setText("");
}

void MeasurementPlotWidget::setMarkers(const std::vector<double> &markerFreqs) {
    m_markerSeries->clear();
    for (double fGhz : markerFreqs) {
        if (m_xData.empty()) continue;
        size_t closestIdx = 0;
        double minDiff = std::abs(m_xData[0] - fGhz);
        for (size_t i = 1; i < m_xData.size(); ++i) {
            double diff = std::abs(m_xData[i] - fGhz);
            if (diff < minDiff) {
                minDiff = diff;
                closestIdx = i;
            }
        }
        m_markerSeries->append(m_xData[closestIdx], m_yData[closestIdx]);
    }
}

void MeasurementPlotWidget::clearData() {
    m_series->clear();
    m_markerSeries->clear();
    m_xData.clear();
    m_yData.clear();
    m_lblTracking->setText("");
}

void MeasurementPlotWidget::onMouseMoved(const QPoint &pos) {
    updateTrackingLabel(pos);
}

void MeasurementPlotWidget::updateTrackingLabel(const QPoint &pos) {
    if (!m_chart || m_xData.empty()) {
        m_lblTracking->setText("");
        return;
    }

    if (!m_chart->plotArea().contains(pos)) {
        m_lblTracking->setText("");
        return;
    }

    QPointF chartVal = m_chart->mapToValue(pos);
    double targetX = chartVal.x();

    // Binary search/snapping to nearest X value in sorted array
    size_t closestIdx = 0;
    double minDiff = std::abs(m_xData[0] - targetX);
    for (size_t i = 1; i < m_xData.size(); ++i) {
        double diff = std::abs(m_xData[i] - targetX);
        if (diff < minDiff) {
            minDiff = diff;
            closestIdx = i;
        }
    }

    double xVal = m_xData[closestIdx];
    double yVal = m_yData[closestIdx];

    m_lblTracking->setText(QString("%1 %2 | %3 %4")
                           .arg(xVal, 0, 'f', 3)
                           .arg(m_xUnit.trimmed())
                           .arg(yVal, 0, 'f', 2)
                           .arg(m_yUnit.trimmed()));
}

void MeasurementPlotWidget::onResetViewClicked() {
    m_chart->zoomReset();
}

void MeasurementPlotWidget::onExportClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Plot as PNG"), "", tr("PNG Image (*.png)"));
    
    if (fileName.isEmpty()) return;

    QPixmap pixmap = m_chartView->grab();
    if (pixmap.save(fileName, "PNG")) {
        QMessageBox::information(this, tr("Export Success"), 
            tr("Plot successfully exported to '%1'").arg(QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::warning(this, tr("Export Failure"), 
            tr("Failed to export plot image."));
    }
}

} // namespace AMAS
