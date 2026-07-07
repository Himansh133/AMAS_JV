#include "DevicesPage.h"
#include "presentation/DevicesPresenter.h"
#include <QTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QStyle>
#include <QScrollBar>

namespace AMAS {

DevicesPage::DevicesPage(DevicesPresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_presenter(presenter)
{
    // Main layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title Block
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);
    
    auto *lblTitle = new QLabel(tr("Devices"), this);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(lblTitle);

    auto *lblSubtitle = new QLabel(tr("Manage and configure all connected measurement instruments."), this);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #C8C8C8;");
    headerLayout->addWidget(lblSubtitle);
    mainLayout->addLayout(headerLayout);

    // Two-Column Content Layout
    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // Left Column: Device Control Panels
    auto *leftColLayout = new QVBoxLayout();
    leftColLayout->setSpacing(20);

    auto *grpVna = new QGroupBox(tr("FieldFox N9951B (VNA)"), this);
    createFieldFoxPanel(grpVna);
    leftColLayout->addWidget(grpVna);

    auto *grpPos = new QGroupBox(tr("TAP-3001 (Positioner)"), this);
    createPositionerPanel(grpPos);
    leftColLayout->addWidget(grpPos);
    
    leftColLayout->addStretch();
    contentLayout->addLayout(leftColLayout, 3); // 3x weight

    // Right Column: Summary & Logging
    auto *rightColLayout = new QVBoxLayout();
    rightColLayout->setSpacing(20);

    auto *grpSummary = new QGroupBox(tr("Connection Summary"), this);
    createSummaryPanel(grpSummary);
    rightColLayout->addWidget(grpSummary);

    auto *grpLog = new QGroupBox(tr("Communication Log"), this);
    createLogPanel(grpLog);
    rightColLayout->addWidget(grpLog);

    rightColLayout->addStretch();
    contentLayout->addLayout(rightColLayout, 2); // 2x weight

    mainLayout->addLayout(contentLayout);

    // Bind action callbacks
    connect(m_btnVnaConnect, &QPushButton::clicked, this, &DevicesPage::onConnectClicked);
    connect(m_btnPosConnect, &QPushButton::clicked, this, &DevicesPage::onConnectClicked);
    connect(m_btnVnaDisconnect, &QPushButton::clicked, this, &DevicesPage::onDisconnectClicked);
    connect(m_btnPosDisconnect, &QPushButton::clicked, this, &DevicesPage::onDisconnectClicked);
    connect(m_btnVnaRefresh, &QPushButton::clicked, this, &DevicesPage::onRefreshClicked);
    connect(m_btnPosRefresh, &QPushButton::clicked, this, &DevicesPage::onRefreshClicked);
    connect(m_btnVnaIdentify, &QPushButton::clicked, this, &DevicesPage::onIdentifyVnaClicked);
    connect(m_btnPosHome, &QPushButton::clicked, this, [this]() { m_presenter->homePositioner(); });
    connect(m_btnPosMove, &QPushButton::clicked, this, [this]() { m_presenter->movePositioner(45.0f); });

    connect(m_presenter, &DevicesPresenter::connectionStatusChanged, this, &DevicesPage::updateConnectionStatus);
    connect(m_presenter, &DevicesPresenter::messageLogged, this, &DevicesPage::appendLogMessage);

    // Load initial connection states
    updateConnectionStatus(m_presenter->isVnaConnected(), m_presenter->isPositionerConnected());
}

void DevicesPage::createFieldFoxPanel(QGroupBox *box) {
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // Grid details
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight);

    auto *lblType = new QLabel(tr("Ethernet (VISA / VXI-11)"), this);
    lblType->setStyleSheet("color: #FFFFFF;");
    formLayout->addRow(tr("Connection Type:"), lblType);

    m_lblVnaResource = new QLabel(m_presenter->vnaResource(), this);
    m_lblVnaResource->setStyleSheet("color: #FFFFFF; font-family: Consolas, monospace;");
    formLayout->addRow(tr("VISA Resource:"), m_lblVnaResource);

    m_lblVnaDeviceName = new QLabel(m_presenter->vnaDeviceName(), this);
    m_lblVnaDeviceName->setStyleSheet("color: #FFFFFF;");
    formLayout->addRow(tr("Device Name:"), m_lblVnaDeviceName);

    auto *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(6);
    m_indVnaStatus = new QLabel(this);
    m_indVnaStatus->setFixedSize(10, 10);
    m_indVnaStatus->setStyleSheet("border-radius: 5px; background-color: #F44336;"); // Red by default
    m_lblVnaStatus = new QLabel(tr("Disconnected"), this);
    m_lblVnaStatus->setStyleSheet("font-weight: bold; color: #C8C8C8;");
    statusLayout->addWidget(m_indVnaStatus);
    statusLayout->addWidget(m_lblVnaStatus);
    statusLayout->addStretch();
    formLayout->addRow(tr("Status:"), statusLayout);

    layout->addLayout(formLayout);

    // Actions Button Bar
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnVnaConnect = new QPushButton(tr("Connect"), this);
    m_btnVnaConnect->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    m_btnVnaConnect->setMinimumHeight(32);
    
    m_btnVnaDisconnect = new QPushButton(tr("Disconnect"), this);
    m_btnVnaDisconnect->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    m_btnVnaDisconnect->setMinimumHeight(32);

    m_btnVnaRefresh = new QPushButton(tr("Refresh"), this);
    m_btnVnaRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_btnVnaRefresh->setMinimumHeight(32);

    m_btnVnaIdentify = new QPushButton(tr("Identify"), this);
    m_btnVnaIdentify->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    m_btnVnaIdentify->setMinimumHeight(32);

    btnLayout->addWidget(m_btnVnaConnect);
    btnLayout->addWidget(m_btnVnaDisconnect);
    btnLayout->addWidget(m_btnVnaRefresh);
    btnLayout->addWidget(m_btnVnaIdentify);
    layout->addLayout(btnLayout);
}

void DevicesPage::createPositionerPanel(QGroupBox *box) {
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight);

    m_lblPosPort = new QLabel(m_presenter->positionerPort(), this);
    m_lblPosPort->setStyleSheet("color: #FFFFFF;");
    formLayout->addRow(tr("COM Port:"), m_lblPosPort);

    m_lblPosDeviceName = new QLabel(m_presenter->positionerDeviceName(), this);
    m_lblPosDeviceName->setStyleSheet("color: #FFFFFF;");
    formLayout->addRow(tr("Device Name:"), m_lblPosDeviceName);

    auto *lblBaud = new QLabel(tr("19200 bps (8N1)"), this);
    lblBaud->setStyleSheet("color: #FFFFFF;");
    formLayout->addRow(tr("Settings:"), lblBaud);

    auto *lblSlave = new QLabel(tr("1"), this);
    lblSlave->setStyleSheet("color: #FFFFFF;");
    formLayout->addRow(tr("Slave ID:"), lblSlave);

    auto *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(6);
    m_indPosStatus = new QLabel(this);
    m_indPosStatus->setFixedSize(10, 10);
    m_indPosStatus->setStyleSheet("border-radius: 5px; background-color: #F44336;"); // Red
    m_lblPosStatus = new QLabel(tr("Disconnected"), this);
    m_lblPosStatus->setStyleSheet("font-weight: bold; color: #C8C8C8;");
    statusLayout->addWidget(m_indPosStatus);
    statusLayout->addWidget(m_lblPosStatus);
    statusLayout->addStretch();
    formLayout->addRow(tr("Status:"), statusLayout);

    layout->addLayout(formLayout);

    // Actions Button Bar
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnPosConnect = new QPushButton(tr("Connect"), this);
    m_btnPosConnect->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    m_btnPosConnect->setMinimumHeight(32);

    m_btnPosDisconnect = new QPushButton(tr("Disconnect"), this);
    m_btnPosDisconnect->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    m_btnPosDisconnect->setMinimumHeight(32);

    m_btnPosHome = new QPushButton(tr("Home"), this);
    m_btnPosHome->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));
    m_btnPosHome->setMinimumHeight(32);

    m_btnPosRefresh = new QPushButton(tr("Refresh"), this);
    m_btnPosRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_btnPosRefresh->setMinimumHeight(32);

    m_btnPosMove = new QPushButton(tr("Move..."), this);
    m_btnPosMove->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    m_btnPosMove->setMinimumHeight(32);

    btnLayout->addWidget(m_btnPosConnect);
    btnLayout->addWidget(m_btnPosDisconnect);
    btnLayout->addWidget(m_btnPosHome);
    btnLayout->addWidget(m_btnPosRefresh);
    btnLayout->addWidget(m_btnPosMove);
    layout->addLayout(btnLayout);
}

void DevicesPage::createSummaryPanel(QGroupBox *box) {
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    layout->addWidget(createStatusRow(tr("VNA State:"), m_indVna, m_valVna, false, tr("Connected"), tr("Disconnected")));
    layout->addWidget(createStatusRow(tr("Positioner State:"), m_indPos, m_valPos, false, tr("Connected"), tr("Disconnected")));
    layout->addWidget(createStatusRow(tr("Measurement Ready:"), m_indReady, m_valReady, false, tr("Yes"), tr("No")));
}

void DevicesPage::createLogPanel(QGroupBox *box) {
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(12, 16, 12, 12);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(150);
    m_logEdit->setMaximumHeight(200);

    // Apply clean dark terminal styling
    m_logEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1E1E1E; "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 4px; "
        "  font-family: Consolas, 'Courier New', monospace; "
        "  font-size: 12px; "
        "  color: #C8C8C8; "
        "  line-height: 18px; "
        "} "
    );

    // Insert log lines
    m_logEdit->append(tr("[14:00:00] [INFO ] AMAS Application started successfully."));
    m_logEdit->append(tr("[14:00:01] [INFO ] Theme system initialized. Visual styles loaded."));
    m_logEdit->append(tr("[14:00:02] [INFO ] Dashboard initialized with placeholder states."));
    m_logEdit->append(tr("[14:05:12] [INFO ] FieldFox detected at 192.168.113.206. Pinging IDN..."));
    m_logEdit->append(tr("[14:05:13] [SUCCESS] FieldFox connected successfully."));
    m_logEdit->append(tr("[14:05:14] [WARN ] Positioner unavailable on COM3 (Serial open timeout)."));

    // Force scroll to bottom
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());

    layout->addWidget(m_logEdit);
}

QWidget* DevicesPage::createStatusRow(const QString &label, QLabel *&indicator, QLabel *&value, bool isActive, const QString &activeText, const QString &inactiveText) {
    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *lblText = new QLabel(label, row);
    lblText->setMinimumWidth(140);
    lblText->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    
    indicator = new QLabel(row);
    indicator->setFixedSize(10, 10);
    
    value = new QLabel(row);
    value->setStyleSheet("color: #FFFFFF; font-weight: bold;");

    if (isActive) {
        indicator->setStyleSheet("border-radius: 5px; background-color: #4CAF50;");
        value->setText(activeText);
    } else {
        indicator->setStyleSheet("border-radius: 5px; background-color: #F44336;");
        value->setText(inactiveText);
    }

    layout->addWidget(lblText);
    layout->addWidget(indicator);
    layout->addWidget(value);
    layout->addStretch();
    return row;
}

void DevicesPage::onConnectClicked() {
    m_presenter->connectHardware("GPIB0::18::INSTR", "COM3");
}

void DevicesPage::onDisconnectClicked() {
    m_presenter->disconnectHardware();
}

void DevicesPage::onRefreshClicked() {
    m_presenter->refreshDevices();
}

void DevicesPage::onIdentifyVnaClicked() {
    m_presenter->identifyVna();
}

void DevicesPage::updateConnectionStatus(bool vnaConnected, bool posConnected) {
    if (m_lblVnaResource) m_lblVnaResource->setText(m_presenter->vnaResource());
    if (m_lblVnaDeviceName) m_lblVnaDeviceName->setText(m_presenter->vnaDeviceName());
    if (m_lblPosPort) m_lblPosPort->setText(m_presenter->positionerPort());
    if (m_lblPosDeviceName) m_lblPosDeviceName->setText(m_presenter->positionerDeviceName());

    m_lblVnaStatus->setText(vnaConnected ? tr("Connected") : tr("Disconnected"));
    m_lblVnaStatus->setStyleSheet(vnaConnected ? "font-weight: bold; color: #4CAF50;" : "font-weight: bold; color: #C8C8C8;");
    m_btnVnaConnect->setEnabled(!vnaConnected);
    m_btnVnaDisconnect->setEnabled(vnaConnected);
    if (m_indVnaStatus) {
        m_indVnaStatus->setStyleSheet(vnaConnected ? "border-radius: 5px; background-color: #4CAF50;" : "border-radius: 5px; background-color: #F44336;");
    }

    m_lblPosStatus->setText(posConnected ? tr("Connected") : tr("Disconnected"));
    m_lblPosStatus->setStyleSheet(posConnected ? "font-weight: bold; color: #4CAF50;" : "font-weight: bold; color: #C8C8C8;");
    m_btnPosConnect->setEnabled(!posConnected);
    m_btnPosDisconnect->setEnabled(posConnected);
    if (m_indPosStatus) {
        m_indPosStatus->setStyleSheet(posConnected ? "border-radius: 5px; background-color: #4CAF50;" : "border-radius: 5px; background-color: #F44336;");
    }

    if (m_indVna && m_valVna) {
        m_indVna->setStyleSheet(vnaConnected ? "border-radius: 5px; background-color: #4CAF50;" : "border-radius: 5px; background-color: #F44336;");
        m_valVna->setText(vnaConnected ? tr("Connected") : tr("Disconnected"));
    }
    if (m_indPos && m_valPos) {
        m_indPos->setStyleSheet(posConnected ? "border-radius: 5px; background-color: #4CAF50;" : "border-radius: 5px; background-color: #F44336;");
        m_valPos->setText(posConnected ? tr("Connected") : tr("Disconnected"));
    }
    bool ready = vnaConnected && posConnected;
    if (m_indReady && m_valReady) {
        m_indReady->setStyleSheet(ready ? "border-radius: 5px; background-color: #4CAF50;" : "border-radius: 5px; background-color: #F44336;");
        m_valReady->setText(ready ? tr("Yes") : tr("No"));
    }
}

void DevicesPage::appendLogMessage(const QString &msg) {
    m_logEdit->append(tr("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg));
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
}

} // namespace AMAS
