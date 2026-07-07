#ifndef AMAS_SETTINGSMANAGER_H
#define AMAS_SETTINGSMANAGER_H

#include <QObject>
#include <QVariant>
#include <QSettings>

namespace AMAS {

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager() override = default;

    // Read/write persistent configurations
    void setValue(const QString &key, const QVariant &value);
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;

    QStringList getRecentFiles(const QString &key) const;
    void addRecentFile(const QString &key, const QString &filePath);
    void clearRecentFiles(const QString &key);

    void sync();

private:
    QSettings m_settings;
};

} // namespace AMAS

#endif // AMAS_SETTINGSMANAGER_H
