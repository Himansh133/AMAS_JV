#include "DashboardPage.h"
#include "presentation/DashboardPresenter.h"
#include "presentation/ResultsPresenter.h"
#include "MainWindow.h"
#include "framework/SettingsManager.h"
#include "ResultsPage.h"
#include "MeasurementSetupPage.h"
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
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

    // Populate recent lists
    SettingsManager *mgr = getSettingsManager();
    if (mgr) {
        QStringList sessions = mgr->getRecentFiles("Recent/Sessions");
        QStringList profiles = mgr->getRecentFiles("Recent/Profiles");
        QStringList reports = mgr->getRecentFiles("Recent/Reports");

        m_tableRecent->setRowCount(0);
        m_tableRecent->setRowCount(10);

        for (int i = 0; i < 10; ++i) {
            auto setCell = [this, i](int col, const QString &text) {
                auto *item = new QTableWidgetItem(text);
                item->setTextAlignment(Qt::AlignCenter);
                m_tableRecent->setItem(i, col, item);
            };

            setCell(0, i < sessions.size() ? QFileInfo(sessions[i]).fileName() : "");
            setCell(1, i < profiles.size() ? profiles[i] : ""); // profile name is a key string
            setCell(2, i < reports.size() ? QFileInfo(reports[i]).fileName() : "");
        }
    }
}

void DashboardPage::setupQuickActions() {
    m_btnConnect = new QuickActionButton(QStyle::SP_ComputerIcon, tr("Connect Devices"), this);
    m_btnSetup   = new QuickActionButton(QStyle::SP_FileDialogListView, tr("Measurement Setup"), this);
    m_btnCal     = new QuickActionButton(QStyle::SP_DialogOpenButton, tr("Load Calibration"), this);
    m_btnResults = new QuickActionButton(QStyle::SP_DirOpenIcon, tr("Open Results"), this);
    m_btnReport  = new QuickActionButton(QStyle::SP_FileIcon, tr("Generate Report"), this);
}

void DashboardPage::setupRecentTable() {
    m_tableRecent = new QTableWidget(10, 3, this);
    m_tableRecent->setMinimumHeight(150);
    m_tableRecent->setMaximumHeight(200);
    m_tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableRecent->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableRecent->setAlternatingRowColors(true);
    
    // Set headers
    QStringList headers = { tr("Recent Sessions"), tr("Recent Profiles"), tr("Recent Reports") };
    m_tableRecent->setHorizontalHeaderLabels(headers);
    m_tableRecent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableRecent->verticalHeader()->setVisible(false);

    m_tableRecent->setStyleSheet(
        "QTableWidget { "
        "  background-color: #2D2D30; "
        "  alternate-background-color: #252526; "
        "  border: none; "
        "} "
    );

    // Double click to open/load
    connect(m_tableRecent, &QTableWidget::cellDoubleClicked, this, [this](int r, int c) {
        auto *item = m_tableRecent->item(r, c);
        if (!item || item->text().isEmpty()) return;

        SettingsManager *mgr = getSettingsManager();
        if (!mgr) return;

        MainWindow *win = qobject_cast<MainWindow*>(m_presenter->parentPresenter()->parent());
        if (!win) return;

        if (c == 0) { // Recent Session
            QStringList sessions = mgr->getRecentFiles("Recent/Sessions");
            if (r < sessions.size()) {
                QString fullPath = sessions[r];
                win->showResults();
                ResultsPage *resPage = win->findChild<ResultsPage*>();
                if (resPage) {
                    MeasurementSession sess;
                    if (m_presenter->parentPresenter()->results()->loadSession(QFileInfo(fullPath).fileName(), sess)) {
                        resPage->loadSessionToUI(sess);
                    }
                }
            }
        } else if (c == 1) { // Recent Profile
            QString profileName = item->text();
            win->showMeasurementSetup();
            MeasurementSetupPage *setupPage = win->findChild<MeasurementSetupPage*>();
            if (setupPage) {
                auto profiles = m_presenter->parentPresenter()->controller()->getProfiles();
                for (const auto &p : profiles) {
                    if (QString::fromStdString(p.profileName) == profileName) {
                        m_presenter->parentPresenter()->controller()->setActiveProfile(p);
                        setupPage->onProfileLoaded(p);
                        break;
                    }
                }
            }
        } else if (c == 2) { // Recent Report
            QStringList reports = mgr->getRecentFiles("Recent/Reports");
            if (r < reports.size()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(reports[r]));
            }
        }
    });
}

void DashboardPage::setupLogPanel() {
    m_logList = new QListWidget(this);
    m_logList->setMinimumHeight(100);
    m_logList->setMaximumHeight(130);
    m_logList->setStyleSheet("border: none; background-color: #2D2D30;");

    m_logList->addItem(tr("[14:00:00] [INFO ] AMAS Dashboard module initialized."));
    m_logList->scrollToBottom();
}

SettingsManager* DashboardPage::getSettingsManager() const {
    MainWindow *win = qobject_cast<MainWindow*>(m_presenter->parentPresenter()->parent());
    return win ? win->settingsManager() : nullptr;
}

} // namespace AMAS
