#ifndef AMAS_WINDOWSTATEMANAGER_H
#define AMAS_WINDOWSTATEMANAGER_H

#include <QObject>
#include "SettingsManager.h"
#include <QMainWindow>
#include <QByteArray>

namespace AMAS {

class WindowStateManager : public QObject {
    Q_OBJECT
public:
    explicit WindowStateManager(SettingsManager *settingsMgr, QObject *parent = nullptr);
    ~WindowStateManager() override = default;

    // Serialize window states and position attributes
    void saveState(QMainWindow *mainWindow, int activePageIndex);
    void restoreState(QMainWindow *mainWindow, int &activePageIndex);

    // Save / Restore splitters separately
    void saveSplitterState(const QString &key, const QByteArray &state);
    QByteArray restoreSplitterState(const QString &key) const;

private:
    SettingsManager *m_settingsMgr;
};

} // namespace AMAS

#endif // AMAS_WINDOWSTATEMANAGER_H
