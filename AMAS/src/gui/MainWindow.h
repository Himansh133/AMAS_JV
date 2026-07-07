#ifndef AMAS_MAINWINDOW_H
#define AMAS_MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QLabel>
#include <QDockWidget>
#include <QTextEdit>
#include <QTimer>
#include <QCloseEvent>

#include "framework/ActionManager.h"
#include "framework/CommandManager.h"
#include "framework/SettingsManager.h"
#include "framework/WindowStateManager.h"
#include "controllers/MeasurementController.h"
#include "presentation/MeasurementPresenter.h"

namespace AMAS {

class LogConsoleWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(std::shared_ptr<MeasurementController> controller, QWidget *parent = nullptr);
    ~MainWindow() override = default;

    SettingsManager* settingsManager() const { return m_settingsMgr; }

    // Navigation triggers
    void showDashboard();
    void showDevices();
    void showMeasurementSetup();
    void showMeasurementProgress();
    void showResults();
    void showProfiles();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initializeFramework();
    void createMenus();
    void createToolBars();
    void createCentralWidget();
    void createDockWidgets();
    void createStatusBarWidget();


    void showSettings();

    void logMessage(const QString &message, const QString &severity = "INFO", const QString &subsystem = "SYSTEM");

    // Clock updater
    void updateClock();
    void updateStatusBar();

private:
    void updateRecentSessionsMenu();
    void updateRecentProfilesMenu();
    void updateRecentReportsMenu();

    // Framework Managers
    ActionManager       *m_actionMgr;
    CommandManager      *m_commandMgr;
    SettingsManager     *m_settingsMgr;
    WindowStateManager  *m_windowStateMgr;

    // UI Components
    QStackedWidget *m_stackedWidget;
    QDockWidget    *m_logDock;
    LogConsoleWidget *m_logConsole;

    // Menu Bar Items
    QMenu *m_fileMenu;
    QMenu *m_recentMenu;
    QMenu *m_menuRecentSessions;
    QMenu *m_menuRecentProfiles;
    QMenu *m_menuRecentReports;
    QMenu *m_devicesMenu;
    QMenu *m_measMenu;
    QMenu *m_viewMenu;
    QMenu *m_toolsMenu;
    QMenu *m_helpMenu;

    // Status Bar Labels
    QLabel *m_lblUser;
    QLabel *m_lblProject;
    QLabel *m_lblProfile;
    QLabel *m_lblHardware;
    QLabel *m_lblAppState;
    QLabel *m_lblMemory;
    QLabel *m_lblClock;

    QTimer *m_clockTimer;

    // Presenter & Controller
    std::shared_ptr<MeasurementController> m_controller;
    MeasurementPresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_MAINWINDOW_H
