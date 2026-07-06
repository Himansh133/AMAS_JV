#include "ProfileManagerPage.h"
#include "presentation/ProfilePresenter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QStyle>
#include <QSettings>
#include <QInputDialog>

namespace AMAS {

ProfileManagerPage::ProfileManagerPage(ProfilePresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_presenter(presenter)
{
    // Main root layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    // Title Block
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);

    auto *lblTitle = new QLabel(tr("Profiles"), this);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(lblTitle);

    auto *lblSubtitle = new QLabel(tr("Manage and configure reusable measurement configuration profiles."), this);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #C8C8C8;");
    headerLayout->addWidget(lblSubtitle);
    mainLayout->addLayout(headerLayout);

    // Splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(6);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #3F3F46; }");

    // Left Panel (Explorer Container)
    auto *explorerContainer = new QWidget(m_splitter);
    createExplorerPanel(explorerContainer);
    m_splitter->addWidget(explorerContainer);

    // Right Panel (Workspace Container)
    auto *workspaceContainer = new QWidget(m_splitter);
    createWorkspacePanel(workspaceContainer);
    m_splitter->addWidget(workspaceContainer);

    // Set initial weights
    m_splitter->setStretchFactor(0, 2);
    m_splitter->setStretchFactor(1, 3);

    // Restore state using QSettings
    QSettings settings("Antigravity", "AMAS");
    QByteArray state = settings.value("ProfileManagerPage/SplitterState").toByteArray();
    if (!state.isEmpty()) {
        m_splitter->restoreState(state);
    }

    // Connect explorer signals
    connect(m_treeProfiles, &QTreeWidget::itemDoubleClicked, this, &ProfileManagerPage::onProfileSelected);
    connect(m_btnSaveAs, &QPushButton::clicked, this, &ProfileManagerPage::onSaveClicked);

    // Connect toolbar buttons
    connect(m_btnNew, &QPushButton::clicked, this, &ProfileManagerPage::onNewClicked);
    connect(m_btnDuplicate, &QPushButton::clicked, this, &ProfileManagerPage::onDuplicateClicked);
    connect(m_btnRename, &QPushButton::clicked, this, &ProfileManagerPage::onRenameClicked);
    connect(m_btnDelete, &QPushButton::clicked, this, &ProfileManagerPage::onDeleteClicked);
    connect(m_btnRefresh, &QPushButton::clicked, this, &ProfileManagerPage::refreshProfileList);
    connect(m_btnOpen, &QPushButton::clicked, this, &ProfileManagerPage::onOpenClicked);

    // Initial load of profiles
    refreshProfileList();

    mainLayout->addWidget(m_splitter, 1);
}

ProfileManagerPage::~ProfileManagerPage() {
    QSettings settings("Antigravity", "AMAS");
    settings.setValue("ProfileManagerPage/SplitterState", m_splitter->saveState());
}

void ProfileManagerPage::createExplorerPanel(QWidget *parent) {
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(8);

    auto *lblExplorer = new QLabel(tr("Profile Explorer"), parent);
    lblExplorer->setStyleSheet("font-weight: bold; color: #FFFFFF; font-size: 14px;");
    layout->addWidget(lblExplorer);

    // Compact toolbar for profile explorer
    auto *toolbarWidget = new QWidget(parent);
    auto *toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);

    auto createToolbarButton = [parent, toolbarLayout](QPushButton *&btn, QStyle::StandardPixmap pixmap, const QString &tooltip) {
        btn = new QPushButton(parent);
        btn->setIcon(parent->style()->standardIcon(pixmap));
        btn->setToolTip(tooltip);
        btn->setFixedSize(30, 30);
        toolbarLayout->addWidget(btn);
    };

    createToolbarButton(m_btnNew, QStyle::SP_FileDialogNewFolder, tr("New Profile"));
    createToolbarButton(m_btnDuplicate, QStyle::SP_FileDialogDetailedView, tr("Duplicate Profile"));
    createToolbarButton(m_btnRename, QStyle::SP_MessageBoxQuestion, tr("Rename Profile"));
    createToolbarButton(m_btnDelete, QStyle::SP_TrashIcon, tr("Delete Profile"));
    createToolbarButton(m_btnImport, QStyle::SP_DialogOpenButton, tr("Import Profile"));
    createToolbarButton(m_btnExport, QStyle::SP_DialogSaveButton, tr("Export Profile"));
    createToolbarButton(m_btnRefresh, QStyle::SP_BrowserReload, tr("Refresh List"));
    
    toolbarLayout->addStretch();
    layout->addWidget(toolbarWidget);

    // Profile Explorer Tree
    m_treeProfiles = new QTreeWidget(parent);
    m_treeProfiles->setHeaderLabel(tr("Configuration Profiles"));
    m_treeProfiles->setMinimumWidth(240);

    // Populate mock tree items
    auto *rootItem = new QTreeWidgetItem(m_treeProfiles);
    rootItem->setText(0, tr("Measurement Profiles"));
    rootItem->setExpanded(true);

    auto *catGain = new QTreeWidgetItem(rootItem);
    catGain->setText(0, tr("Antenna Gain"));
    catGain->setExpanded(true);
    auto *profGain1 = new QTreeWidgetItem(catGain);
    profGain1->setText(0, tr("Horn 8-12 GHz"));
    auto *profGain2 = new QTreeWidgetItem(catGain);
    profGain2->setText(0, tr("Patch 12-18 GHz"));

    auto *catPattern = new QTreeWidgetItem(rootItem);
    catPattern->setText(0, tr("Radiation Pattern"));
    catPattern->setExpanded(true);
    auto *profPat1 = new QTreeWidgetItem(catPattern);
    profPat1->setText(0, tr("Chamber Test"));
    auto *profPat2 = new QTreeWidgetItem(catPattern);
    profPat2->setText(0, tr("Outdoor Scan"));

    auto *catS11 = new QTreeWidgetItem(rootItem);
    catS11->setText(0, tr("S11"));
    catS11->setExpanded(true);
    auto *profS11_1 = new QTreeWidgetItem(catS11);
    profS11_1->setText(0, tr("PCB Antenna"));
    auto *profS11_2 = new QTreeWidgetItem(catS11);
    profS11_2->setText(0, tr("Waveguide"));

    layout->addWidget(m_treeProfiles);
}

void ProfileManagerPage::createWorkspacePanel(QWidget *parent) {
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(16);

    // Wrap Right panel inside a scroll area to handle lower heights safely
    auto *scrollArea = new QScrollArea(parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");
    layout->addWidget(scrollArea);

    auto *scrollContent = new QWidget(scrollArea);
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(16);

    // Details Group
    auto *grpDetails = new QGroupBox(tr("Profile Details"), scrollContent);
    createDetailsGroup(grpDetails);
    scrollLayout->addWidget(grpDetails);

    // Configuration Preview Group
    auto *grpPreview = new QGroupBox(tr("Configuration Preview"), scrollContent);
    createPreviewGroup(grpPreview);
    scrollLayout->addWidget(grpPreview);

    // Validation Group
    auto *grpVal = new QGroupBox(tr("Profile Validation"), scrollContent);
    createValidationGroup(grpVal);
    scrollLayout->addWidget(grpVal);

    // Bottom Action Bar Layout (QHBoxLayout wrapper widget)
    auto *bottomWidget = new QWidget(scrollContent);
    auto *bottomLayout = new QHBoxLayout(bottomWidget);
    createBottomActionBar(bottomLayout);
    scrollLayout->addWidget(bottomWidget);

    scrollArea->setWidget(scrollContent);
}

void ProfileManagerPage::createDetailsGroup(QGroupBox *box) {
    auto *layout = new QGridLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto addDetailLabel = [box, layout](int r, int c, const QString &tag, QLabel *&refLabel, const QString &val) {
        layout->addWidget(new QLabel(tag, box), r, c);
        refLabel = new QLabel(val, box);
        refLabel->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        layout->addWidget(refLabel, r, c + 1);
    };

    addDetailLabel(0, 0, tr("Profile Name:"), m_lblProfName, tr("Horn 8-12 GHz"));
    addDetailLabel(0, 2, tr("Measurement Type:"), m_lblMeasType, tr("Gain Measurement"));
    addDetailLabel(0, 4, tr("Frequency Band:"), m_lblBand, tr("8-12 GHz"));

    addDetailLabel(1, 0, tr("Calibration:"), m_lblCal, tr("calchamber8_12ghz.sta"));
    addDetailLabel(1, 2, tr("Sweep Points:"), m_lblSweepPoints, tr("401"));
    addDetailLabel(1, 4, tr("Output Folder:"), m_lblOutputFolder, tr("measurements/horn_active"));

    addDetailLabel(2, 0, tr("Operator:"), m_lblOperator, tr("Operator-01"));
    addDetailLabel(2, 2, tr("Status:"), m_lblStatus, tr("Offline Draft"));
    addDetailLabel(2, 4, tr("Description:"), m_lblDescription, tr("Standard X-band horn calibration sweep."));

    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);
    layout->setColumnStretch(5, 1);
}

void ProfileManagerPage::createPreviewGroup(QGroupBox *box) {
    auto *layout = new QFormLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto addPreviewRow = [box, layout](const QString &tag, QLabel *&refLabel, const QString &val) {
        refLabel = new QLabel(val, box);
        refLabel->setStyleSheet("color: #FFFFFF;");
        layout->addRow(tag, refLabel);
    };

    addPreviewRow(tr("Frequency Range:"), m_lblPrevFreqRange, tr("8.000 GHz to 12.000 GHz"));
    addPreviewRow(tr("Angular Sweep:"), m_lblPrevAngularSweep, tr("Azimuth: 0.0\xc2\xb0 to 180.0\xc2\xb0 (Step: 10.0\xc2\xb0)"));
    addPreviewRow(tr("Polarization:"), m_lblPrevPolarization, tr("Vertical Co-Polarization"));
    addPreviewRow(tr("Output Format:"), m_lblPrevOutputFormat, tr("CSV Data points + PDF Report generation"));
    addPreviewRow(tr("Estimated Sweep Duration:"), m_lblPrevDuration, tr("00:03:45"));
}

void ProfileManagerPage::createValidationGroup(QGroupBox *box) {
    auto *layout = new QHBoxLayout(box);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(20);

    layout->addWidget(createStatusRow(box, tr("Profile Config Valid:"), true));
    layout->addWidget(createStatusRow(box, tr("Calibration File Available:"), true));
    layout->addWidget(createStatusRow(box, tr("System Measurement Ready:"), false));
}

void ProfileManagerPage::createBottomActionBar(QHBoxLayout *layout) {
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QWidget *parentWidget = layout->parentWidget();

    m_btnOpen = new QPushButton(tr("Open Profile"), parentWidget);
    m_btnOpen->setIcon(parentWidget->style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_btnOpen->setMinimumWidth(140);
    m_btnOpen->setMinimumHeight(32);

    m_btnEdit = new QPushButton(tr("Edit"), parentWidget);
    m_btnEdit->setIcon(parentWidget->style()->standardIcon(QStyle::SP_DialogYesButton));
    m_btnEdit->setMinimumWidth(140);
    m_btnEdit->setMinimumHeight(32);

    m_btnCopy = new QPushButton(tr("Copy"), parentWidget);
    m_btnCopy->setIcon(parentWidget->style()->standardIcon(QStyle::SP_FileDialogInfoView));
    m_btnCopy->setMinimumWidth(140);
    m_btnCopy->setMinimumHeight(32);

    m_btnSaveAs = new QPushButton(tr("Save As..."), parentWidget);
    m_btnSaveAs->setIcon(parentWidget->style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnSaveAs->setMinimumWidth(140);
    m_btnSaveAs->setMinimumHeight(32);

    m_btnClose = new QPushButton(tr("Close"), parentWidget);
    m_btnClose->setIcon(parentWidget->style()->standardIcon(QStyle::SP_DialogCancelButton));
    m_btnClose->setMinimumWidth(140);
    m_btnClose->setMinimumHeight(32);

    layout->addWidget(m_btnOpen);
    layout->addWidget(m_btnEdit);
    layout->addWidget(m_btnCopy);
    layout->addStretch();
    layout->addWidget(m_btnSaveAs);
    layout->addWidget(m_btnClose);
}

QWidget* ProfileManagerPage::createStatusRow(QWidget *container, const QString &label, bool isActive) {
    auto *row = new QWidget(container);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *lblText = new QLabel(label, row);
    lblText->setStyleSheet("color: #C8C8C8; font-weight: 500;");
    
    auto *indicator = new QLabel(row);
    indicator->setFixedSize(10, 10);
    
    auto *lblVal = new QLabel(row);
    lblVal->setStyleSheet("color: #FFFFFF; font-weight: bold;");

    if (isActive) {
        indicator->setStyleSheet("border-radius: 5px; background-color: #4CAF50;"); // Green
        lblVal->setText(tr("Yes"));
    } else {
        indicator->setStyleSheet("border-radius: 5px; background-color: #F44336;"); // Red
        lblVal->setText(tr("No"));
    }

    layout->addWidget(lblText);
    layout->addWidget(indicator);
    layout->addWidget(lblVal);
    layout->addStretch();
    return row;
}

void ProfileManagerPage::onProfileSelected(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString name = item->text(0);
    if (name == tr("Measurement Profiles") || name == tr("Antenna Gain") || name == tr("Radiation Pattern") || name == tr("S11")) {
        return;
    }

    MeasurementProfile profile;
    if (m_presenter->loadProfile(name, profile)) {
        m_lblProfName->setText(QString::fromStdString(profile.profileName));
        m_lblMeasType->setText(QString::fromStdString(profile.measurementType));
        m_lblBand->setText(name.contains("Horn") ? tr("8-12 GHz") : tr("12-18 GHz"));
        m_lblCal->setText(QString::fromStdString(profile.calibrationFile));
        m_lblSweepPoints->setText(QString::number(profile.sweepPoints));
        m_lblStatus->setText(tr("Draft Ready"));

        m_lblPrevFreqRange->setText(tr("%1 GHz to %2 GHz").arg(profile.startFrequencyHz / 1e9).arg(profile.stopFrequencyHz / 1e9));
        m_lblPrevAngularSweep->setText(tr("Azimuth: %1\xc2\xb0 to %2\xc2\xb0 (Step: %3\xc2\xb0)")
                                      .arg(profile.positioner.startAngleDeg)
                                      .arg(profile.positioner.stopAngleDeg)
                                      .arg(profile.positioner.stepAngleDeg));
        m_lblPrevPolarization->setText(tr("Vertical"));
        m_lblPrevOutputFormat->setText(tr("CSV / PDF"));
        m_lblPrevDuration->setText(tr("00:03:45"));
    }
}

void ProfileManagerPage::onSaveClicked() {
    MeasurementProfile profile;
    profile.profileName = m_lblProfName->text().toStdString();
    profile.measurementType = m_lblMeasType->text().toStdString();
    profile.startFrequencyHz = 8.0e9;
    profile.stopFrequencyHz = 12.0e9;
    profile.sweepPoints = m_lblSweepPoints->text().toInt();
    profile.calibrationFile = m_lblCal->text().toStdString();

    m_presenter->saveProfile(QString::fromStdString(profile.profileName), profile);
}

void ProfileManagerPage::refreshProfileList() {
    m_treeProfiles->clear();
    auto *rootItem = new QTreeWidgetItem(m_treeProfiles);
    rootItem->setText(0, tr("Measurement Profiles"));
    rootItem->setExpanded(true);

    auto *catGain = new QTreeWidgetItem(rootItem);
    catGain->setText(0, tr("Antenna Gain"));
    catGain->setExpanded(true);

    auto *catPattern = new QTreeWidgetItem(rootItem);
    catPattern->setText(0, tr("Radiation Pattern"));
    catPattern->setExpanded(true);

    auto *catS11 = new QTreeWidgetItem(rootItem);
    catS11->setText(0, tr("S11"));
    catS11->setExpanded(true);

    for (const auto& name : m_presenter->getProfiles()) {
        QTreeWidgetItem *parentItem = rootItem;
        if (name.contains("Horn") || name.contains("Patch")) {
            parentItem = catGain;
        } else if (name.contains("Pattern") || name.contains("Scan") || name.contains("Chamber")) {
            parentItem = catPattern;
        } else if (name.contains("PCB") || name.contains("Waveguide")) {
            parentItem = catS11;
        }
        auto *item = new QTreeWidgetItem(parentItem);
        item->setText(0, name);
    }
}

void ProfileManagerPage::onNewClicked() {
    bool ok = false;
    QString newName = QInputDialog::getText(this, tr("Create Profile"), tr("Enter Profile Name:"), QLineEdit::Normal, "", &ok);
    if (ok && !newName.isEmpty()) {
        MeasurementProfile p;
        p.profileName = newName.toStdString();
        p.measurementType = "S11";
        p.startFrequencyHz = 8.0e9;
        p.stopFrequencyHz = 12.0e9;
        p.sweepPoints = 401;
        p.calibrationFile = "None";
        p.positioner.usePositioner = true;
        p.positioner.startAngleDeg = 0.0f;
        p.positioner.stopAngleDeg = 180.0f;
        p.positioner.stepAngleDeg = 10.0f;
        p.positioner.settleTimeMs = 150;

        m_presenter->saveProfile(newName, p);
    }
}

void ProfileManagerPage::onDuplicateClicked() {
    QTreeWidgetItem *item = m_treeProfiles->currentItem();
    if (item && item->parent() && item->parent() != m_treeProfiles->invisibleRootItem()) {
        QString source = item->text(0);
        m_presenter->duplicateProfile(source, source + " (Copy)");
    }
}

void ProfileManagerPage::onRenameClicked() {
    QTreeWidgetItem *item = m_treeProfiles->currentItem();
    if (item && item->parent() && item->parent() != m_treeProfiles->invisibleRootItem()) {
        QString oldName = item->text(0);
        bool ok = false;
        QString newName = QInputDialog::getText(this, tr("Rename Profile"), tr("New Profile Name:"), QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.isEmpty()) {
            MeasurementProfile p;
            if (m_presenter->loadProfile(oldName, p)) {
                m_presenter->deleteProfile(oldName);
                m_presenter->saveProfile(newName, p);
            }
        }
    }
}

void ProfileManagerPage::onDeleteClicked() {
    QTreeWidgetItem *item = m_treeProfiles->currentItem();
    if (item && item->parent() && item->parent() != m_treeProfiles->invisibleRootItem()) {
        m_presenter->deleteProfile(item->text(0));
    }
}

void ProfileManagerPage::onOpenClicked() {
    QTreeWidgetItem *item = m_treeProfiles->currentItem();
    if (item && item->parent() && item->parent() != m_treeProfiles->invisibleRootItem()) {
        MeasurementProfile p;
        if (m_presenter->loadProfile(item->text(0), p)) {
            m_presenter->setActiveProfile(p);
        }
    }
}

} // namespace AMAS
