#include "DashboardPage.h"
#include "presentation/DashboardPresenter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>

namespace AMAS {

DashboardPage::DashboardPage(DashboardPresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_presenter(presenter)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Dashboard Title Header
    auto *lblTitle = new QLabel(tr("Dashboard"), this);
    lblTitle->setStyleSheet(
        "font-size: 24px; "
        "font-weight: bold; "
        "color: #FFFFFF; "
        "margin-bottom: 5px;"
    );
    mainLayout->addWidget(lblTitle);

    // 1. Assemble Status Card Row (Top Row)
    setupTopRow();
    auto *topRowLayout = new QHBoxLayout();
    topRowLayout->setSpacing(16);
    topRowLayout->addWidget(m_cardVna);
    topRowLayout->addWidget(m_cardPositioner);
    topRowLayout->addWidget(m_cardProfile);
    mainLayout->addLayout(topRowLayout);

    // 2. Assemble Quick Actions Row (Second Row)
    setupQuickActions();
    auto *grpActions = new QGroupBox(tr("Quick Actions"), this);
    auto *actionsLayout = new QHBoxLayout(grpActions);
    actionsLayout->setContentsMargins(16, 16, 16, 16);
    actionsLayout->setSpacing(16);
    actionsLayout->addWidget(m_btnConnect);
    actionsLayout->addWidget(m_btnSetup);
    actionsLayout->addWidget(m_btnCal);
    actionsLayout->addWidget(m_btnResults);
    actionsLayout->addWidget(m_btnReport);
    mainLayout->addWidget(grpActions);

    // 3. Assemble Recent Measurements Table (Third Row)
    setupRecentTable();
    auto *grpRecent = new QGroupBox(tr("Recent Measurements"), this);
    auto *recentLayout = new QVBoxLayout(grpRecent);
    recentLayout->setContentsMargins(12, 16, 12, 12);
    recentLayout->addWidget(m_tableRecent);
    mainLayout->addWidget(grpRecent);

    // 4. Assemble System Log Panel (Fourth Row)
    setupLogPanel();
    auto *grpLog = new QGroupBox(tr("System Log"), this);
    auto *logLayout = new QVBoxLayout(grpLog);
    logLayout->setContentsMargins(12, 16, 12, 12);
    logLayout->addWidget(m_logList);
    mainLayout->addWidget(grpLog);

    // Initial load and signal connection
    updateDashboardState();
    connect(m_presenter, &DashboardPresenter::dashboardUpdated, this, &DashboardPage::updateDashboardState);
}

void DashboardPage::setupTopRow() {
    // VNA Card Setup (Connected)
    m_cardVna = new DeviceStatusCard(tr("FieldFox N9951B"), this);
    m_cardVna->setConnectionStatus(true, tr("Connected"));
    m_cardVna->setDetails(tr("TCPIP0::192.168.113.206::inst0::INSTR"), tr("Firmware:"), tr("A.10.15"));

    // Positioner Card Setup (Disconnected)
    m_cardPositioner = new DeviceStatusCard(tr("TAP-3001 Positioner"), this);
    m_cardPositioner->setConnectionStatus(false, tr("Disconnected"));
    m_cardPositioner->setDetails(tr("COM3"), tr("Slave ID:"), tr("1"));

    // Profile Card Setup
    m_cardProfile = new MeasProfileCard(this);
    m_cardProfile->setProfile(tr("None Selected"), tr("Inactive"), 0);
}

void DashboardPage::updateDashboardState() {
    bool vnaConnected = m_presenter->isVnaConnected();
    m_cardVna->setConnectionStatus(vnaConnected, vnaConnected ? tr("Connected") : tr("Disconnected"));
    m_cardVna->setDetails(m_presenter->vnaResource(), tr("Firmware:"), m_presenter->vnaFirmware());

    bool posConnected = m_presenter->isPositionerConnected();
    m_cardPositioner->setConnectionStatus(posConnected, posConnected ? tr("Connected") : tr("Disconnected"));
    m_cardPositioner->setDetails(m_presenter->positionerPort(), tr("Slave ID:"), m_presenter->positionerSlaveId());

    m_cardProfile->setProfile(m_presenter->currentProfileName(), m_presenter->calibrationStatus(), m_presenter->currentProfilePoints());

    m_logList->clear();
    m_logList->addItem(tr("Software Version: %1").arg(m_presenter->applicationVersion()));
    m_logList->addItem(tr("Connected Device Count: %1").arg(m_presenter->connectedDeviceCount()));
    m_logList->addItem(tr("System Ready Status: %1").arg(m_presenter->isSystemReady() ? tr("Ready") : tr("Not Ready")));
    m_logList->addItem(tr("Measurement State: %1").arg(m_presenter->systemState()));
    m_logList->addItem(tr("Current Measurement Profile: %1").arg(m_presenter->currentProfileName()));
    m_logList->addItem(tr("Calibration Status: %1").arg(m_presenter->calibrationStatus()));
    m_logList->addItem(tr("Active Session: %1").arg(m_presenter->activeSession()));
    m_logList->addItem(tr("Last Measurement Time: %1").arg(m_presenter->lastMeasurementTime()));
}

void DashboardPage::setupQuickActions() {
    m_btnConnect = new QuickActionButton(QStyle::SP_ComputerIcon, tr("Connect Devices"), this);
    m_btnSetup   = new QuickActionButton(QStyle::SP_FileDialogListView, tr("Measurement Setup"), this);
    m_btnCal     = new QuickActionButton(QStyle::SP_DialogOpenButton, tr("Load Calibration"), this);
    m_btnResults = new QuickActionButton(QStyle::SP_DirOpenIcon, tr("Open Results"), this);
    m_btnReport  = new QuickActionButton(QStyle::SP_FileIcon, tr("Generate Report"), this);
}

void DashboardPage::setupRecentTable() {
    m_tableRecent = new QTableWidget(3, 5, this);
    m_tableRecent->setMinimumHeight(120);
    m_tableRecent->setMaximumHeight(150);
    m_tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableRecent->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableRecent->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableRecent->setAlternatingRowColors(true);
    
    // Set headers
    QStringList headers = { tr("Time"), tr("Measurement"), tr("RF Band"), tr("Status"), tr("Result") };
    m_tableRecent->setHorizontalHeaderLabels(headers);
    m_tableRecent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableRecent->verticalHeader()->setVisible(false);

    // QSS formatting for borders
    m_tableRecent->setStyleSheet(
        "QTableWidget { "
        "  background-color: #2D2D30; "
        "  alternate-background-color: #252526; "
        "  border: none; "
        "} "
    );

    // Mock data rows
    auto setCell = [this](int r, int c, const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        m_tableRecent->setItem(r, c, item);
    };

    setCell(0, 0, tr("2026-07-01 10:15:30"));
    setCell(0, 1, tr("Horn Antenna Pattern"));
    setCell(0, 2, tr("8-12 GHz"));
    setCell(0, 3, tr("Completed"));
    setCell(0, 4, tr("12.4 dBi (Peak)"));

    setCell(1, 0, tr("2026-07-01 11:22:12"));
    setCell(1, 1, tr("Microstrip Patch S11"));
    setCell(1, 2, tr("12-18 GHz"));
    setCell(1, 3, tr("Completed"));
    setCell(1, 4, tr("-18.5 dB (RL)"));

    setCell(2, 0, tr("2026-07-01 13:40:05"));
    setCell(2, 1, tr("Dipole Radiation Sweep"));
    setCell(2, 2, tr("1.8-2.4 GHz"));
    setCell(2, 3, tr("Aborted"));
    setCell(2, 4, tr("N/A"));
}

void DashboardPage::setupLogPanel() {
    m_logList = new QListWidget(this);
    m_logList->setMinimumHeight(100);
    m_logList->setMaximumHeight(130);
    m_logList->setStyleSheet("border: none; background-color: #2D2D30;");

    // Add some log placeholders
    m_logList->addItem(tr("[14:00:00] [INFO ] AMAS Application started successfully."));
    m_logList->addItem(tr("[14:00:01] [INFO ] Custom dark theme stylesheet applied."));
    m_logList->addItem(tr("[14:00:02] [INFO ] Dashboard initialized with placeholder hardware states."));
    m_logList->addItem(tr("[14:00:05] [WARN ] Positioner connection offline. COM3 port unavailable."));
    
    // Auto scroll to bottom
    m_logList->scrollToBottom();
}

} // namespace AMAS
