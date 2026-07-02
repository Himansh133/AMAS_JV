#ifndef AMAS_COMMANDMANAGER_H
#define AMAS_COMMANDMANAGER_H

#include <QObject>
#include <functional>
#include <QMap>
#include "ActionManager.h"

namespace AMAS {

class CommandManager : public QObject {
    Q_OBJECT
public:
    explicit CommandManager(ActionManager *actionMgr, QObject *parent = nullptr);
    ~CommandManager() override = default;

    // Map a callback command execution to an action trigger
    void registerCommand(ActionId id, std::function<void()> command);

private slots:
    void onActionTriggered();

private:
    ActionManager *m_actionMgr;
    QMap<ActionId, std::function<void()>> m_commands;
    QMap<QAction*, ActionId> m_actionToIdMap;
};

} // namespace AMAS

#endif // AMAS_COMMANDMANAGER_H
