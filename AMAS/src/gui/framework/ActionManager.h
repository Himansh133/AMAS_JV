#ifndef AMAS_ACTIONMANAGER_H
#define AMAS_ACTIONMANAGER_H

#include <QObject>
#include <QAction>
#include <QMap>
#include <QStyle>

namespace AMAS {

enum class ActionId {
    // File
    FileNewProfile,
    FileOpenProfile,
    FileSaveProfile,
    FileSaveProfileAs,
    FileImportProfile,
    FileExportProfile,
    FileExit,

    // Devices
    DeviceConnect,
    DeviceDisconnect,
    DeviceRefresh,

    // Measurement
    MeasStart,
    MeasPause,
    MeasResume,
    MeasStop,
    MeasAbort,

    // View
    ViewDashboard,
    ViewDevices,
    ViewProfiles,
    ViewMeasSetup,
    ViewMeasProgress,
    ViewResults,
    ViewSettings,
    ViewToggleLogDock,

    // Tools
    ToolsPreferences,
    ToolsCalManager,

    // Help
    HelpDoc,
    HelpAbout
};

class ActionManager : public QObject {
    Q_OBJECT
public:
    explicit ActionManager(QObject *parent = nullptr);
    ~ActionManager() override = default;

    // Retrieve a registered action
    QAction* getAction(ActionId id) const;

private:
    void initializeActions();
    QAction* createAction(ActionId id, const QString &text, const QString &tooltip, const QKeySequence &shortcut, QStyle::StandardPixmap iconPixmap);

    QMap<ActionId, QAction*> m_actions;
};

} // namespace AMAS

#endif // AMAS_ACTIONMANAGER_H
