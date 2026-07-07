#include "SettingsPage.h"
#include "theme/ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

namespace AMAS {

SettingsPage::SettingsPage(SettingsManager *settingsMgr, MeasurementPresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_settingsMgr(settingsMgr)
    , m_presenter(presenter)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    // Title Block
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);
    auto *lblTitle = new QLabel(tr("Application Settings"), this);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(lblTitle);
    auto *lblSubtitle = new QLabel(tr("Configure grid preferences, plot themes, default workspace directories, and clear settings cache."), this);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #C8C8C8;");
    headerLayout->addWidget(lblSubtitle);
    mainLayout->addLayout(headerLayout);

    // Group 1: UI & Plot Customization
    auto *grpVisual = new QGroupBox(tr("Visual Settings"), this);
    grpVisual->setMinimumHeight(150);
    auto *visualLayout = new QFormLayout(grpVisual);
    visualLayout->setSpacing(12);

    m_comboTheme = new QComboBox(grpVisual);
    m_comboTheme->addItems({tr("Dark"), tr("Light"), tr("Classic Gray")});
    m_comboTheme->setStyleSheet("color: #FFFFFF; background-color: #2D2D30; border: 1px solid #3F3F46; padding: 4px; border-radius: 4px;");
    QString activeTheme = m_settingsMgr->getValue("Plot/Theme", "Dark").toString();
    m_comboTheme->setCurrentText(activeTheme);
    visualLayout->addRow(tr("Plot Theme:"), m_comboTheme);

    m_chkGrid = new QCheckBox(tr("Show Grid Lines on Plots"), grpVisual);
    bool gridVisible = m_settingsMgr->getValue("Plot/GridVisible", true).toBool();
    m_chkGrid->setChecked(gridVisible);
    visualLayout->addRow(tr("Grid Lines:"), m_chkGrid);

    m_chkMarkers = new QCheckBox(tr("Show Measurement Markers on Plots"), grpVisual);
    bool markersVisible = m_settingsMgr->getValue("Plot/MarkerVisible", true).toBool();
    m_chkMarkers->setChecked(markersVisible);
    visualLayout->addRow(tr("Markers:"), m_chkMarkers);

    mainLayout->addWidget(grpVisual);

    // Group 2: Paths & Folders
    auto *grpPaths = new QGroupBox(tr("Workspace & Import/Export Folders"), this);
    grpPaths->setMinimumHeight(150);
    auto *pathsLayout = new QFormLayout(grpPaths);
    pathsLayout->setSpacing(12);

    auto *exportRow = new QHBoxLayout();
    m_editExportPath = new QLineEdit(grpPaths);
    m_editExportPath->setStyleSheet("color: #FFFFFF; background-color: #2D2D30; border: 1px solid #3F3F46; padding: 4px; border-radius: 4px;");
    m_editExportPath->setText(m_settingsMgr->getValue("Dir/ExportFolder", "Reports").toString());
    m_editExportPath->setReadOnly(true);
    exportRow->addWidget(m_editExportPath);
    m_btnBrowseExport = new QPushButton(tr("Browse..."), grpPaths);
    exportRow->addWidget(m_btnBrowseExport);
    pathsLayout->addRow(tr("Default Reports Folder:"), exportRow);

    auto *importRow = new QHBoxLayout();
    m_editImportPath = new QLineEdit(grpPaths);
    m_editImportPath->setStyleSheet("color: #FFFFFF; background-color: #2D2D30; border: 1px solid #3F3F46; padding: 4px; border-radius: 4px;");
    m_editImportPath->setText(m_settingsMgr->getValue("Dir/ImportFolder", "Profiles").toString());
    m_editImportPath->setReadOnly(true);
    importRow->addWidget(m_editImportPath);
    m_btnBrowseImport = new QPushButton(tr("Browse..."), grpPaths);
    importRow->addWidget(m_btnBrowseImport);
    pathsLayout->addRow(tr("Default Profiles Folder:"), importRow);

    mainLayout->addWidget(grpPaths);

    // Group 3: History & Cache Reset
    auto *grpCache = new QGroupBox(tr("System History"), this);
    grpCache->setMinimumHeight(100);
    auto *cacheLayout = new QHBoxLayout(grpCache);
    cacheLayout->setContentsMargins(16, 16, 16, 16);
    m_btnClearHistory = new QPushButton(tr("Clear Recent Files History"), grpCache);
    m_btnClearHistory->setIcon(qApp->style()->standardIcon(QStyle::SP_TrashIcon));
    m_btnClearHistory->setStyleSheet("font-weight: bold; background-color: #F44336; color: white; padding: 6px 12px; border-radius: 4px;");
    cacheLayout->addWidget(m_btnClearHistory);
    cacheLayout->addStretch();
    mainLayout->addWidget(grpCache);

    mainLayout->addStretch();

    // Connect signals
    connect(m_comboTheme, &QComboBox::currentTextChanged, this, &SettingsPage::onThemeChanged);
    connect(m_chkGrid, &QCheckBox::toggled, this, &SettingsPage::onGridToggled);
    connect(m_chkMarkers, &QCheckBox::toggled, this, &SettingsPage::onMarkersToggled);
    connect(m_btnBrowseExport, &QPushButton::clicked, this, &SettingsPage::onBrowseExport);
    connect(m_btnBrowseImport, &QPushButton::clicked, this, &SettingsPage::onBrowseImport);
    connect(m_btnClearHistory, &QPushButton::clicked, this, &SettingsPage::onClearHistory);
}

void SettingsPage::onThemeChanged(const QString &themeName) {
    m_settingsMgr->setValue("Plot/Theme", themeName);
    m_settingsMgr->sync();
    
    // Apply globally
    ThemeManager::applyTheme(qApp, themeName);
    QMessageBox::information(this, tr("Theme Switched"), tr("Plot Theme changed to '%1'. Please restart if any stylesheet updates do not display immediately.").arg(themeName));
}

void SettingsPage::onGridToggled(bool checked) {
    m_settingsMgr->setValue("Plot/GridVisible", checked);
    m_settingsMgr->sync();
}

void SettingsPage::onMarkersToggled(bool checked) {
    m_settingsMgr->setValue("Plot/MarkerVisible", checked);
    m_settingsMgr->sync();
}

void SettingsPage::onBrowseExport() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Default Reports Export Folder"), m_editExportPath->text());
    if (!dir.isEmpty()) {
        m_editExportPath->setText(dir);
        m_settingsMgr->setValue("Dir/ExportFolder", dir);
        m_settingsMgr->sync();
    }
}

void SettingsPage::onBrowseImport() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Default Profiles Import Folder"), m_editImportPath->text());
    if (!dir.isEmpty()) {
        m_editImportPath->setText(dir);
        m_settingsMgr->setValue("Dir/ImportFolder", dir);
        m_settingsMgr->sync();
    }
}

void SettingsPage::onClearHistory() {
    auto reply = QMessageBox::question(this, tr("Clear History"), tr("Are you sure you want to clear the recent profiles, sessions, and reports lists?"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_settingsMgr->clearRecentFiles("Recent/Sessions");
        m_settingsMgr->clearRecentFiles("Recent/Profiles");
        m_settingsMgr->clearRecentFiles("Recent/Reports");
        QMessageBox::information(this, tr("Success"), tr("Recent files history has been cleared successfully."));
    }
}

} // namespace AMAS
