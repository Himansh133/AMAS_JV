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
#include <QLineEdit>
#include <QSplitter>
#include "measurement/MeasurementSession.h"

namespace AMAS {

class ResultsPresenter;
class MeasurementPlotWidget;
class PolarPlotWidget;
class SmithChartWidget;
#ifdef AMAS_HAS_3D_VISUALIZATION
class RadiationPattern3DWidget;
#endif

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
    QWidget* createMagnitudeTab(QWidget *parent);
    QWidget* createPhaseTab(QWidget *parent);
    QWidget* createPolarTab(QWidget *parent);
    QWidget* createSmithTab(QWidget *parent);
    QWidget* createPattern3DTab(QWidget *parent);
    QWidget* createTableTab(QWidget *parent);
    QWidget* createStatisticsTab(QWidget *parent);
    QWidget* createReportTab(QWidget *parent);

    void createBottomStatusBar(QHBoxLayout *layout);
    void loadSessionToUI(const MeasurementSession &session);

    // Sidebar widgets
    QTreeWidget *m_treeBrowser;
    QTableWidget *m_tableMarkers;
    QPushButton *m_btnAddMarker;
    QPushButton *m_btnRemoveMarker;
    QPushButton *m_btnClearMarkers;

    // Top Summary widgets
    QLabel *m_lblSumName;
    QLabel *m_lblSumType;
    QLabel *m_lblSumBand;
    QLabel *m_lblSumCal;
    QLabel *m_lblSumDate;
    QLabel *m_lblSumOperator;
    QLabel *m_lblSumStatus;

    // Tabs
    QTabWidget   *m_tabWidget;
    QTableWidget *m_dataTable;

    // Dynamic Stats labels
    QLabel *m_lblStatMinMag;
    QLabel *m_lblStatMaxMag;
    QLabel *m_lblStatAvgMag;
    QLabel *m_lblStatMinPhase;
    QLabel *m_lblStatMaxPhase;
    QLabel *m_lblStatAvgPhase;
    QLabel *m_lblStatPeakFreq;
    QLabel *m_lblStatSamples;
    QLabel *m_lblStatFreqSpan;

    // Plots
    MeasurementPlotWidget *m_magPlot;
    MeasurementPlotWidget *m_phasePlot;
    PolarPlotWidget       *m_polarPlot;
    SmithChartWidget      *m_smithChart;
#ifdef AMAS_HAS_3D_VISUALIZATION
    RadiationPattern3DWidget *m_pattern3DPlot;
#else
    QWidget                  *m_pattern3DPlot;
#endif

    // Report Preview Text Edit
    QTextEdit *m_txtReportPreview;

    // Report Customization inputs
    QLineEdit *m_editProjectName;
    QLineEdit *m_editOperator;
    QLineEdit *m_editCompany;
    QLineEdit *m_editLab;
    QLineEdit *m_editReportTitle;
    QTextEdit *m_editComments;

    // Report Actions buttons
    QPushButton *m_btnGenerateReport;
    QPushButton *m_btnPreviewReport;
    QPushButton *m_btnExportPdf;
    QPushButton *m_btnExportHtml;
    QPushButton *m_btnExportMd;
    QPushButton *m_btnOpenFolder;

    QString m_lastReportFolderPath;

    // Status bar labels
    QLabel *m_statusLoaded;
    QLabel *m_statusMemory;
    QLabel *m_statusAnalysis;
    QLabel *m_statusUpdate;

    QSplitter *m_splitter;

    void refreshBrowser();
    void onSessionChanged();

private slots:
    void onSessionSelected(QTreeWidgetItem *item, int column);
    void onAddMarkerClicked();
    void onRemoveMarkerClicked();
    void onClearMarkersClicked();

private:
    ResultsPresenter *m_presenter;

    struct EngineeringMarker {
        int id;
        double frequencyHz;
        double magnitudeDb;
        double phaseDeg;
        double angleDeg;
        int pointIndex;
    };
    std::vector<EngineeringMarker> m_markers;
    MeasurementSession m_currentSession;
    void updateMarkersUI();
};

} // namespace AMAS

#endif // AMAS_RESULTSPAGE_H
