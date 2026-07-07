#include "SettingsManager.h"

namespace AMAS {

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_settings("Antigravity", "AMAS")
{
}

void SettingsManager::setValue(const QString &key, const QVariant &value) {
    m_settings.setValue(key, value);
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) const {
    return m_settings.value(key, defaultValue);
}

QStringList SettingsManager::getRecentFiles(const QString &key) const {
    return getValue(key).toStringList();
}

void SettingsManager::addRecentFile(const QString &key, const QString &filePath) {
    if (filePath.isEmpty()) return;
    QStringList list = getValue(key).toStringList();
    list.removeAll(filePath);
    list.prepend(filePath);
    while (list.size() > 10) {
        list.removeLast();
    }
    setValue(key, list);
    sync();
}

void SettingsManager::clearRecentFiles(const QString &key) {
    setValue(key, QStringList());
    sync();
}

void SettingsManager::sync() {
    m_settings.sync();
}

} // namespace AMAS
