#include "PolarPlotWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFileDialog>
#include <QStyle>
#include <QMessageBox>
#include <cmath>
#include <algorithm>
#include <set>

namespace AMAS {

// ==========================================
// CustomPolarView Implementation
// ==========================================

CustomPolarView::CustomPolarView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setRenderHint(QPainter::Antialiasing);
}

void CustomPolarView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        m_lastMousePos = event->pos();
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void CustomPolarView::mouseMoveEvent(QMouseEvent *event) {
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

void CustomPolarView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QChartView::mouseReleaseEvent(event);
}

void CustomPolarView::wheelEvent(QWheelEvent *event) {
    qreal factor = (event->angleDelta().y() > 0) ? 0.9 : 1.1;
    chart()->zoom(factor);
    event->accept();
}

void CustomPolarView::mouseDoubleClickEvent(QMouseEvent *event) {
    emit doubleClicked();
    QChartView::mouseDoubleClickEvent(event);
}


// ==========================================
// PolarPlotWidget Implementation
// ==========================================

PolarPlotWidget::PolarPlotWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    // Vertical Layout
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Header Toolbar
    auto *headerLayout = new QHBoxLayout();

    m_lblTitle = new QLabel(title, this);
    m_lblTitle->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 13px;");
    headerLayout->addWidget(m_lblTitle);

    headerLayout->addSpacing(16);

    m_comboTrace = new QComboBox(this);
    m_comboTrace->addItems({
        tr("Magnitude Pattern"),
        tr("Phase Pattern")
    });
    m_comboTrace->setStyleSheet(
        "QComboBox { background-color: #2D2D30; border: 1px solid #3F3F46; color: #FFFFFF; font-size: 11px; padding: 2px 4px; }"
    );
    headerLayout->addWidget(m_comboTrace);

    m_comboFreq = new QComboBox(this);
    m_comboFreq->setStyleSheet(
        "QComboBox { background-color: #2D2D30; border: 1px solid #3F3F46; color: #FFFFFF; font-size: 11px; padding: 2px 4px; }"
    );
    headerLayout->addWidget(m_comboFreq);

    headerLayout->addStretch();

    m_lblTracking = new QLabel("", this);
    m_lblTracking->setStyleSheet("color: #4CAF50; font-family: monospace; font-size: 11px; font-weight: bold; margin-right: 12px;");
    headerLayout->addWidget(m_lblTracking);

    m_btnReset = new QPushButton(tr("Reset"), this);
    m_btnReset->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_btnReset->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #DCDCDC; padding: 2px 8px; font-size: 11px; } "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnReset);

    m_btnExport = new QPushButton(tr("PNG"), this);
    m_btnExport->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnExport->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #DCDCDC; padding: 2px 8px; font-size: 11px; } "
        "QPushButton:hover { background-color: #3F3F46; } "
    );
    headerLayout->addWidget(m_btnExport);

    layout->addLayout(headerLayout);

    // Setup Polar Chart
    m_chart = new QPolarChart();
    m_chart->legend()->hide();
    m_chart->setBackgroundBrush(QBrush(QColor("#1E1E1E")));
    m_chart->setPlotAreaBackgroundBrush(QBrush(QColor("#252526")));
    m_chart->setPlotAreaBackgroundVisible(true);
    m_chart->setMargins(QMargins(8, 8, 8, 8));

    // Lines & Markers
    m_series = new QLineSeries(m_chart);
    m_series->setColor(QColor("#007ACC")); // Blue
    m_chart->addSeries(m_series);

    m_markerSeries = new QScatterSeries(m_chart);
    m_markerSeries->setColor(QColor("#FF4C4C")); // Red
    m_markerSeries->setMarkerSize(8.0);
    m_chart->addSeries(m_markerSeries);

    // Configure Axes
    m_axisAngular = new QValueAxis(m_chart);
    m_axisAngular->setRange(0, 360);
    m_axisAngular->setTickCount(9); // 45 degree steps
    m_axisAngular->setLabelFormat("%d");
    m_axisAngular->setGridLineColor(QColor("#3F3F46"));
    m_axisAngular->setLabelsColor(QColor("#C8C8C8"));
    m_chart->addAxis(m_axisAngular, QPolarChart::PolarOrientationAngular);

    m_axisRadial = new QValueAxis(m_chart);
    m_axisRadial->setGridLineColor(QColor("#3F3F46"));
    m_axisRadial->setLabelsColor(QColor("#C8C8C8"));
    m_axisRadial->setTitleBrush(QBrush(QColor("#FFFFFF")));
    m_chart->addAxis(m_axisRadial, QPolarChart::PolarOrientationRadial);

    m_series->attachAxis(m_axisAngular);
    m_series->attachAxis(m_axisRadial);
    m_markerSeries->attachAxis(m_axisAngular);
    m_markerSeries->attachAxis(m_axisRadial);

    // Chart View
    m_chartView = new CustomPolarView(m_chart, this);
    m_chartView->setStyleSheet("background-color: #1E1E1E; border: 1px solid #3F3F46;");
    layout->addWidget(m_chartView, 1);

    // Connect handlers
    connect(m_comboTrace, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PolarPlotWidget::onTraceTypeChanged);
    connect(m_comboFreq, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PolarPlotWidget::onFrequencyChanged);
    connect(m_chartView, &CustomPolarView::mouseMoved, this, &PolarPlotWidget::onMouseMoved);
    connect(m_chartView, &CustomPolarView::doubleClicked, this, &PolarPlotWidget::onResetViewClicked);
    connect(m_btnReset, &QPushButton::clicked, this, &PolarPlotWidget::onResetViewClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &PolarPlotWidget::onExportClicked);
}

void PolarPlotWidget::setSessionData(const std::vector<ProcessedMeasurementPoint> &points) {
    m_points = points;
    m_uniqueFrequencies.clear();

    // Collect sorted unique frequencies
    std::set<double> freqSet;
    for (const auto &p : points) {
        freqSet.insert(p.frequencyHz);
    }
    m_uniqueFrequencies.assign(freqSet.begin(), freqSet.end());

    m_comboFreq->blockSignals(true);
    m_comboFreq->clear();
    for (double f : m_uniqueFrequencies) {
        m_comboFreq->addItem(tr("%1 GHz").arg(f / 1e9, 0, 'f', 3));
    }
    m_comboFreq->blockSignals(false);

    if (!m_uniqueFrequencies.empty()) {
        m_selectedFreqHz = m_uniqueFrequencies.front();
        m_comboFreq->setCurrentIndex(0);
    } else {
        m_selectedFreqHz = 0.0;
    }

    updatePlot();
}

void PolarPlotWidget::setMarkers(const std::vector<QPointF> &markerPoints) {
    m_markerSeries->clear();
    for (const auto &pt : markerPoints) {
        double angle = pt.x();
        if (angle < 0.0) angle += 360.0;
        m_markerSeries->append(angle, pt.y());
    }
}

void PolarPlotWidget::clearData() {
    m_points.clear();
    m_uniqueFrequencies.clear();
    m_comboFreq->clear();
    m_series->clear();
    m_markerSeries->clear();
    m_lblTracking->setText("");
}

void PolarPlotWidget::onTraceTypeChanged(int index) {
    m_showMagnitude = (index == 0);
    updatePlot();
}

void PolarPlotWidget::onFrequencyChanged(int index) {
    if (index >= 0 && index < static_cast<int>(m_uniqueFrequencies.size())) {
        m_selectedFreqHz = m_uniqueFrequencies[index];
        updatePlot();
    }
}

void PolarPlotWidget::updatePlot() {
    m_series->clear();
    if (m_points.empty() || m_selectedFreqHz == 0.0) {
        return;
    }

    // Filter points matching the selected frequency
    std::vector<QPointF> plotPoints;
    for (const auto &p : m_points) {
        if (std::abs(p.frequencyHz - m_selectedFreqHz) < 1.0) { // floating point margin
            double angle = p.angleDeg;
            if (angle < 0.0) angle += 360.0;
            double radius = m_showMagnitude ? p.magnitudeDb : p.phaseDeg;
            plotPoints.push_back(QPointF(angle, radius));
        }
    }

    // Sort by angle to draw a clean circular line response without crisscrossing
    std::sort(plotPoints.begin(), plotPoints.end(), [](const QPointF &a, const QPointF &b) {
        return a.x() < b.x();
    });

    if (plotPoints.empty()) return;

    for (const auto &pt : plotPoints) {
        m_series->append(pt.x(), pt.y());
    }

    // Calculate Bounds
    double minR = plotPoints[0].y();
    double maxR = plotPoints[0].y();
    for (const auto &pt : plotPoints) {
        if (pt.y() < minR) minR = pt.y();
        if (pt.y() > maxR) maxR = pt.y();
    }

    double margin = std::abs(maxR - minR) * 0.15;
    if (margin == 0.0) margin = 2.0;

    m_axisRadial->setRange(minR - margin, maxR + margin);
    m_axisRadial->setTitleText(m_showMagnitude ? tr("Magnitude (dB)") : tr("Phase (deg)"));
}

void PolarPlotWidget::onMouseMoved(const QPoint &pos) {
    updateTrackingLabel(pos);
}

void PolarPlotWidget::updateTrackingLabel(const QPoint &pos) {
    if (!m_chart || m_points.empty() || m_selectedFreqHz == 0.0) {
        m_lblTracking->setText("");
        return;
    }

    if (!m_chart->plotArea().contains(pos)) {
        m_lblTracking->setText("");
        return;
    }

    QPointF chartVal = m_chart->mapToValue(pos);
    double targetAngle = chartVal.x(); // 0 to 360

    // Match closest angle point in the loaded dataset
    double minDiff = 9999.0;
    const ProcessedMeasurementPoint *closestPt = nullptr;

    for (const auto &p : m_points) {
        if (std::abs(p.frequencyHz - m_selectedFreqHz) < 1.0) {
            double angle = p.angleDeg;
            if (angle < 0.0) angle += 360.0;
            double diff = std::abs(angle - targetAngle);
            if (diff < minDiff) {
                minDiff = diff;
                closestPt = &p;
            }
        }
    }

    if (closestPt) {
        double val = m_showMagnitude ? closestPt->magnitudeDb : closestPt->phaseDeg;
        QString unit = m_showMagnitude ? "dB" : "deg";
        m_lblTracking->setText(QString("%1° | %2 %3 | %4 GHz")
                               .arg(closestPt->angleDeg, 0, 'f', 1)
                               .arg(val, 0, 'f', 2)
                               .arg(unit)
                               .arg(m_selectedFreqHz / 1e9, 0, 'f', 3));
    }
}

void PolarPlotWidget::onResetViewClicked() {
    m_chart->zoomReset();
}

void PolarPlotWidget::onExportClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Polar Plot as PNG"), "", tr("PNG Image (*.png)"));
    
    if (fileName.isEmpty()) return;

    QPixmap pixmap = m_chartView->grab();
    if (pixmap.save(fileName, "PNG")) {
        QMessageBox::information(this, tr("Export Success"), 
            tr("Polar plot successfully exported."));
    } else {
        QMessageBox::warning(this, tr("Export Failure"), 
            tr("Failed to save image."));
    }
}

} // namespace AMAS
