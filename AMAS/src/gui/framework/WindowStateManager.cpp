#include "WindowStateManager.h"

namespace AMAS {

WindowStateManager::WindowStateManager(SettingsManager *settingsMgr, QObject *parent)
    : QObject(parent)
    , m_settingsMgr(settingsMgr)
{
}

void WindowStateManager::saveState(QMainWindow *mainWindow, int activePageIndex) {
    if (!mainWindow) return;
    m_settingsMgr->setValue("Window/Geometry", mainWindow->saveGeometry());
    m_settingsMgr->setValue("Window/State", mainWindow->saveState());
    m_settingsMgr->setValue("Window/ActivePage", activePageIndex);
    m_settingsMgr->sync();
}

void WindowStateManager::restoreState(QMainWindow *mainWindow, int &activePageIndex) {
    if (!mainWindow) return;
    QByteArray geom = m_settingsMgr->getValue("Window/Geometry").toByteArray();
    if (!geom.isEmpty()) {
        mainWindow->restoreGeometry(geom);
    }
    
    QByteArray state = m_settingsMgr->getValue("Window/State").toByteArray();
    if (!state.isEmpty()) {
        mainWindow->restoreState(state);
    }
    
    activePageIndex = m_settingsMgr->getValue("Window/ActivePage", 0).toInt();
}

void WindowStateManager::saveSplitterState(const QString &key, const QByteArray &state) {
    m_settingsMgr->setValue(QString("Splitter/%1").arg(key), state);
}

QByteArray WindowStateManager::restoreSplitterState(const QString &key) const {
    return m_settingsMgr->getValue(QString("Splitter/%1").arg(key)).toByteArray();
}

} // namespace AMAS
