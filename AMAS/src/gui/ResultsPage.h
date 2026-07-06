#ifndef AMAS_RESULTSPAGE_H
#define AMAS_RESULTSPAGE_H

#include <QWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QSplitter>

namespace AMAS {

class ResultsPresenter;
struct MeasurementSession;

class ResultsPage : public QWidget {
    Q_OBJECT
public:
    explicit ResultsPage(ResultsPresenter *presenter, QWidget *parent = nullptr);
    ~ResultsPage() override;

private:
    void createBrowserPanel(QWidget *parent);
    void createWorkspacePanel(QWidget *parent);

    // Tab content creation helpers
    QWidget* createOverviewTab(QWidget *parent);
    QWidget* createGraphsTab(QWidget *parent);
    QWidget* createTableTab(QWidget *parent);
    QWidget* createStatisticsTab(QWidget *parent);
    QWidget* createReportTab(QWidget *parent);

    void createBottomStatusBar(QHBoxLayout *layout);
    void loadSessionToUI(const MeasurementSession &session);

    // Sidebar widgets
    QTreeWidget *m_treeBrowser;

    // Top Summary widgets
    QLabel *m_lblSumName;
    QLabel *m_lblSumType;
    QLabel *m_lblSumBand;
    QLabel *m_lblSumCal;
    QLabel *m_lblSumDate;
    QLabel *m_lblSumOperator;
    QLabel *m_lblSumStatus;

    // Tabs
    QTabWidget  *m_tabWidget;
    QTableWidget *m_dataTable;

    // Status bar labels
    QLabel *m_statusLoaded;
    QLabel *m_statusMemory;
    QLabel *m_statusAnalysis;
    QLabel *m_statusUpdate;

    QSplitter *m_splitter;

private slots:
    void onSessionSelected(QTreeWidgetItem *item, int column);

private:
    ResultsPresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_RESULTSPAGE_H
