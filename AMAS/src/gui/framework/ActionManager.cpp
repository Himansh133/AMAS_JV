#include "ActionManager.h"
#include <QApplication>
#include <QWidget>
#include <QStyle>

namespace AMAS {

ActionManager::ActionManager(QObject *parent)
    : QObject(parent)
{
    initializeActions();
}

QAction* ActionManager::getAction(ActionId id) const {
    return m_actions.value(id, nullptr);
}

void ActionManager::initializeActions() {
    // File
    createAction(ActionId::FileNewProfile, tr("&New Profile"), tr("Create a new measurement profile"), QKeySequence::New, QStyle::SP_FileDialogNewFolder);
    createAction(ActionId::FileOpenProfile, tr("&Open Profile..."), tr("Open an existing measurement profile"), QKeySequence::Open, QStyle::SP_DialogOpenButton);
    createAction(ActionId::FileSaveProfile, tr("&Save Profile"), tr("Save the active measurement profile"), QKeySequence::Save, QStyle::SP_DialogSaveButton);
    createAction(ActionId::FileSaveProfileAs, tr("Save Profile &As..."), tr("Save profile under a new filename"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), QStyle::SP_DialogSaveButton);
    createAction(ActionId::FileImportProfile, tr("&Import Profile..."), tr("Import profile configuration file"), QKeySequence(), QStyle::SP_DialogOpenButton);
    createAction(ActionId::FileExportProfile, tr("&Export Profile..."), tr("Export profile configuration file"), QKeySequence(), QStyle::SP_DialogSaveButton);
    createAction(ActionId::FileExit, tr("E&xit"), tr("Close the application"), QKeySequence(Qt::CTRL | Qt::Key_Q), QStyle::SP_DialogCancelButton);
    createAction(ActionId::EditUndo, tr("&Undo"), tr("Undo last action"), QKeySequence::Undo, QStyle::SP_ArrowBack);
    createAction(ActionId::EditRedo, tr("&Redo"), tr("Redo last action"), QKeySequence::Redo, QStyle::SP_ArrowForward);

    // Devices
    createAction(ActionId::DeviceConnect, tr("&Connect Devices"), tr("Connect all configured laboratory instruments"), QKeySequence(), QStyle::SP_DialogYesButton);
    createAction(ActionId::DeviceDisconnect, tr("&Disconnect Devices"), tr("Disconnect from all hardware"), QKeySequence(), QStyle::SP_DialogNoButton);
    createAction(ActionId::DeviceRefresh, tr("&Refresh Devices"), tr("Refresh instrument diagnostic connections"), QKeySequence(Qt::CTRL | Qt::Key_R), QStyle::SP_BrowserReload);

    // Measurement
    createAction(ActionId::MeasStart, tr("&Start Measurement"), tr("Start the configured coordinated sweep run"), QKeySequence(Qt::Key_F5), QStyle::SP_MediaPlay);
    createAction(ActionId::MeasPause, tr("&Pause"), tr("Pause the running measurement"), QKeySequence(Qt::Key_F10), QStyle::SP_MediaPause);
    createAction(ActionId::MeasResume, tr("&Resume"), tr("Resume the paused measurement"), QKeySequence(), QStyle::SP_MediaPlay);
    createAction(ActionId::MeasStop, tr("S&top"), tr("Gracefully stop measurement after current sweep"), QKeySequence(Qt::Key_F11), QStyle::SP_MediaStop);
    createAction(ActionId::MeasAbort, tr("&Abort"), tr("Immediately terminate measurement operations"), QKeySequence(), QStyle::SP_DialogCancelButton);

    // View
    createAction(ActionId::ViewDashboard, tr("&Dashboard"), tr("Show Dashboard"), QKeySequence(Qt::CTRL | Qt::Key_1), QStyle::SP_ComputerIcon);
    createAction(ActionId::ViewDevices, tr("&Devices"), tr("Manage laboratory instruments"), QKeySequence(Qt::CTRL | Qt::Key_2), QStyle::SP_FileDialogListView);
    createAction(ActionId::ViewProfiles, tr("&Profiles"), tr("Manage measurement configurations"), QKeySequence(Qt::CTRL | Qt::Key_3), QStyle::SP_FileDialogDetailedView);
    createAction(ActionId::ViewMeasSetup, tr("&Measurement Setup"), tr("Setup sweep variables"), QKeySequence(Qt::CTRL | Qt::Key_4), QStyle::SP_FileDialogListView);
    createAction(ActionId::ViewMeasProgress, tr("Measurement P&rogress"), tr("Monitor running sweeps"), QKeySequence(Qt::CTRL | Qt::Key_5), QStyle::SP_CommandLink);
    createAction(ActionId::ViewResults, tr("&Results"), tr("Post-processing results & analysis"), QKeySequence(Qt::CTRL | Qt::Key_6), QStyle::SP_DirIcon);
    createAction(ActionId::ViewSettings, tr("&Settings"), tr("Configure AMAS system settings"), QKeySequence(), QStyle::SP_FileDialogInfoView);
    createAction(ActionId::ViewToggleLogDock, tr("Toggle &Application Log"), tr("Show or hide the dock log panel"), QKeySequence(), QStyle::SP_FileDialogDetailedView);

    // Tools
    createAction(ActionId::ToolsPreferences, tr("&Preferences..."), tr("Open Preferences Dialog"), QKeySequence(), QStyle::SP_FileDialogInfoView);
    createAction(ActionId::ToolsCalManager, tr("&Calibration Manager..."), tr("Open Calibration Manager"), QKeySequence(), QStyle::SP_DialogSaveButton);
    createAction(ActionId::ToolsValidate, tr("&Validate Configuration"), tr("Validate sweep profile setup configuration"), QKeySequence(Qt::Key_F9), QStyle::SP_DialogApplyButton);

    // Help
    createAction(ActionId::HelpDoc, tr("&Documentation"), tr("Open AMAS user help documentation"), QKeySequence(), QStyle::SP_MessageBoxQuestion);
    createAction(ActionId::HelpAbout, tr("&About"), tr("About AMAS"), QKeySequence(), QStyle::SP_MessageBoxQuestion);
}

QAction* ActionManager::createAction(ActionId id, const QString &text, const QString &tooltip, const QKeySequence &shortcut, QStyle::StandardPixmap iconPixmap) {
    auto *act = new QAction(text, this);
    act->setToolTip(tooltip);
    act->setStatusTip(tooltip);
    
    if (!shortcut.isEmpty()) {
        act->setShortcut(shortcut);
    }
    
    // Set standard icon from application style context
    act->setIcon(qApp->style()->standardIcon(iconPixmap));

    m_actions.insert(id, act);
    return act;
}

} // namespace AMAS
