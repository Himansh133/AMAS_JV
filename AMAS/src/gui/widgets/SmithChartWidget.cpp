#include "SmithChartWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFileDialog>
#include <QStyle>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AMAS {

SmithChartWidget::SmithChartWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Layout
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Header Toolbar
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

    // Set height stretch to push canvas down
    layout->addSpacing(4);
    
    // Add layout stretch instead of a child widget to allow transparent paint space
    layout->addStretch(1);

    // Connect slots
    connect(m_btnReset, &QPushButton::clicked, this, &SmithChartWidget::onResetViewClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &SmithChartWidget::onExportClicked);

    setMouseTracking(true);
}

QSize SmithChartWidget::sizeHint() const {
    return QSize(400, 400);
}

void SmithChartWidget::setSessionData(const std::vector<ProcessedMeasurementPoint> &points) {
    m_points = points;
    update();
}

void SmithChartWidget::setMarkers(const std::vector<double> &markerFreqs) {
    m_markerFreqs = markerFreqs;
    update();
}

void SmithChartWidget::clearData() {
    m_points.clear();
    m_markerFreqs.clear();
    m_lblTracking->setText("");
    update();
}

void SmithChartWidget::onResetViewClicked() {
    m_zoomFactor = 1.0;
    m_panOffset = QPointF(0, 0);
    update();
}

void SmithChartWidget::onExportClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Smith Chart as PNG"), "", tr("PNG Image (*.png)"));
    
    if (fileName.isEmpty()) return;

    QPixmap pixmap(size());
    render(&pixmap);
    if (pixmap.save(fileName, "PNG")) {
        QMessageBox::information(this, tr("Export Success"), 
            tr("Smith Chart successfully exported."));
    } else {
        QMessageBox::warning(this, tr("Export Failure"), 
            tr("Failed to save image."));
    }
}

QRect SmithChartWidget::getChartRect() const {
    // Leave space at top for toolbar
    int topOffset = 36;
    return QRect(0, topOffset, width(), height() - topOffset);
}

QPointF SmithChartWidget::mapToPixel(double re, double im, const QRect &chartRect) const {
    double centerX = chartRect.center().x() + m_panOffset.x();
    double centerY = chartRect.center().y() + m_panOffset.y();
    double radius = (std::min(chartRect.width(), chartRect.height()) / 2.0 - 20.0) * m_zoomFactor;
    
    double px = centerX + re * radius;
    double py = centerY - im * radius;
    return QPointF(px, py);
}

QPointF SmithChartWidget::mapToValue(const QPoint &pixelPos, const QRect &chartRect) const {
    double centerX = chartRect.center().x() + m_panOffset.x();
    double centerY = chartRect.center().y() + m_panOffset.y();
    double radius = (std::min(chartRect.width(), chartRect.height()) / 2.0 - 20.0) * m_zoomFactor;
    if (radius == 0.0) radius = 1.0;

    double re = (pixelPos.x() - centerX) / radius;
    double im = (centerY - pixelPos.y()) / radius;
    return QPointF(re, im);
}

void SmithChartWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void SmithChartWidget::mouseMoveEvent(QMouseEvent *event) {
    m_hoverPos = event->pos();
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_panOffset += QPointF(delta.x(), delta.y());
        m_lastMousePos = event->pos();
        update();
        event->accept();
        return;
    }
    updateTrackingLabel(event->pos());
    QWidget::mouseMoveEvent(event);
}

void SmithChartWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void SmithChartWidget::wheelEvent(QWheelEvent *event) {
    double factor = (event->angleDelta().y() > 0) ? 1.15 : 0.85;
    m_zoomFactor = std::clamp(m_zoomFactor * factor, 0.2, 50.0);
    update();
    event->accept();
}

void SmithChartWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    onResetViewClicked();
    QWidget::mouseDoubleClickEvent(event);
}

void SmithChartWidget::updateTrackingLabel(const QPoint &pos) {
    if (m_points.empty()) {
        m_lblTracking->setText("");
        return;
    }

    QRect chartRect = getChartRect();
    double radius0 = (std::min(chartRect.width(), chartRect.height()) / 2.0 - 20.0) * m_zoomFactor;
    if (radius0 <= 0.0) return;

    QPointF pCenter = mapToPixel(0.0, 0.0, chartRect);
    double mouseDist = std::hypot(pos.x() - pCenter.x(), pos.y() - pCenter.y());
    
    // Check if mouse is within the outer boundary circle
    if (mouseDist > radius0 + 10.0) {
        m_lblTracking->setText("");
        return;
    }

    QPointF gammaHover = mapToValue(pos, chartRect);
    double targetRe = gammaHover.x();
    double targetIm = gammaHover.y();

    // Snap to nearest frequency trace point
    double minDiff = 9999.0;
    const ProcessedMeasurementPoint *closestPt = nullptr;

    for (const auto &p : m_points) {
        double r = std::pow(10.0, p.magnitudeDb / 20.0);
        double theta = p.phaseDeg * M_PI / 180.0;
        double ptRe = r * std::cos(theta);
        double ptIm = r * std::sin(theta);
        
        double diff = std::hypot(ptRe - targetRe, ptIm - targetIm);
        if (diff < minDiff) {
            minDiff = diff;
            closestPt = &p;
        }
    }

    if (closestPt) {
        double mag = closestPt->magnitudeDb;
        double phase = closestPt->phaseDeg;

        double r = std::pow(10.0, mag / 20.0);
        double theta = phase * M_PI / 180.0;
        double re = r * std::cos(theta);
        double im = r * std::sin(theta);

        // Convert reflection coefficient Gamma to impedance Z = R + jX
        // Z = Zo * (1 + Gamma) / (1 - Gamma)
        double denom = (1.0 - re) * (1.0 - re) + im * im;
        double R = 50.0; // Normalized Zo
        double X = 0.0;
        if (denom > 1e-9) {
            R = 50.0 * (1.0 - re * re - im * im) / denom;
            X = 50.0 * (2.0 * im) / denom;
        }

        m_lblTracking->setText(QString("%1 GHz | %2 %3%4j Ω")
                               .arg(closestPt->frequencyHz / 1e9, 0, 'f', 3)
                               .arg(R, 0, 'f', 1)
                               .arg((X >= 0) ? "+" : "")
                               .arg(X, 0, 'f', 1));
    }
}

void SmithChartWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect chartRect = getChartRect();
    double radius0 = (std::min(chartRect.width(), chartRect.height()) / 2.0 - 20.0) * m_zoomFactor;
    QPointF pCenter0 = mapToPixel(0.0, 0.0, chartRect);

    // Canvas background
    painter.fillRect(rect(), QColor("#1E1E1E"));
    
    // Circular chart background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#252526"));
    painter.drawEllipse(pCenter0, radius0, radius0);

    // Circular clip path for drawing resistance/reactance grids inside boundaries
    painter.save();
    QPainterPath clipPath;
    clipPath.addEllipse(pCenter0, radius0, radius0);
    painter.setClipPath(clipPath);

    // Grid pen
    QColor gridColor("#3F3F46");
    gridColor.setAlpha(160);
    QPen gridPen(gridColor);
    gridPen.setWidth(1);
    painter.setPen(gridPen);
    painter.setBrush(Qt::NoBrush);

    // Constant resistance circles
    std::vector<double> rValues = { 0.2, 0.5, 1.0, 2.0, 5.0 };
    for (double r : rValues) {
        double xc = r / (r + 1.0);
        double rad = 1.0 / (r + 1.0);
        QPointF pc = mapToPixel(xc, 0.0, chartRect);
        double pRad = rad * radius0;
        painter.drawEllipse(pc, pRad, pRad);
    }

    // Constant reactance arcs
    std::vector<double> xValues = { 0.2, 0.5, 1.0, 2.0, 5.0 };
    for (double x : xValues) {
        // Upper Reactance (positive)
        {
            double yc = 1.0 / x;
            double rad = 1.0 / x;
            QPointF pc = mapToPixel(1.0, yc, chartRect);
            double pRad = rad * radius0;
            painter.drawEllipse(pc, pRad, pRad);
        }
        // Lower Reactance (negative)
        {
            double yc = -1.0 / x;
            double rad = 1.0 / x;
            QPointF pc = mapToPixel(1.0, yc, chartRect);
            double pRad = rad * radius0;
            painter.drawEllipse(pc, pRad, pRad);
        }
    }

    // Horizontal centerline
    painter.drawLine(mapToPixel(-1.0, 0.0, chartRect), mapToPixel(1.0, 0.0, chartRect));

    painter.restore();

    // Outer circle boundary line
    QPen outerPen(QColor("#FFFFFF"));
    outerPen.setWidth(2);
    painter.setPen(outerPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(pCenter0, radius0, radius0);

    // Gracefully handle empty or invalid dataset
    if (m_points.empty()) {
        painter.setPen(QColor("#C8C8C8"));
        painter.setFont(QFont("Segoe UI", 11));
        painter.drawText(chartRect, Qt::AlignCenter, tr("No complex S-parameter data available."));
        return;
    }

    // Plot Locus trace
    QPen tracePen(QColor("#E2B83E")); // Gold locus
    tracePen.setWidth(2);
    painter.setPen(tracePen);
    
    QPainterPath tracePath;
    bool first = true;
    for (const auto &p : m_points) {
        double r = std::pow(10.0, p.magnitudeDb / 20.0);
        double theta = p.phaseDeg * M_PI / 180.0;
        double re = r * std::cos(theta);
        double im = r * std::sin(theta);
        
        QPointF pPoint = mapToPixel(re, im, chartRect);
        if (first) {
            tracePath.moveTo(pPoint);
            first = false;
        } else {
            tracePath.lineTo(pPoint);
        }
    }
    painter.drawPath(tracePath);

    // Draw active markers
    int markerIdx = 1;
    for (double fHz : m_markerFreqs) {
        const ProcessedMeasurementPoint *mPoint = nullptr;
        double minDiff = 9999.0;
        for (const auto &p : m_points) {
            double diff = std::abs(p.frequencyHz - fHz);
            if (diff < minDiff) {
                minDiff = diff;
                mPoint = &p;
            }
        }

        if (mPoint) {
            double r = std::pow(10.0, mPoint->magnitudeDb / 20.0);
            double theta = mPoint->phaseDeg * M_PI / 180.0;
            double re = r * std::cos(theta);
            double im = r * std::sin(theta);
            QPointF pPoint = mapToPixel(re, im, chartRect);

            // Red square marker point
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#FF4C4C"));
            painter.drawRect(QRectF(pPoint.x() - 4, pPoint.y() - 4, 8, 8));

            // Marker label text
            painter.setPen(QColor("#FFFFFF"));
            painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
            painter.drawText(pPoint.x() + 8, pPoint.y() - 4, QString("M%1").arg(markerIdx));
            markerIdx++;
        }
    }
}

} // namespace AMAS
