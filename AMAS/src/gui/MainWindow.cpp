#include "MainWindow.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QTime>
#include <QStatusBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include "SettingsPage.h"
#include "HelpDialogs.h"
#include "widgets/LogConsoleWidget.h"

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

    // Auto save settings, active profile and session on close
    m_settingsMgr->sync();

    // Auto save active profile
    MeasurementProfile prof = m_presenter->currentProfile();
    if (!prof.profileName.empty()) {
        m_presenter->controller()->saveProfile(prof);
    }

    // Auto save active session
    MeasurementSession sess = m_presenter->controller()->getLatestSession();
    if (!sess.sessionName.empty()) {
        QDir dir;
        dir.mkpath("Sessions");
        QString filePath = QString("Sessions/%1_session.txt").arg(QString::fromStdString(sess.sessionName));
        sess.serialize(filePath.toStdString());
    }

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
        AboutDialog dlg(this);
        dlg.exec();
    });

    m_commandMgr->registerCommand(ActionId::HelpDoc, [this]() {
        UserManualDialog dlg(this);
        dlg.exec();
    });

    // Wire Undo/Redo commands to QUndoStack
    m_commandMgr->registerCommand(ActionId::EditUndo, [this]() {
        m_presenter->undoStack()->undo();
    });
    m_commandMgr->registerCommand(ActionId::EditRedo, [this]() {
        m_presenter->undoStack()->redo();
    });

    QAction *undoAct = m_actionMgr->getAction(ActionId::EditUndo);
    QAction *redoAct = m_actionMgr->getAction(ActionId::EditRedo);
    if (undoAct && redoAct) {
        undoAct->setEnabled(m_presenter->undoStack()->canUndo());
        redoAct->setEnabled(m_presenter->undoStack()->canRedo());

        connect(m_presenter->undoStack(), &QUndoStack::canUndoChanged, undoAct, &QAction::setEnabled);
        connect(m_presenter->undoStack(), &QUndoStack::canRedoChanged, redoAct, &QAction::setEnabled);

        connect(m_presenter->undoStack(), &QUndoStack::undoTextChanged, this, [undoAct](const QString &text) {
            undoAct->setText(tr("&Undo %1").arg(text));
        });
        connect(m_presenter->undoStack(), &QUndoStack::redoTextChanged, this, [redoAct](const QString &text) {
            redoAct->setText(tr("&Redo %1").arg(text));
        });
    }

    // Logging closures for simulated commands
    auto logAction = [this](const QString &actionName) {
        return [this, actionName]() {
            logMessage(tr("Triggered action: %1").arg(actionName), "INFO", "USER");
        };
    };

    m_commandMgr->registerCommand(ActionId::FileNewProfile, [this]() {
        showMeasurementSetup();
        MeasurementSetupPage *setupPage = findChild<MeasurementSetupPage*>();
        if (setupPage) {
            MeasurementProfile blank;
            blank.profileName = "NewProfile";
            m_controller->setActiveProfile(blank);
            setupPage->onProfileLoaded(blank);
            logMessage(tr("Created a new profile 'NewProfile'"), "INFO", "USER");
        }
    });

    m_commandMgr->registerCommand(ActionId::FileOpenProfile, [this]() {
        showMeasurementSetup();
        MeasurementSetupPage *setupPage = findChild<MeasurementSetupPage*>();
        if (setupPage) {
            QString fileName = QFileDialog::getOpenFileName(this, tr("Open Profile Configuration"), "", tr("Profile Files (*.txt *.json)"));
            if (!fileName.isEmpty()) {
                MeasurementProfile p;
                if (p.loadFromFile(fileName.toStdString())) {
                    m_controller->setActiveProfile(p);
                    setupPage->onProfileLoaded(p);
                    m_settingsMgr->addRecentFile("Recent/Profiles", QString::fromStdString(p.profileName));
                    logMessage(tr("Loaded profile configuration '%1'").arg(QString::fromStdString(p.profileName)), "INFO", "USER");
                } else {
                    QMessageBox::warning(this, tr("Open Profile"), tr("Failed to load selected profile."));
                    logMessage(tr("Failed to load profile configuration from file '%1'").arg(fileName), "ERROR", "USER");
                }
            }
        }
    });

    m_commandMgr->registerCommand(ActionId::FileSaveProfile, [this]() {
        showMeasurementSetup();
        MeasurementSetupPage *setupPage = findChild<MeasurementSetupPage*>();
        if (setupPage) {
            setupPage->onSaveClicked();
        }
    });

    m_commandMgr->registerCommand(ActionId::FileSaveProfileAs, [this]() {
        showMeasurementSetup();
        MeasurementSetupPage *setupPage = findChild<MeasurementSetupPage*>();
        if (setupPage) {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save Profile Configuration As"), "", tr("Profile Files (*.txt *.json)"));
            if (!fileName.isEmpty()) {
                MeasurementProfile p = m_presenter->currentProfile();
                if (p.saveToFile(fileName.toStdString())) {
                    m_settingsMgr->addRecentFile("Recent/Profiles", QString::fromStdString(p.profileName));
                    logMessage(tr("Saved profile configuration as '%1'").arg(fileName), "INFO", "USER");
                } else {
                    QMessageBox::warning(this, tr("Save Profile As"), tr("Failed to save active profile."));
                }
            }
        }
    });

    m_commandMgr->registerCommand(ActionId::FileImportProfile, logAction(tr("Import Profile")));
    m_commandMgr->registerCommand(ActionId::FileExportProfile, logAction(tr("Export Profile")));

    m_commandMgr->registerCommand(ActionId::DeviceConnect, logAction(tr("Connect Devices")));
    m_commandMgr->registerCommand(ActionId::DeviceDisconnect, logAction(tr("Disconnect Devices")));
    m_commandMgr->registerCommand(ActionId::DeviceRefresh, logAction(tr("Refresh Devices")));

    m_commandMgr->registerCommand(ActionId::MeasStart, [this]() {
        logMessage(tr("Action: Start Measurement"), "INFO", "USER");
        showMeasurementProgress();
        m_presenter->progress()->startMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasPause, [this]() {
        logMessage(tr("Action: Pause Measurement"), "WARNING", "USER");
        m_presenter->progress()->pauseMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasResume, [this]() {
        logMessage(tr("Action: Resume Measurement"), "INFO", "USER");
        m_presenter->progress()->resumeMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasStop, [this]() {
        logMessage(tr("Action: Stop Measurement"), "WARNING", "USER");
        m_presenter->progress()->stopMeasurement();
    });
    m_commandMgr->registerCommand(ActionId::MeasAbort, [this]() {
        logMessage(tr("Action: Abort Measurement"), "ERROR", "USER");
        m_presenter->progress()->abortMeasurement();
    });

    m_commandMgr->registerCommand(ActionId::ToolsPreferences, [this]() {
        showSettings();
    });
    m_commandMgr->registerCommand(ActionId::ToolsCalManager, logAction(tr("Calibration Manager")));
    m_commandMgr->registerCommand(ActionId::ToolsValidate, [this]() {
        showMeasurementSetup();
        MeasurementSetupPage *setupPage = findChild<MeasurementSetupPage*>();
        if (setupPage) {
            setupPage->onValidateClicked();
        }
    });
}

void MainWindow::createMenus() {
    // File
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileNewProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileOpenProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileSaveProfile));
    m_fileMenu->addAction(m_actionMgr->getAction(ActionId::FileSaveProfileAs));
    m_fileMenu->addSeparator();

    // Recent Items Submenus
    m_menuRecentSessions = m_fileMenu->addMenu(tr("&Recent Sessions"));
    m_menuRecentProfiles = m_fileMenu->addMenu(tr("Recent &Profiles"));
    m_menuRecentReports = m_fileMenu->addMenu(tr("Recent &Reports"));

    connect(m_menuRecentSessions, &QMenu::aboutToShow, this, &MainWindow::updateRecentSessionsMenu);
    connect(m_menuRecentProfiles, &QMenu::aboutToShow, this, &MainWindow::updateRecentProfilesMenu);
    connect(m_menuRecentReports, &QMenu::aboutToShow, this, &MainWindow::updateRecentReportsMenu);

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

    // Index 6: Settings Page
    auto *settingsPage = new SettingsPage(m_settingsMgr, m_presenter, this);
    m_stackedWidget->addWidget(settingsPage);

    setCentralWidget(m_stackedWidget);
}

void MainWindow::createDockWidgets() {
    m_logDock = new QDockWidget(tr("Application Log"), this);
    m_logDock->setObjectName("ApplicationLogDockWidget");
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_logConsole = new LogConsoleWidget(m_logDock);
    m_logDock->setWidget(m_logConsole);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);

    logMessage(tr("AMAS Application Core Initialized."), "SUCCESS", "SYSTEM");
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

void MainWindow::logMessage(const QString &message, const QString &severity, const QString &subsystem) {
    if (m_logConsole) {
        m_logConsole->addLog(message, severity, subsystem);
    }
}

void MainWindow::updateRecentSessionsMenu() {
    m_menuRecentSessions->clear();
    QStringList files = m_settingsMgr->getRecentFiles("Recent/Sessions");
    if (files.isEmpty()) {
        auto *act = m_menuRecentSessions->addAction(tr("No Recent Sessions"));
        act->setEnabled(false);
        return;
    }
    for (const QString &file : files) {
        auto *act = m_menuRecentSessions->addAction(QFileInfo(file).fileName());
        connect(act, &QAction::triggered, this, [this, file]() {
            showResults();
            ResultsPage *resPage = findChild<ResultsPage*>();
            if (resPage) {
                MeasurementSession sess;
                if (m_presenter->results()->loadSession(QFileInfo(file).fileName(), sess)) {
                    resPage->loadSessionToUI(sess);
                }
            }
        });
    }
}

void MainWindow::updateRecentProfilesMenu() {
    m_menuRecentProfiles->clear();
    QStringList files = m_settingsMgr->getRecentFiles("Recent/Profiles");
    if (files.isEmpty()) {
        auto *act = m_menuRecentProfiles->addAction(tr("No Recent Profiles"));
        act->setEnabled(false);
        return;
    }
    for (const QString &file : files) {
        auto *act = m_menuRecentProfiles->addAction(file); // profileName is stored as is
        connect(act, &QAction::triggered, this, [this, file]() {
            showMeasurementSetup();
            MeasurementSetupPage *setupPage = findChild<MeasurementSetupPage*>();
            if (setupPage) {
                auto profiles = m_controller->getProfiles();
                for (const auto &p : profiles) {
                    if (QString::fromStdString(p.profileName) == file) {
                        m_controller->setActiveProfile(p);
                        setupPage->onProfileLoaded(p);
                        break;
                    }
                }
            }
        });
    }
}

void MainWindow::updateRecentReportsMenu() {
    m_menuRecentReports->clear();
    QStringList files = m_settingsMgr->getRecentFiles("Recent/Reports");
    if (files.isEmpty()) {
        auto *act = m_menuRecentReports->addAction(tr("No Recent Reports"));
        act->setEnabled(false);
        return;
    }
    for (const QString &file : files) {
        auto *act = m_menuRecentReports->addAction(QFileInfo(file).fileName());
        connect(act, &QAction::triggered, this, [file]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(file));
        });
    }
}

} // namespace AMAS
