#include "DeviceStatusCard.h"
#include <QGridLayout>
#include <QHBoxLayout>

namespace AMAS {

DeviceStatusCard::DeviceStatusCard(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    // Configure default frame properties
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);
    
    // QSS styling to integrate with the dark theme
    setStyleSheet(
        "AMAS--DeviceStatusCard { "
        "  background-color: #2D2D30; "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 8px; "
        "} "
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // Title label
    m_lblTitle = new QLabel(title, this);
    m_lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #007ACC;");
    mainLayout->addWidget(m_lblTitle);

    // Status row (Indicator Circle + Status Text)
    auto *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(8);
    
    m_lblIndicator = new QLabel(this);
    m_lblIndicator->setFixedSize(10, 10);
    // Defaults to disconnected red
    m_lblIndicator->setStyleSheet("border-radius: 5px; background-color: #F44336;");
    statusLayout->addWidget(m_lblIndicator);

    m_lblStatus = new QLabel(tr("Disconnected"), this);
    m_lblStatus->setStyleSheet("font-weight: 500; color: #C8C8C8;");
    statusLayout->addWidget(m_lblStatus);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);

    // Grid for connection metadata
    auto *detailsLayout = new QGridLayout();
    detailsLayout->setSpacing(6);

    // Connection line
    auto *lblConnTag = new QLabel(tr("Connection:"), this);
    lblConnTag->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    m_lblConnDetail = new QLabel(tr("N/A"), this);
    m_lblConnDetail->setStyleSheet("color: #FFFFFF;");
    detailsLayout->addWidget(lblConnTag, 0, 0);
    detailsLayout->addWidget(m_lblConnDetail, 0, 1);

    // Extra line (e.g. Firmware or Slave ID)
    m_lblExtraLabel = new QLabel(tr("Detail:"), this);
    m_lblExtraLabel->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    m_lblExtraValue = new QLabel(tr("N/A"), this);
    m_lblExtraValue->setStyleSheet("color: #FFFFFF;");
    detailsLayout->addWidget(m_lblExtraLabel, 1, 0);
    detailsLayout->addWidget(m_lblExtraValue, 1, 1);

    mainLayout->addLayout(detailsLayout);
    mainLayout->addStretch();
}

void DeviceStatusCard::setConnectionStatus(bool isConnected, const QString &statusText) {
    m_lblStatus->setText(statusText);
    if (isConnected) {
        m_lblIndicator->setStyleSheet("border-radius: 5px; background-color: #4CAF50;");
        m_lblStatus->setStyleSheet("font-weight: 500; color: #FFFFFF;");
    } else {
        m_lblIndicator->setStyleSheet("border-radius: 5px; background-color: #F44336;");
        m_lblStatus->setStyleSheet("font-weight: 500; color: #C8C8C8;");
    }
}

void DeviceStatusCard::setDetails(const QString &connectionDetail, const QString &extraLabel, const QString &extraValue) {
    m_lblConnDetail->setText(connectionDetail);
    m_lblExtraLabel->setText(extraLabel);
    m_lblExtraValue->setText(extraValue);
}

} // namespace AMAS
