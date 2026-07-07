#include "MainWindow.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QTime>
#include <QStatusBar>

#include "DashboardPage.h"
#include "DevicesPage.h"
#include "MeasurementSetupPage.h"
#include "MeasurementProgressPage.h"
#include "ResultsPage.h"
#include "ProfileManagerPage.h"
#include "presentation/DashboardPresenter.h"
#include "presentation/DevicesPresenter.h"
#include "presentation/SetupPresenter.h"
#include "presentation/ProgressPresenter.h"
#include "presentation/ResultsPresenter.h"
#include "presentation/ProfilePresenter.h"

namespace AMAS {

MainWindow::MainWindow(std::shared_ptr<MeasurementController> controller, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    m_presenter = new MeasurementPresenter(m_controller, this);
    setWindowTitle(tr("Antenna Measurement & Analysis Software (AMAS)"));
    resize(1200, 800);

    initializeFramework();
    createDockWidgets();
    createStatusBarWidget();
    createMenus();
    createToolBars();
    createCentralWidget();

    // Establish presenter status bar synchronization
    connect(m_presenter, &MeasurementPresenter::stateChanged, this, &MainWindow::updateStatusBar);
    connect(m_presenter, &MeasurementPresenter::currentProfileChanged, this, &MainWindow::updateStatusBar);
    connect(m_presenter, &MeasurementPresenter::deviceConnectionChanged, this, &MainWindow::updateStatusBar);
    updateStatusBar();

    // Restore state using WindowStateManager
    int activePage = 0;
    m_windowStateMgr->restoreState(this, activePage);
    if (activePage >= 0 && activePage < m_stackedWidget->count()) {
        m_stackedWidget->setCurrentIndex(activePage);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Save state using WindowStateManager
    m_windowStateMgr->saveState(this, m_stackedWidget->currentIndex());
    QMainWindow::closeEvent(event);
}

void MainWindow::initializeFramework() {
    m_settingsMgr = new SettingsManager(this);
    m_actionMgr = new ActionManager(this);
    m_commandMgr = new CommandManager(m_actionMgr, this);
    m_windowStateMgr = new WindowStateManager(m_settingsMgr, this);

    // Bind commands
    m_commandMgr->registerCommand(ActionId::FileExit, [this]() { close(); });
    m_commandMgr->registerCommand(ActionId::ViewDashboard, [this]() { showDashboard(); });
    m_commandMgr->registerCommand(ActionId::ViewDevices, [this]() { showDevices(); });
    m_commandMgr->registerCommand(ActionId::ViewProfiles, [this]() { showProfiles(); });
    m_commandMgr->registerCommand(ActionId::ViewMeasSetup, [this]() { showMeasurementSetup(); });
    m_commandMgr->registerCommand(ActionId::ViewMeasProgress, [this]() { showMeasurementProgress(); });
    m_commandMgr->registerCommand(ActionId::ViewResults, [this]() { showResults(); });
    m_commandMgr->registerCommand(ActionId::ViewSettings, [this]() { showSettings(); });

    m_commandMgr->registerCommand(ActionId::ViewToggleLogDock, [this]() {
        if (m_logDock) {
            m_logDock->setVisible(!m_logDock->isVisible());
        }
    });

    m_commandMgr->registerCommand(ActionId::HelpAbout, [this]() {
        QMessageBox::about(this, tr("About AMAS"),
            tr("<h3>Antenna Measurement & Analysis Software (AMAS)</h3>"
               "<p>Version 1.0.0</p>"
               "<p>A professional industrial software framework for antenna sweeps, "
               "processing, and automated radiation pattern extraction.</p>"));
    });

    // Logging closures for simulated commands
    auto logAction = [this](const QString &actionName) {
        return [this, actionName]() {
            m_dockLogEdit->append(tr("[%1] [USER] Triggered action: %2")
                                  .arg(QTime::currentTime().toString("hh:mm:ss"))
                                  .arg(actionName));
        };
    };

    m_commandMgr->registerCommand(ActionId::FileNewProfile, logAction(tr("New Profile")));
    m_commandMgr->registerCommand(ActionId::FileOpenProfile, logAction(tr("Open Profile")));
    m_commandMgr->registerCommand(ActionId::FileSaveProfile, logAction(tr("Save Profile")));
    m_commandMgr->registerCommand(ActionId::FileSaveProfileAs, logAction(tr("Save Profile As")));
    m_commandMgr->registerCommand(ActionId::FileImportProfile, logAction(tr("Import Profile")));
    m_commandMgr->registerCommand(ActionId::FileExportProfile, logAction(tr("Export Profile")));

    m_commandMgr->registerCommand(ActionId::DeviceConnect, logAction(tr("Connect Devices")));
    m_commandMgr->registerCommand(ActionId::DeviceDisconnect, logAction(tr("Disconnect Devices")));
    m_commandMgr->registerCommand(ActionId::DeviceRefresh, logAction(tr("Refresh Devices")));

    m_commandMgr->registerCommand(ActionId::MeasStart, [this]() {
        m_dockLogEdit->append(tr("[%1] [USER] Action: Start Measurement").arg(QTime::currentTime().toString("hh:mm:ss")));
        showMeasurementProgress();
        m_presenter->progress()->startMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasPause, [this]() {
        m_dockLogEdit->append(tr("[%1] [USER] Action: Pause Measurement").arg(QTime::currentTime().toString("hh:mm:ss")));
        m_presenter->progress()->pauseMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasResume, [this]() {
        m_dockLogEdit->append(tr("[%1] [USER] Action: Resume Measurement").arg(QTime::currentTime().toString("hh:mm:ss")));
        m_presenter->progress()->resumeMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasStop, [this]() {
        m_dockLogEdit->append(tr("[%1] [USER] Action: Stop Measurement").arg(QTime::currentTime().toString("hh:mm:ss")));
        m_presenter->progress()->stopMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasAbort, [this]() {
        m_dockLogEdit->append(tr("[%1] [USER] Action: Abort Measurement").arg(QTime::currentTime().toString("hh:mm:ss")));
        m_presenter->progress()->abortMeasurement();
    });

    m_commandMgr->registerCommand(ActionId::ToolsPreferences, logAction(tr("Preferences")));
    m_commandMgr->registerCommand(ActionId::ToolsCalManager, logAction(tr("Calibration Manager")));
    m_commandMgr->registerCommand(ActionId::HelpDoc, logAction(tr("Documentation")));
}

void MainWindow::createMenus() {
    // File
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileNewProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileOpenProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileSaveProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileSaveProfileAs));
    m_fileMenu->addSeparator();

    // Recent Profiles Menu
    m_recentMenu = m_fileMenu->addMenu(tr("&Recent Profiles"));
    QStringList mockProfiles = {
        "gain_horn_8_12.json",
        "gain_patch_12_18.json",
        "pattern_2d_horn.json",
        "pattern_3d_array.json",
        "s11_waveguide.json"
    };
    for (const QString &profile : mockProfiles) {
        auto *act = m_recentMenu->addAction(profile);
        connect(act, &QAction::triggered, this, [this, profile]() {
            m_dockLogEdit->append(tr("[%1] [USER] Loaded recent profile: %2")
                                  .arg(QTime::currentTime().toString("hh:mm:ss"))
                                  .arg(profile));
        });
    }

    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileImportProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileExportProfile));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileExit));

    // Devices
    m_devicesMenu = menuBar()->addMenu(tr("&Devices"));
    m_devicesMenu->addAction(m_actionMgr->getAction(ActionId::DeviceConnect));
    m_devicesMenu->addAction(m_actionMgr->getAction(ActionId::DeviceDisconnect));
    m_devicesMenu->addAction(m_actionMgr->getAction(ActionId::DeviceRefresh));

    // Measurements
    m_measMenu = menuBar()->addMenu(tr("&Measurements"));
    m_measMenu->addAction(m_actionMgr->getAction(ActionId::MeasStart));
    m_measMenu->addAction(m_actionMgr->getAction(ActionId::MeasPause));
    m_measMenu->addAction(m_actionMgr->getAction(ActionId::MeasResume));
    m_measMenu->addAction(m_actionMgr->getAction(ActionId::MeasStop));
    m_measMenu->addAction(m_actionMgr->getAction(ActionId::MeasAbort));

    // View
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewDashboard));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewDevices));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewProfiles));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewMeasSetup));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewMeasProgress));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewResults));
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewSettings));
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_actionMgr->getAction(ActionId::ViewToggleLogDock));

    // Tools
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_toolsMenu->addAction(m_actionMgr->getAction(ActionId::ToolsPreferences));
    m_toolsMenu->addAction(m_actionMgr->getAction(ActionId::ToolsCalManager));

    // Help
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_actionMgr->getAction(ActionId::HelpDoc));
    m_helpMenu->addAction(m_actionMgr->getAction(ActionId::HelpAbout));
}

void MainWindow::createToolBars() {
    QToolBar *toolbar = addToolBar(tr("Navigation Toolbar"));
    toolbar->setObjectName("MainNavigationToolbar"); // Set object name for restoring states
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewDashboard));
    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewDevices));
    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewProfiles));
    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewMeasSetup));
    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewMeasProgress));
    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewResults));
    toolbar->addAction(m_actionMgr->getAction(ActionId::ViewSettings));
}

void MainWindow::createCentralWidget() {
    m_stackedWidget = new QStackedWidget(this);

    // Index 0
    auto *dashboard = new DashboardPage(m_presenter->dashboard(), this);
    m_stackedWidget->addWidget(dashboard);

    // Index 1
    auto *devices = new DevicesPage(m_presenter->devices(), this);
    m_stackedWidget->addWidget(devices);

    // Index 2
    auto *setup = new MeasurementSetupPage(m_presenter->setup(), this);
    m_stackedWidget->addWidget(setup);
    connect(setup, &MeasurementSetupPage::startRequested, m_actionMgr->getAction(ActionId::MeasStart), &QAction::trigger);

    // Index 3
    auto *progress = new MeasurementProgressPage(m_presenter->progress(), this);
    m_stackedWidget->addWidget(progress);

    // Index 4
    auto *results = new ResultsPage(m_presenter->results(), this);
    m_stackedWidget->addWidget(results);

    // Index 5
    auto *profiles = new ProfileManagerPage(m_presenter->profiles(), this);
    m_stackedWidget->addWidget(profiles);
    connect(profiles, &ProfileManagerPage::editRequested, this, &MainWindow::showMeasurementSetup);

    // Index 6: Settings placeholder
    QStringList titles = { tr("Settings") };
    for (const QString& title : titles) {
        QWidget *page = new QWidget(m_stackedWidget);
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setAlignment(Qt::AlignCenter);

        QLabel *label = new QLabel(title, page);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 32px; font-weight: bold; color: #FFFFFF;");
        layout->addWidget(label);
        m_stackedWidget->addWidget(page);
    }

    setCentralWidget(m_stackedWidget);
}

void MainWindow::createDockWidgets() {
    m_logDock = new QDockWidget(tr("Application Log"), this);
    m_logDock->setObjectName("ApplicationLogDockWidget");
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_dockLogEdit = new QTextEdit(m_logDock);
    m_dockLogEdit->setReadOnly(true);
    m_dockLogEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1E1E1E; "
        "  border: none; "
        "  font-family: Consolas, 'Courier New', monospace; "
        "  font-size: 11px; "
        "  color: #C8C8C8; "
        "} "
    );

    m_logDock->setWidget(m_dockLogEdit);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);

    m_dockLogEdit->append(tr("[%1] [SYSTEM] AMAS Application Core Initialized.")
                          .arg(QTime::currentTime().toString("hh:mm:ss")));
}

void MainWindow::createStatusBarWidget() {
    m_lblUser = new QLabel(tr("User: Operator-01"), this);
    m_lblProject = new QLabel(tr("Project: AMAS_JV"), this);
    m_lblProfile = new QLabel(tr("Profile: Horn 8-12 GHz"), this);
    m_lblHardware = new QLabel(tr("Hardware: VNA Connected | Pos Offline"), this);
    m_lblAppState = new QLabel(tr("State: Offline Draft"), this);
    m_lblMemory = new QLabel(tr("RAM: 18.2 MB"), this);
    m_lblClock = new QLabel(QTime::currentTime().toString("hh:mm:ss"), this);

    statusBar()->addWidget(m_lblUser);
    statusBar()->addPermanentWidget(m_lblProject);
    statusBar()->addPermanentWidget(m_lblProfile);
    statusBar()->addPermanentWidget(m_lblHardware);
    statusBar()->addPermanentWidget(m_lblAppState);
    statusBar()->addPermanentWidget(m_lblMemory);
    statusBar()->addPermanentWidget(m_lblClock);

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    m_clockTimer->start(1000);
}

void MainWindow::updateClock() {
    m_lblClock->setText(QTime::currentTime().toString("hh:mm:ss"));
}

void MainWindow::showDashboard() { m_stackedWidget->setCurrentIndex(0); }
void MainWindow::showDevices() { m_stackedWidget->setCurrentIndex(1); }
void MainWindow::showMeasurementSetup() { m_stackedWidget->setCurrentIndex(2); }
void MainWindow::showMeasurementProgress() { m_stackedWidget->setCurrentIndex(3); }
void MainWindow::showResults() { m_stackedWidget->setCurrentIndex(4); }
void MainWindow::showProfiles() { m_stackedWidget->setCurrentIndex(5); }
void MainWindow::showSettings() { m_stackedWidget->setCurrentIndex(6); }

void MainWindow::updateStatusBar() {
    // 1. Active Profile
    QString profName = QString::fromStdString(m_presenter->currentProfile().profileName);
    m_lblProfile->setText(tr("Profile: %1").arg(profName.isEmpty() ? tr("None Selected") : profName));

    // 2. Hardware Status
    bool vna = m_presenter->controller()->isVnaConnected();
    bool pos = m_presenter->controller()->isPositionerConnected();
    QString hwStr;
    if (vna && pos) {
        hwStr = tr("VNA Connected | Pos Connected");
    } else if (vna) {
        hwStr = tr("VNA Connected | Pos Offline");
    } else if (pos) {
        hwStr = tr("VNA Offline | Pos Connected");
    } else {
        hwStr = tr("VNA Offline | Pos Offline");
    }
    m_lblHardware->setText(tr("Hardware: %1").arg(hwStr));

    // 3. Current Session
    QString sessName = QString::fromStdString(m_presenter->controller()->getLatestSession().sessionName);
    m_lblProject->setText(tr("Session: %1").arg(sessName.isEmpty() ? tr("None") : sessName));

    // 4. System State
    m_lblAppState->setText(tr("State: %1").arg(m_presenter->systemState()));
}

} // namespace AMAS
