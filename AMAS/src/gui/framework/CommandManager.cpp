#include "CommandManager.h"
#include <QAction>

namespace AMAS {

CommandManager::CommandManager(ActionManager *actionMgr, QObject *parent)
    : QObject(parent)
    , m_actionMgr(actionMgr)
{
}

void CommandManager::registerCommand(ActionId id, std::function<void()> command) {
    m_commands.insert(id, command);
    
    QAction *act = m_actionMgr->getAction(id);
    if (act) {
        m_actionToIdMap.insert(act, id);
        // Clean disconnect before connecting to prevent duplicate signals
        disconnect(act, &QAction::triggered, this, &CommandManager::onActionTriggered);
        connect(act, &QAction::triggered, this, &CommandManager::onActionTriggered);
    }
}

void CommandManager::onActionTriggered() {
    auto *act = qobject_cast<QAction*>(sender());
    if (act && m_actionToIdMap.contains(act)) {
        ActionId id = m_actionToIdMap.value(act);
        if (m_commands.contains(id)) {
            m_commands.value(id)();
        }
    }
}

} // namespace AMAS
