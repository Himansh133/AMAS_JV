#ifndef AMAS_SETTINGSPAGE_H
#define AMAS_SETTINGSPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include "framework/SettingsManager.h"
#include "presentation/MeasurementPresenter.h"

namespace AMAS {

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(SettingsManager *settingsMgr, MeasurementPresenter *presenter, QWidget *parent = nullptr);
    ~SettingsPage() override = default;

private slots:
    void onThemeChanged(const QString &themeName);
    void onGridToggled(bool checked);
    void onMarkersToggled(bool checked);
    void onBrowseExport();
    void onBrowseImport();
    void onClearHistory();

private:
    SettingsManager      *m_settingsMgr;
    MeasurementPresenter *m_presenter;

    QComboBox   *m_comboTheme;
    QCheckBox   *m_chkGrid;
    QCheckBox   *m_chkMarkers;
    QLineEdit   *m_editExportPath;
    QLineEdit   *m_editImportPath;
    QPushButton *m_btnBrowseExport;
    QPushButton *m_btnBrowseImport;
    QPushButton *m_btnClearHistory;
};

} // namespace AMAS

#endif // AMAS_SETTINGSPAGE_H
