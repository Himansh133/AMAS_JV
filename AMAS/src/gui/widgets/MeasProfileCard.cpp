#include "MeasProfileCard.h"
#include <QVBoxLayout>
#include <QGridLayout>

namespace AMAS {

MeasProfileCard::MeasProfileCard(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);
    
    setStyleSheet(
        "AMAS--MeasProfileCard { "
        "  background-color: #2D2D30; "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 8px; "
        "} "
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // Title label
    m_lblTitle = new QLabel(tr("Active Measurement Profile"), this);
    m_lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #007ACC;");
    mainLayout->addWidget(m_lblTitle);

    // Profile detail grid
    auto *detailsLayout = new QGridLayout();
    detailsLayout->setSpacing(8);

    auto *lblBandTag = new QLabel(tr("RF Band:"), this);
    lblBandTag->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    m_lblBandVal = new QLabel(tr("8-12 GHz"), this);
    m_lblBandVal->setStyleSheet("color: #FFFFFF; font-weight: bold;");
    detailsLayout->addWidget(lblBandTag, 0, 0);
    detailsLayout->addWidget(m_lblBandVal, 0, 1);

    auto *lblCalTag = new QLabel(tr("Calibration:"), this);
    lblCalTag->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    m_lblCalVal = new QLabel(tr("Loaded"), this);
    m_lblCalVal->setStyleSheet("color: #4CAF50; font-weight: bold;"); // Green highlight
    detailsLayout->addWidget(lblCalTag, 1, 0);
    detailsLayout->addWidget(m_lblCalVal, 1, 1);

    auto *lblPointsTag = new QLabel(tr("Sweep Points:"), this);
    lblPointsTag->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    m_lblPointsVal = new QLabel(tr("401"), this);
    m_lblPointsVal->setStyleSheet("color: #FFFFFF;");
    detailsLayout->addWidget(lblPointsTag, 2, 0);
    detailsLayout->addWidget(m_lblPointsVal, 2, 1);

    mainLayout->addLayout(detailsLayout);
    mainLayout->addStretch();
}

void MeasProfileCard::setProfile(const QString &band, const QString &calibration, int points) {
    m_lblBandVal->setText(band);
    m_lblCalVal->setText(calibration);
    m_lblPointsVal->setText(QString::number(points));
    
    if (calibration.toLower() == "loaded") {
        m_lblCalVal->setStyleSheet("color: #4CAF50; font-weight: bold;");
    } else {
        m_lblCalVal->setStyleSheet("color: #F44336; font-weight: bold;");
    }
}

} // namespace AMAS
