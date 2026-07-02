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

void SettingsManager::sync() {
    m_settings.sync();
}

} // namespace AMAS
