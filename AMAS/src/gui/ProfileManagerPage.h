#ifndef AMAS_PROFILEMANAGERPAGE_H
#define AMAS_PROFILEMANAGERPAGE_H

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QSplitter>

namespace AMAS {

class ProfilePresenter;

class ProfileManagerPage : public QWidget {
    Q_OBJECT
public:
    explicit ProfileManagerPage(ProfilePresenter *presenter, QWidget *parent = nullptr);
    ~ProfileManagerPage() override;

private:
    void createExplorerPanel(QWidget *parent);
    void createWorkspacePanel(QWidget *parent);

    // Right panel subdivision helpers
    void createDetailsGroup(QGroupBox *box);
    void createPreviewGroup(QGroupBox *box);
    void createValidationGroup(QGroupBox *box);
    void createBottomActionBar(QHBoxLayout *layout);

    // Helpers to create styled status lights
    QWidget* createStatusRow(QWidget *container, const QString &label, bool isActive);

    // Sidebar Explorer
    QTreeWidget *m_treeProfiles;

    // Sidebar Toolbar Buttons
    QPushButton *m_btnNew;
    QPushButton *m_btnDuplicate;
    QPushButton *m_btnRename;
    QPushButton *m_btnDelete;
    QPushButton *m_btnImport;
    QPushButton *m_btnExport;
    QPushButton *m_btnRefresh;

    // Detail Panel Labels
    QLabel *m_lblProfName;
    QLabel *m_lblMeasType;
    QLabel *m_lblBand;
    QLabel *m_lblCal;
    QLabel *m_lblSweepPoints;
    QLabel *m_lblOutputFolder;
    QLabel *m_lblOperator;
    QLabel *m_lblDescription;
    QLabel *m_lblStatus;

    // Preview Panel Labels
    QLabel *m_lblPrevFreqRange;
    QLabel *m_lblPrevAngularSweep;
    QLabel *m_lblPrevPolarization;
    QLabel *m_lblPrevOutputFormat;
    QLabel *m_lblPrevDuration;

    // Validation Indicators
    QLabel *m_indValid;
    QLabel *m_indCal;
    QLabel *m_indReady;

    // Bottom action buttons
    QPushButton *m_btnOpen;
    QPushButton *m_btnEdit;
    QPushButton *m_btnCopy;
    QPushButton *m_btnSaveAs;
    QPushButton *m_btnClose;

    QSplitter *m_splitter;

private slots:
    void onProfileSelected(QTreeWidgetItem *item, int column);
    void onSaveClicked();
    void refreshProfileList();
    void onNewClicked();
    void onDuplicateClicked();
    void onRenameClicked();
    void onDeleteClicked();
    void onOpenClicked();

private:
    ProfilePresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_PROFILEMANAGERPAGE_H
