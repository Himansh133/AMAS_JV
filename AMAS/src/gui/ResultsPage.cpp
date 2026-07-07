#include "ResultsPage.h"
#include "presentation/ResultsPresenter.h"
#include "presentation/MeasurementPresenter.h"
#include "measurement/MeasurementSession.h"
#include "MainWindow.h"
#include "framework/SettingsManager.h"
#include "widgets/MeasurementPlotWidget.h"
#include "widgets/PolarPlotWidget.h"
#include "widgets/SmithChartWidget.h"
#ifdef AMAS_HAS_3D_VISUALIZATION
#include "widgets/RadiationPattern3DWidget.h"
#endif
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QFrame>
#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QPdfWriter>
#include <QTextDocument>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include "reports/ReportGenerator.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

#include <QUndoCommand>

namespace AMAS {

class MetadataChangeCommand : public QUndoCommand {
public:
    MetadataChangeCommand(ResultsPage *page, MeasurementSession &session, const std::string &fieldName, const std::string &oldValue, const std::string &newValue, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent)
        , m_page(page)
        , m_session(session)
        , m_fieldName(fieldName)
        , m_oldValue(oldValue)
        , m_newValue(newValue)
    {
        setText(QObject::tr("Edit %1").arg(QString::fromStdString(fieldName)));
    }

    void undo() override {
        updateValue(m_oldValue);
    }

    void redo() override {
        updateValue(m_newValue);
    }

private:
    void updateValue(const std::string &value) {
        if (m_fieldName == "Project Name") m_session.metadata.projectName = value;
        else if (m_fieldName == "Operator Name") m_session.metadata.operatorName = value;
        else if (m_fieldName == "Company") m_session.metadata.company = value;
        else if (m_fieldName == "Laboratory") m_session.metadata.laboratory = value;
        else if (m_fieldName == "Report Title") m_session.metadata.reportTitle = value;
        else if (m_fieldName == "Comments") m_session.metadata.comments = value;

        m_page->loadSessionMetadataOnly(m_session.metadata);
        m_page->autoSaveSession();
    }

    ResultsPage *m_page;
    MeasurementSession &m_session;
    std::string m_fieldName;
    std::string m_oldValue;
    std::string m_newValue;
};

ResultsPage::ResultsPage(ResultsPresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_presenter(presenter)
{
    // Main layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    // Title Block
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);

    auto *lblTitle = new QLabel(tr("Results & Analysis"), this);
    lblTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(lblTitle);

    auto *lblSubtitle = new QLabel(tr("Review, analyze, and generate reports for completed measurement runs."), this);
    lblSubtitle->setStyleSheet("font-size: 13px; color: #C8C8C8;");
    headerLayout->addWidget(lblSubtitle);
    mainLayout->addLayout(headerLayout);

    // Resizable Horizontal Splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(6);
    m_splitter->setStyleSheet("QSplitter::handle { background-color: #3F3F46; }");

    // Add Left sidebar (Browser)
    auto *browserContainer = new QWidget(m_splitter);
    createBrowserPanel(browserContainer);
    m_splitter->addWidget(browserContainer);

    // Add Right workspace
    auto *workspaceContainer = new QWidget(m_splitter);
    createWorkspacePanel(workspaceContainer);
    m_splitter->addWidget(workspaceContainer);

    // Set initial splitter stretch factors
    m_splitter->setStretchFactor(0, 1); // Sidebar weight
    m_splitter->setStretchFactor(1, 4); // Workspace weight

    // Restore state using QSettings
    QSettings settings("Antigravity", "AMAS");
    QByteArray state = settings.value("ResultsPage/SplitterState").toByteArray();
    if (!state.isEmpty()) {
        m_splitter->restoreState(state);
    }

    mainLayout->addWidget(m_splitter, 1);

    // Bind Browser Tree signals
    connect(m_treeBrowser, &QTreeWidget::itemDoubleClicked, this, &ResultsPage::onSessionSelected);
    connect(m_presenter->parentPresenter(), &MeasurementPresenter::stateChanged, this, &ResultsPage::onSessionChanged);

    // Bind Marker Control signals
    connect(m_btnAddMarker, &QPushButton::clicked, this, &ResultsPage::onAddMarkerClicked);
    connect(m_btnRemoveMarker, &QPushButton::clicked, this, &ResultsPage::onRemoveMarkerClicked);
    connect(m_btnClearMarkers, &QPushButton::clicked, this, &ResultsPage::onClearMarkersClicked);

    // Bottom Status Bar
    auto *statusBarLayout = new QHBoxLayout();
    createBottomStatusBar(statusBarLayout);
    mainLayout->addLayout(statusBarLayout);

    // Load initial/latest session if present
    onSessionChanged();
}

ResultsPage::~ResultsPage() {
    QSettings settings("Antigravity", "AMAS");
    settings.setValue("ResultsPage/SplitterState", m_splitter->saveState());
}

void ResultsPage::createBrowserPanel(QWidget *parent) {
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(8);

    auto *lblTitle = new QLabel(tr("Measurement Browser"), parent);
    lblTitle->setStyleSheet("font-weight: bold; color: #FFFFFF; font-size: 14px;");
    layout->addWidget(lblTitle);

    m_treeBrowser = new QTreeWidget(parent);
    m_treeBrowser->setHeaderLabel(tr("Measurements"));
    m_treeBrowser->setMinimumWidth(220);
    m_treeBrowser->setMinimumHeight(150);
    layout->addWidget(m_treeBrowser, 2);

    // Markers Section
    auto *lblMarkers = new QLabel(tr("Active Markers"), parent);
    lblMarkers->setStyleSheet("font-weight: bold; color: #FFFFFF; font-size: 14px; margin-top: 10px;");
    layout->addWidget(lblMarkers);

    m_tableMarkers = new QTableWidget(0, 4, parent);
    m_tableMarkers->setHorizontalHeaderLabels({ tr("ID"), tr("Freq (GHz)"), tr("Mag (dB)"), tr("Phase (°)") });
    m_tableMarkers->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableMarkers->verticalHeader()->setVisible(false);
    m_tableMarkers->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableMarkers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableMarkers->setAlternatingRowColors(true);
    m_tableMarkers->setStyleSheet(
        "QTableWidget { background-color: #2D2D30; alternate-background-color: #252526; border: 1px solid #3F3F46; color: #FFFFFF; }"
    );
    m_tableMarkers->setMinimumHeight(120);
    layout->addWidget(m_tableMarkers, 1);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    m_btnAddMarker = new QPushButton(tr("+ Add"), parent);
    m_btnRemoveMarker = new QPushButton(tr("- Del"), parent);
    m_btnClearMarkers = new QPushButton(tr("Clear"), parent);

    auto styleBtn = [](QPushButton *btn) {
        btn->setStyleSheet(
            "QPushButton { background-color: #2D2D30; border: 1px solid #3F3F46; color: #FFFFFF; font-size: 11px; padding: 4px; }"
            "QPushButton:hover { background-color: #3F3F46; }"
        );
        btn->setMinimumHeight(24);
    };
    styleBtn(m_btnAddMarker);
    styleBtn(m_btnRemoveMarker);
    styleBtn(m_btnClearMarkers);

    btnLayout->addWidget(m_btnAddMarker);
    btnLayout->addWidget(m_btnRemoveMarker);
    btnLayout->addWidget(m_btnClearMarkers);
    layout->addLayout(btnLayout);
}

void ResultsPage::refreshBrowser() {
    m_treeBrowser->clear();
    auto *rootItem = new QTreeWidgetItem(m_treeBrowser);
    rootItem->setText(0, tr("Sessions"));
    rootItem->setExpanded(true);

    // 1. Add mock historical sessions
    auto *sess1 = new QTreeWidgetItem(rootItem);
    sess1->setText(0, tr("Session_001"));
    
    auto *itemS11_1 = new QTreeWidgetItem(sess1);
    itemS11_1->setText(0, tr("S11 Reflection"));
    auto *itemGain_1 = new QTreeWidgetItem(sess1);
    itemGain_1->setText(0, tr("Gain Sweep"));

    auto *sess2 = new QTreeWidgetItem(rootItem);
    sess2->setText(0, tr("Session_002"));

    auto *itemS11_2 = new QTreeWidgetItem(sess2);
    itemS11_2->setText(0, tr("S11 Patch"));

    // 2. Add latest live session if present
    if (m_presenter->hasLatestSession()) {
        MeasurementSession session;
        if (m_presenter->loadLatestSession(session)) {
            auto *sessLive = new QTreeWidgetItem(rootItem);
            sessLive->setText(0, QString::fromStdString(session.sessionName));
            sessLive->setExpanded(true);
            auto *itemLive = new QTreeWidgetItem(sessLive);
            itemLive->setText(0, QString::fromStdString(session.measurementType));
        }
    }
}

void ResultsPage::onSessionChanged() {
    refreshBrowser();
    if (m_presenter->hasLatestSession()) {
        MeasurementSession session;
        if (m_presenter->loadLatestSession(session)) {
            loadSessionToUI(session);
        }
    }
}

void ResultsPage::createWorkspacePanel(QWidget *parent) {
    auto *layout = new QVBoxLayout(parent);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(16);

    // Top Summary Box
    auto *summaryBox = new QGroupBox(tr("Active Measurement Summary"), parent);
    auto *summaryLayout = new QGridLayout(summaryBox);
    summaryLayout->setContentsMargins(12, 12, 12, 12);
    summaryLayout->setSpacing(12);

    auto addSummaryLabel = [parent, summaryLayout](int r, int c, const QString &tag, QLabel *&refLabel, const QString &valText) {
        summaryLayout->addWidget(new QLabel(tag, parent), r, c);
        refLabel = new QLabel(valText, parent);
        refLabel->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        summaryLayout->addWidget(refLabel, r, c + 1);
    };

    addSummaryLabel(0, 0, tr("Name:"), m_lblSumName, tr("Horn_Gain_Sweep"));
    addSummaryLabel(0, 2, tr("Type:"), m_lblSumType, tr("Gain Measurement"));
    addSummaryLabel(0, 4, tr("Date:"), m_lblSumDate, tr("2026-07-01 10:15"));

    addSummaryLabel(1, 0, tr("Band:"), m_lblSumBand, tr("8-12 GHz"));
    addSummaryLabel(1, 2, tr("Calibration:"), m_lblSumCal, tr("calchamber8_12ghz.sta"));
    addSummaryLabel(1, 4, tr("Operator:"), m_lblSumOperator, tr("Operator-01"));

    summaryLayout->addWidget(new QLabel(tr("Status:"), parent), 2, 0);
    m_lblSumStatus = new QLabel(tr("Completed"), parent);
    m_lblSumStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    summaryLayout->addWidget(m_lblSumStatus, 2, 1);

    layout->addWidget(summaryBox);

    // Tab Widget
    m_tabWidget = new QTabWidget(parent);
    m_tabWidget->addTab(createOverviewTab(m_tabWidget), tr("Overview"));
    m_tabWidget->addTab(createMagnitudeTab(m_tabWidget), tr("Magnitude Plot"));
    m_tabWidget->addTab(createPhaseTab(m_tabWidget), tr("Phase Plot"));
    m_tabWidget->addTab(createPolarTab(m_tabWidget), tr("Polar Plot"));
    m_tabWidget->addTab(createSmithTab(m_tabWidget), tr("Smith Chart"));
    m_tabWidget->addTab(createPattern3DTab(m_tabWidget), tr("3D Pattern"));
    m_tabWidget->addTab(createTableTab(m_tabWidget), tr("Data Table"));
    m_tabWidget->addTab(createStatisticsTab(m_tabWidget), tr("Statistics"));
    m_tabWidget->addTab(createReportTab(m_tabWidget), tr("Report"));

    layout->addWidget(m_tabWidget);
}

QWidget* ResultsPage::createOverviewTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *infoBox = new QGroupBox(tr("Measurement Information"), tab);
    auto *infoLayout = new QFormLayout(infoBox);
    infoLayout->setSpacing(10);

    infoLayout->addRow(tr("RF Instrument:"), new QLabel(tr("Keysight FieldFox N9951B"), infoBox));
    infoLayout->addRow(tr("Calibration State:"), new QLabel(tr("Coaxial 2-Port SOLT loaded"), infoBox));
    infoLayout->addRow(tr("Start Frequency:"), new QLabel(tr("8.000 GHz"), infoBox));
    infoLayout->addRow(tr("Stop Frequency:"), new QLabel(tr("12.000 GHz"), infoBox));
    infoLayout->addRow(tr("Sweep Points:"), new QLabel(tr("401"), infoBox));

    layout->addWidget(infoBox);

    auto *notesBox = new QGroupBox(tr("Session Notes"), tab);
    auto *notesLayout = new QVBoxLayout(notesBox);
    auto *notesEdit = new QTextEdit(notesBox);
    notesEdit->setPlaceholderText(tr("Enter session notes, weather details, chamber configurations, etc. here..."));
    notesLayout->addWidget(notesEdit);

    layout->addWidget(notesBox);
    layout->addStretch();
    return tab;
}

QWidget* ResultsPage::createMagnitudeTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    m_magPlot = new MeasurementPlotWidget(tr("Magnitude Response (S11)"), tab);
    layout->addWidget(m_magPlot);
    return tab;
}

QWidget* ResultsPage::createPhaseTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    m_phasePlot = new MeasurementPlotWidget(tr("Phase Response (S11)"), tab);
    layout->addWidget(m_phasePlot);
    return tab;
}

QWidget* ResultsPage::createPolarTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    m_polarPlot = new PolarPlotWidget(tr("Polar Radiation Response"), tab);
    layout->addWidget(m_polarPlot);
    return tab;
}

QWidget* ResultsPage::createSmithTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    m_smithChart = new SmithChartWidget(tr("Complex Reflection Locus"), tab);
    layout->addWidget(m_smithChart);
    return tab;
}

QWidget* ResultsPage::createPattern3DTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

#ifdef AMAS_HAS_3D_VISUALIZATION
    m_pattern3DPlot = new RadiationPattern3DWidget(tr("3D Radiation Balloon Pattern"), tab);
    layout->addWidget(m_pattern3DPlot);
#else
    auto *lblFallback = new QLabel(tr("3D Radiation Pattern visualization requires the Qt Data Visualization module, "
                                      "which is not installed in the current Qt environment."), tab);
    lblFallback->setAlignment(Qt::AlignCenter);
    lblFallback->setStyleSheet("color: #C8C8C8; font-size: 13px; font-weight: bold;");
    layout->addWidget(lblFallback);
    m_pattern3DPlot = nullptr;
#endif
    return tab;
}

QWidget* ResultsPage::createTableTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);

    m_dataTable = new QTableWidget(5, 5, tab);
    m_dataTable->setAlternatingRowColors(true);
    m_dataTable->setSortingEnabled(true);
    m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Headers
    QStringList headers = { tr("Frequency (GHz)"), tr("Magnitude (dB)"), tr("Phase (deg)"), tr("Angle (deg)"), tr("Gain (dBi)") };
    m_dataTable->setHorizontalHeaderLabels(headers);
    m_dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_dataTable->verticalHeader()->setVisible(false);

    m_dataTable->setStyleSheet(
        "QTableWidget { "
        "  background-color: #2D2D30; "
        "  alternate-background-color: #252526; "
        "  border: none; "
        "} "
    );

    // Mock data rows
    auto setCell = [this](int r, int c, double val) {
        auto *item = new QTableWidgetItem();
        item->setData(Qt::DisplayRole, val);
        item->setTextAlignment(Qt::AlignCenter);
        m_dataTable->setItem(r, c, item);
    };

    setCell(0, 0, 8.000);  setCell(0, 1, -12.4); setCell(0, 2, 45.2);  setCell(0, 3, 0.0);   setCell(0, 4, 10.2);
    setCell(1, 0, 9.000);  setCell(1, 1, -15.8); setCell(1, 2, -22.1); setCell(1, 3, 10.0);  setCell(1, 4, 11.5);
    setCell(2, 0, 10.000); setCell(2, 1, -22.1); setCell(2, 2, -88.4); setCell(2, 3, 20.0);  setCell(2, 4, 12.4);
    setCell(3, 0, 11.000); setCell(3, 1, -18.2); setCell(3, 2, 120.3); setCell(3, 3, 30.0);  setCell(3, 4, 11.9);
    setCell(4, 0, 12.000); setCell(4, 1, -14.1); setCell(4, 2, 155.1); setCell(4, 3, 40.0);  setCell(4, 4, 10.8);

    layout->addWidget(m_dataTable);
    return tab;
}

QWidget* ResultsPage::createStatisticsTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *statsBox = new QGroupBox(tr("Analysis Output Metrics"), tab);
    auto *formLayout = new QFormLayout(statsBox);
    formLayout->setSpacing(10);

    m_lblStatMinMag = new QLabel(tr("N/A"), statsBox);
    m_lblStatMaxMag = new QLabel(tr("N/A"), statsBox);
    m_lblStatAvgMag = new QLabel(tr("N/A"), statsBox);
    m_lblStatMinPhase = new QLabel(tr("N/A"), statsBox);
    m_lblStatMaxPhase = new QLabel(tr("N/A"), statsBox);
    m_lblStatAvgPhase = new QLabel(tr("N/A"), statsBox);
    m_lblStatPeakFreq = new QLabel(tr("N/A"), statsBox);
    m_lblStatSamples = new QLabel(tr("0"), statsBox);
    m_lblStatFreqSpan = new QLabel(tr("N/A"), statsBox);

    auto setLabelStyle = [](QLabel *lbl) {
        lbl->setStyleSheet("color: #FFFFFF; font-weight: bold; font-family: monospace;");
    };
    setLabelStyle(m_lblStatMinMag);
    setLabelStyle(m_lblStatMaxMag);
    setLabelStyle(m_lblStatAvgMag);
    setLabelStyle(m_lblStatMinPhase);
    setLabelStyle(m_lblStatMaxPhase);
    setLabelStyle(m_lblStatAvgPhase);
    setLabelStyle(m_lblStatPeakFreq);
    setLabelStyle(m_lblStatSamples);
    setLabelStyle(m_lblStatFreqSpan);

    formLayout->addRow(tr("Minimum Magnitude:"), m_lblStatMinMag);
    formLayout->addRow(tr("Maximum Magnitude:"), m_lblStatMaxMag);
    formLayout->addRow(tr("Average Magnitude:"), m_lblStatAvgMag);
    formLayout->addRow(tr("Minimum Phase:"), m_lblStatMinPhase);
    formLayout->addRow(tr("Maximum Phase:"), m_lblStatMaxPhase);
    formLayout->addRow(tr("Average Phase:"), m_lblStatAvgPhase);
    formLayout->addRow(tr("Peak Frequency:"), m_lblStatPeakFreq);
    formLayout->addRow(tr("Number of Samples:"), m_lblStatSamples);
    formLayout->addRow(tr("Frequency Span:"), m_lblStatFreqSpan);

    layout->addWidget(statsBox);

    auto *btnLayout = new QHBoxLayout();
    auto *btnExportCsv = new QPushButton(tr("Export Statistics to CSV"), tab);
    btnExportCsv->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    btnExportCsv->setMinimumHeight(32);
    btnExportCsv->setMinimumWidth(200);
    
    connect(btnExportCsv, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, 
            tr("Export Statistics as CSV"), "", tr("CSV Files (*.csv)"));
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Metric,Value\n";
            out << "Minimum Magnitude," << m_lblStatMinMag->text().trimmed() << "\n";
            out << "Maximum Magnitude," << m_lblStatMaxMag->text().trimmed() << "\n";
            out << "Average Magnitude," << m_lblStatAvgMag->text().trimmed() << "\n";
            out << "Minimum Phase," << m_lblStatMinPhase->text().trimmed() << "\n";
            out << "Maximum Phase," << m_lblStatMaxPhase->text().trimmed() << "\n";
            out << "Average Phase," << m_lblStatAvgPhase->text().trimmed() << "\n";
            out << "Peak Frequency," << m_lblStatPeakFreq->text().trimmed() << "\n";
            out << "Number of Samples," << m_lblStatSamples->text().trimmed() << "\n";
            out << "Frequency Span," << m_lblStatFreqSpan->text().trimmed() << "\n";
            file.close();
            QMessageBox::information(this, tr("Export Success"), 
                tr("Statistics successfully exported to '%1'").arg(QFileInfo(fileName).fileName()));
        } else {
            QMessageBox::warning(this, tr("Export Failure"), 
                tr("Failed to write to file."));
        }
    });

    btnLayout->addWidget(btnExportCsv);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    layout->addStretch();
    return tab;
}

QWidget* ResultsPage::createReportTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *mainLayout = new QHBoxLayout(tab);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // Left Panel: Configuration & Actions
    auto *leftContainer = new QWidget(tab);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    // Group 1: Customization Inputs
    auto *customBox = new QGroupBox(tr("Report Metadata Customization"), leftContainer);
    auto *formLayout = new QFormLayout(customBox);
    formLayout->setSpacing(8);

    m_editProjectName = new QLineEdit(customBox);
    m_editOperator = new QLineEdit(customBox);
    m_editCompany = new QLineEdit(customBox);
    m_editLab = new QLineEdit(customBox);
    m_editReportTitle = new QLineEdit(customBox);
    
    m_editComments = new QTextEdit(customBox);
    m_editComments->setMaximumHeight(80);
    m_editComments->installEventFilter(this);

    formLayout->addRow(tr("Report Title:"), m_editReportTitle);
    formLayout->addRow(tr("Project Name:"), m_editProjectName);
    formLayout->addRow(tr("Operator Name:"), m_editOperator);
    formLayout->addRow(tr("Company Name:"), m_editCompany);
    formLayout->addRow(tr("Laboratory:"), m_editLab);
    formLayout->addRow(tr("Notes / Comments:"), m_editComments);

    leftLayout->addWidget(customBox);

    // Group 2: Actions & Buttons
    auto *actionsBox = new QGroupBox(tr("Report Commands"), leftContainer);
    auto *actionsLayout = new QGridLayout(actionsBox);
    actionsLayout->setSpacing(8);

    m_btnPreviewReport = new QPushButton(tr("Preview Report"), actionsBox);
    m_btnPreviewReport->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    
    m_btnGenerateReport = new QPushButton(tr("Generate Report Bundle"), actionsBox);
    m_btnGenerateReport->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    m_btnGenerateReport->setStyleSheet("font-weight: bold; background-color: #0F52BA; color: white;");

    m_btnExportPdf = new QPushButton(tr("Export PDF"), actionsBox);
    m_btnExportPdf->setIcon(style()->standardIcon(QStyle::SP_FileIcon));

    m_btnExportHtml = new QPushButton(tr("Export HTML"), actionsBox);
    m_btnExportHtml->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    m_btnExportMd = new QPushButton(tr("Export Markdown"), actionsBox);
    m_btnExportMd->setIcon(style()->standardIcon(QStyle::SP_FileLinkIcon));

    m_btnOpenFolder = new QPushButton(tr("Open Output Folder"), actionsBox);
    m_btnOpenFolder->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    m_btnOpenFolder->setEnabled(false);

    actionsLayout->addWidget(m_btnPreviewReport, 0, 0);
    actionsLayout->addWidget(m_btnGenerateReport, 0, 1);
    actionsLayout->addWidget(m_btnExportPdf, 1, 0);
    actionsLayout->addWidget(m_btnExportHtml, 1, 1);
    actionsLayout->addWidget(m_btnExportMd, 2, 0);
    actionsLayout->addWidget(m_btnOpenFolder, 2, 1);

    leftLayout->addWidget(actionsBox);
    leftLayout->addStretch();

    leftContainer->setFixedWidth(340);
    mainLayout->addWidget(leftContainer);

    // Right Panel: Live Document Preview
    auto *previewBox = new QGroupBox(tr("Rendered Document Preview"), tab);
    auto *previewLayout = new QVBoxLayout(previewBox);
    previewLayout->setContentsMargins(8, 8, 8, 8);

    m_txtReportPreview = new QTextEdit(previewBox);
    m_txtReportPreview->setReadOnly(true);
    m_txtReportPreview->setStyleSheet(
        "QTextEdit { "
        "  background-color: #FFFFFF; "
        "  border: 1px solid #D1D5DB; "
        "  color: #1F2937; "
        "} "
    );
    previewLayout->addWidget(m_txtReportPreview);

    mainLayout->addWidget(previewBox, 1);

    // Wire Customization Signals (Connect edits directly to updating active session metadata with Undo/Redo support)
    auto connectLineEdit = [this](QLineEdit *edit, const std::string &fieldName, std::string &memberVar) {
        connect(edit, &QLineEdit::editingFinished, this, [this, edit, fieldName, &memberVar]() {
            if (m_isUpdatingUI) return;
            std::string oldValue = memberVar;
            std::string newValue = edit->text().toStdString();
            if (oldValue == newValue) return;

            m_presenter->parentPresenter()->undoStack()->push(
                new MetadataChangeCommand(this, m_currentSession, fieldName, oldValue, newValue)
            );
            memberVar = newValue;
            autoSaveSession();
        });
    };

    connectLineEdit(m_editProjectName, "Project Name", m_currentSession.metadata.projectName);
    connectLineEdit(m_editOperator, "Operator Name", m_currentSession.metadata.operatorName);
    connectLineEdit(m_editCompany, "Company", m_currentSession.metadata.company);
    connectLineEdit(m_editLab, "Laboratory", m_currentSession.metadata.laboratory);
    connectLineEdit(m_editReportTitle, "Report Title", m_currentSession.metadata.reportTitle);

    // Wire Command Buttons
    connect(m_btnPreviewReport, &QPushButton::clicked, this, [this]() {
        auto report = ReportGenerator::generateReport(m_currentSession, ReportFormat::HTML);
        if (report) {
            m_txtReportPreview->setHtml(QString::fromStdString(report->getContent()));
        }
    });

    connect(m_btnGenerateReport, &QPushButton::clicked, this, [this]() {
        if (m_currentSession.sessionName.empty()) {
            QMessageBox::warning(this, tr("Export Failure"), tr("No active session loaded to generate a report."));
            return;
        }

        // 1. Create unique timestamped directory under "Reports"
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
        QString reportDirName = QString("Reports/%1_%2").arg(timestamp).arg(QString::fromStdString(m_currentSession.sessionName));
        QDir dir;
        if (!dir.exists(reportDirName)) {
            dir.mkpath(reportDirName);
        }
        m_lastReportFolderPath = dir.absoluteFilePath(reportDirName);

        // 2. Programmatically save active plot screenshots
        bool hasResults = !m_currentSession.results.empty();
        if (hasResults) {
            if (m_magPlot) m_magPlot->savePng(m_lastReportFolderPath + "/Magnitude.png");
            if (m_phasePlot) m_phasePlot->savePng(m_lastReportFolderPath + "/Phase.png");
            if (m_polarPlot && m_currentSession.profile.positioner.usePositioner) {
                m_polarPlot->savePng(m_lastReportFolderPath + "/Polar.png");
            }
            if (m_smithChart) m_smithChart->savePng(m_lastReportFolderPath + "/Smith.png");

            // Save Statistics.csv
            QFile csvFile(m_lastReportFolderPath + "/Statistics.csv");
            if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&csvFile);
                out << "Metric,Value\n";
                if (m_lblStatMinMag) out << "Minimum Magnitude," << m_lblStatMinMag->text() << "\n";
                if (m_lblStatMaxMag) out << "Maximum Magnitude," << m_lblStatMaxMag->text() << "\n";
                if (m_lblStatAvgMag) out << "Average Magnitude," << m_lblStatAvgMag->text() << "\n";
                if (m_lblStatMinPhase) out << "Minimum Phase," << m_lblStatMinPhase->text() << "\n";
                if (m_lblStatMaxPhase) out << "Maximum Phase," << m_lblStatMaxPhase->text() << "\n";
                if (m_lblStatAvgPhase) out << "Average Phase," << m_lblStatAvgPhase->text() << "\n";
                if (m_lblStatPeakFreq) out << "Peak Frequency," << m_lblStatPeakFreq->text() << "\n";
                csvFile.close();
            }
        }

        // 3. Generate HTML & Markdown reports
        auto htmlReport = ReportGenerator::generateReport(m_currentSession, ReportFormat::HTML);
        auto mdReport = ReportGenerator::generateReport(m_currentSession, ReportFormat::Markdown);

        if (htmlReport) {
            QFile file(m_lastReportFolderPath + "/Report.html");
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << QString::fromStdString(htmlReport->getContent());
                file.close();
            }
            // Load preview
            m_txtReportPreview->setHtml(QString::fromStdString(htmlReport->getContent()));
        }

        if (mdReport) {
            QFile file(m_lastReportFolderPath + "/Report.md");
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << QString::fromStdString(mdReport->getContent());
                file.close();
            }
        }

        // 4. Generate real PDF from HTML report
        if (htmlReport) {
            QString pdfPath = m_lastReportFolderPath + "/Report.pdf";
            QPdfWriter pdfWriter(pdfPath);
            pdfWriter.setPageSize(QPageSize(QPageSize::A4));
            pdfWriter.setPageOrientation(QPageLayout::Portrait);
            pdfWriter.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

            QTextDocument doc;
            doc.setHtml(QString::fromStdString(htmlReport->getContent()));
            doc.setPageSize(QSizeF(pdfWriter.width(), pdfWriter.height()));
            doc.print(&pdfWriter);

            MainWindow *win = qobject_cast<MainWindow*>(m_presenter->parentPresenter()->parent());
            if (win) {
                win->settingsManager()->addRecentFile("Recent/Reports", pdfPath);
            }
        }

        m_btnOpenFolder->setEnabled(true);

        QMessageBox::information(this, tr("Report Generated"), 
            tr("Engineering Report Bundle successfully generated in:\n%1").arg(m_lastReportFolderPath));
    });

    connect(m_btnExportPdf, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, 
            tr("Save PDF Report"), "", tr("PDF Files (*.pdf)"));
        if (fileName.isEmpty()) return;

        auto htmlReport = ReportGenerator::generateReport(m_currentSession, ReportFormat::HTML);
        if (htmlReport) {
            QPdfWriter pdfWriter(fileName);
            pdfWriter.setPageSize(QPageSize(QPageSize::A4));
            pdfWriter.setPageOrientation(QPageLayout::Portrait);
            pdfWriter.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

            QTextDocument doc;
            doc.setHtml(QString::fromStdString(htmlReport->getContent()));
            doc.setPageSize(QSizeF(pdfWriter.width(), pdfWriter.height()));
            doc.print(&pdfWriter);

            QMessageBox::information(this, tr("Export Success"), tr("PDF Report saved successfully."));
        }
    });

    connect(m_btnExportHtml, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, 
            tr("Save HTML Report"), "", tr("HTML Files (*.html)"));
        if (fileName.isEmpty()) return;

        auto htmlReport = ReportGenerator::generateReport(m_currentSession, ReportFormat::HTML);
        if (htmlReport) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << QString::fromStdString(htmlReport->getContent());
                file.close();
                QMessageBox::information(this, tr("Export Success"), tr("HTML Report saved successfully."));
            }
        }
    });

    connect(m_btnExportMd, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, 
            tr("Save Markdown Report"), "", tr("Markdown Files (*.md)"));
        if (fileName.isEmpty()) return;

        auto mdReport = ReportGenerator::generateReport(m_currentSession, ReportFormat::Markdown);
        if (mdReport) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << QString::fromStdString(mdReport->getContent());
                file.close();
                QMessageBox::information(this, tr("Export Success"), tr("Markdown Report saved successfully."));
            }
        }
    });

    connect(m_btnOpenFolder, &QPushButton::clicked, this, [this]() {
        if (!m_lastReportFolderPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastReportFolderPath));
        }
    });

    return tab;
}

void ResultsPage::createBottomStatusBar(QHBoxLayout *layout) {
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(12);

    auto *parentWidget = layout->parentWidget();

    // Create wrapper status labels parented to the layout's parentWidget
    m_statusLoaded = new QLabel(tr("Loaded: Session_001/Gain Sweep"), parentWidget);
    m_statusLoaded->setStyleSheet("color: #FFFFFF; font-weight: 500;");

    m_statusMemory = new QLabel(tr("Memory: 18.2 MB"), parentWidget);
    m_statusMemory->setStyleSheet("color: #C8C8C8;");

    m_statusAnalysis = new QLabel(tr("Analysis: Offline Ready"), parentWidget);
    m_statusAnalysis->setStyleSheet("color: #4CAF50; font-weight: bold;");

    m_statusUpdate = new QLabel(tr("Updated: 2026-07-01 20:00"), parentWidget);
    m_statusUpdate->setStyleSheet("color: #C8C8C8;");

    layout->addWidget(m_statusLoaded);
    layout->addStretch();
    layout->addWidget(m_statusMemory);
    layout->addWidget(m_statusAnalysis);
    layout->addWidget(m_statusUpdate);

    // Apply border styling to status line using a divider line or styling on parent container
}

void ResultsPage::onSessionSelected(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString name = item->text(0);
    if (name == tr("Sessions") || name == tr("Session_001") || name == tr("Session_002")) {
        return;
    }
    
    // Determine session ID based on parent
    QTreeWidgetItem *parentItem = item->parent();
    if (!parentItem) return;
    QString sessionFolder = parentItem->text(0);

    MeasurementSession session;
    if (m_presenter->loadSession(sessionFolder, session)) {
        loadSessionToUI(session);
    }
}

void ResultsPage::loadSessionToUI(const MeasurementSession &session) {
    m_isUpdatingUI = true;
    m_currentSession = session;
    m_markers.clear();
    updateMarkersUI();

    if (!session.sessionName.empty()) {
        QString sessionPath = QString("Sessions/%1_session.txt").arg(QString::fromStdString(session.sessionName));
        MainWindow *win = qobject_cast<MainWindow*>(m_presenter->parentPresenter()->parent());
        if (win) {
            win->settingsManager()->addRecentFile("Recent/Sessions", sessionPath);
        }
    }

    // Populate report customization widgets (blocking signals to avoid self-triggering updates)
    if (m_editProjectName) {
        m_editProjectName->blockSignals(true);
        m_editProjectName->setText(QString::fromStdString(session.metadata.projectName));
        m_editProjectName->blockSignals(false);
    }
    if (m_editOperator) {
        m_editOperator->blockSignals(true);
        m_editOperator->setText(QString::fromStdString(session.metadata.operatorName));
        m_editOperator->blockSignals(false);
    }
    if (m_editCompany) {
        m_editCompany->blockSignals(true);
        m_editCompany->setText(QString::fromStdString(session.metadata.company));
        m_editCompany->blockSignals(false);
    }
    if (m_editLab) {
        m_editLab->blockSignals(true);
        m_editLab->setText(QString::fromStdString(session.metadata.laboratory));
        m_editLab->blockSignals(false);
    }
    if (m_editReportTitle) {
        m_editReportTitle->blockSignals(true);
        m_editReportTitle->setText(QString::fromStdString(session.metadata.reportTitle));
        m_editReportTitle->blockSignals(false);
    }
    if (m_editComments) {
        m_editComments->blockSignals(true);
        m_editComments->setPlainText(QString::fromStdString(session.metadata.comments));
        m_editComments->blockSignals(false);
    }

    // Update Top Summary
    m_lblSumName->setText(QString::fromStdString(session.sessionName));
    m_lblSumType->setText(QString::fromStdString(session.measurementType));
    m_lblSumBand->setText(QString::fromStdString(session.bandName));
    m_lblSumCal->setText(QString::fromStdString(session.calibrationFile));
    m_lblSumDate->setText(QString::fromStdString(session.timestamp));
    m_lblSumOperator->setText(QString::fromStdString(session.metadata.operatorName));
    m_lblSumStatus->setText(tr("Loaded"));
    m_lblSumStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");

    // Update Bottom Status Bar
    m_statusLoaded->setText(tr("Loaded: %1").arg(QString::fromStdString(session.sessionName)));
    m_statusUpdate->setText(tr("Updated: %1").arg(QString::fromStdString(session.timestamp)));

    if (session.results.empty()) {
        if (m_magPlot) m_magPlot->clearData();
        if (m_phasePlot) m_phasePlot->clearData();
        if (m_polarPlot) m_polarPlot->clearData();
        if (m_smithChart) m_smithChart->clearData();
#ifdef AMAS_HAS_3D_VISUALIZATION
        if (m_pattern3DPlot) m_pattern3DPlot->clearData();
#endif
        
        m_lblStatMinMag->setText(tr("N/A"));
        m_lblStatMaxMag->setText(tr("N/A"));
        m_lblStatAvgMag->setText(tr("N/A"));
        m_lblStatMinPhase->setText(tr("N/A"));
        m_lblStatMaxPhase->setText(tr("N/A"));
        m_lblStatAvgPhase->setText(tr("N/A"));
        m_lblStatPeakFreq->setText(tr("N/A"));
        m_lblStatSamples->setText(tr("0"));
        m_lblStatFreqSpan->setText(tr("N/A"));
        
        m_dataTable->setRowCount(0);
        if (m_txtReportPreview) m_txtReportPreview->clear();
        
        QMessageBox::warning(this, tr("Empty Session"), tr("The selected measurement session contains no data."));
        return;
    }

    std::vector<double> freqs;
    std::vector<double> mags;
    std::vector<double> phases;
    freqs.reserve(session.results.size());
    mags.reserve(session.results.size());
    phases.reserve(session.results.size());

    double minMag = 9999.0;
    double maxMag = -9999.0;
    double sumMag = 0.0;

    double minPhase = 9999.0;
    double maxPhase = -9999.0;
    double sumPhase = 0.0;

    double peakFreqVal = 0.0;

    for (const auto &pt : session.results) {
        double fGhz = pt.frequencyHz / 1e9;
        freqs.push_back(fGhz);
        mags.push_back(pt.magnitudeDb);
        phases.push_back(pt.phaseDeg);

        // Magnitude stats
        if (pt.magnitudeDb < minMag) minMag = pt.magnitudeDb;
        if (pt.magnitudeDb > maxMag) {
            maxMag = pt.magnitudeDb;
            peakFreqVal = fGhz;
        }
        sumMag += pt.magnitudeDb;

        // Phase stats
        if (pt.phaseDeg < minPhase) minPhase = pt.phaseDeg;
        if (pt.phaseDeg > maxPhase) maxPhase = pt.phaseDeg;
        sumPhase += pt.phaseDeg;
    }

    double avgMag = sumMag / session.results.size();
    double avgPhase = sumPhase / session.results.size();
    double minF = freqs.front();
    double maxF = freqs.back();
    double freqSpanVal = std::abs(maxF - minF);

    // Update Plots
    if (m_magPlot) m_magPlot->setData(freqs, mags, tr("Frequency (GHz)"), tr("Magnitude (dB)"));
    if (m_phasePlot) m_phasePlot->setData(freqs, phases, tr("Frequency (GHz)"), tr("Phase (deg)"));
    if (m_polarPlot) m_polarPlot->setSessionData(session.results);
    if (m_smithChart) m_smithChart->setSessionData(session.results);
#ifdef AMAS_HAS_3D_VISUALIZATION
    if (m_pattern3DPlot) m_pattern3DPlot->setSessionData(session);
#endif

    // Update Statistics Panel labels
    m_lblStatMinMag->setText(tr("%1 dB").arg(minMag, 0, 'f', 2));
    m_lblStatMaxMag->setText(tr("%1 dB").arg(maxMag, 0, 'f', 2));
    m_lblStatAvgMag->setText(tr("%1 dB").arg(avgMag, 0, 'f', 2));
    m_lblStatMinPhase->setText(tr("%1 deg").arg(minPhase, 0, 'f', 2));
    m_lblStatMaxPhase->setText(tr("%1 deg").arg(maxPhase, 0, 'f', 2));
    m_lblStatAvgPhase->setText(tr("%1 deg").arg(avgPhase, 0, 'f', 2));
    m_lblStatPeakFreq->setText(tr("%1 GHz").arg(peakFreqVal, 0, 'f', 3));
    m_lblStatSamples->setText(QString::number(session.results.size()));
    m_lblStatFreqSpan->setText(tr("%1 GHz").arg(freqSpanVal, 0, 'f', 3));

    // Update Report Text Edit
    if (m_txtReportPreview) {
        m_txtReportPreview->clear();
        m_txtReportPreview->append(tr("=================================================="));
        m_txtReportPreview->append(tr("          AMAS ANTENNA MEASUREMENT REPORT         "));
        m_txtReportPreview->append(tr("=================================================="));
        m_txtReportPreview->append(tr("Session ID: %1").arg(QString::fromStdString(session.sessionName)));
        m_txtReportPreview->append(tr("Timestamp:  %1").arg(QString::fromStdString(session.timestamp)));
        m_txtReportPreview->append(tr("Operator:   %1").arg(QString::fromStdString(session.metadata.operatorName)));
        m_txtReportPreview->append(tr("Frequency band: %1").arg(QString::fromStdString(session.bandName)));
        m_txtReportPreview->append(tr("Measurement Type: %1").arg(QString::fromStdString(session.measurementType)));
        m_txtReportPreview->append(tr("Peak Gain / Max Mag: %1 dB @ %2 GHz").arg(QString::number(maxMag, 'f', 2)).arg(QString::number(peakFreqVal, 'f', 3)));
        m_txtReportPreview->append(tr("Average Magnitude:   %1 dB").arg(QString::number(avgMag, 'f', 2)));
        m_txtReportPreview->append(tr("Average Phase:       %1 deg").arg(QString::number(avgPhase, 'f', 2)));
        m_txtReportPreview->append(tr("Frequency Span:      %1 GHz").arg(QString::number(freqSpanVal, 'f', 3)));
        m_txtReportPreview->append(tr("Data Points:         %1").arg(session.results.size()));
        m_txtReportPreview->append(tr("Calibration state:   %1").arg(QString::fromStdString(session.calibrationFile)));
        m_txtReportPreview->append(tr("=================================================="));
    }

    // Update Data Table
    m_dataTable->setRowCount(0);
    m_dataTable->setRowCount(static_cast<int>(session.results.size()));
    
    auto setCell = [this](int r, int c, double val) {
        auto *cellItem = new QTableWidgetItem();
        cellItem->setData(Qt::DisplayRole, val);
        cellItem->setTextAlignment(Qt::AlignCenter);
        m_dataTable->setItem(r, c, cellItem);
    };

    for (size_t i = 0; i < session.results.size(); ++i) {
        const auto &pt = session.results[i];
        setCell(i, 0, pt.frequencyHz / 1e9);
        setCell(i, 1, pt.magnitudeDb);
        setCell(i, 2, pt.phaseDeg);
        setCell(i, 3, pt.angleDeg);
        setCell(i, 4, pt.returnLossDb);
    }
    
    MainWindow *win = qobject_cast<MainWindow*>(m_presenter->parentPresenter()->parent());
    if (win) {
        bool gridVisible = win->settingsManager()->getValue("Plot/GridVisible", true).toBool();
        bool markersVisible = win->settingsManager()->getValue("Plot/MarkerVisible", true).toBool();
        applyPlotSettings(gridVisible, markersVisible);
    }

    m_isUpdatingUI = false;
}

void ResultsPage::onAddMarkerClicked() {
    if (m_currentSession.results.empty()) {
        QMessageBox::warning(this, tr("No Data"), tr("No active session data to place a marker on."));
        return;
    }

    int currentRow = m_dataTable->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_currentSession.results.size())) {
        currentRow = 0;
    }

    double freq = m_currentSession.results[currentRow].frequencyHz;
    for (const auto &m : m_markers) {
        if (std::abs(m.frequencyHz - freq) < 1.0) {
            QMessageBox::warning(this, tr("Duplicate Marker"), tr("A marker already exists at this frequency."));
            return;
        }
    }

    EngineeringMarker marker;
    marker.id = m_markers.empty() ? 1 : (m_markers.back().id + 1);
    marker.frequencyHz = freq;
    marker.magnitudeDb = m_currentSession.results[currentRow].magnitudeDb;
    marker.phaseDeg = m_currentSession.results[currentRow].phaseDeg;
    marker.angleDeg = m_currentSession.results[currentRow].angleDeg;
    marker.pointIndex = currentRow;

    m_markers.push_back(marker);
    updateMarkersUI();
}

void ResultsPage::onRemoveMarkerClicked() {
    int row = m_tableMarkers->currentRow();
    if (row >= 0 && row < static_cast<int>(m_markers.size())) {
        m_markers.erase(m_markers.begin() + row);
        updateMarkersUI();
    } else {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a marker in the list to delete."));
    }
}

void ResultsPage::onClearMarkersClicked() {
    m_markers.clear();
    updateMarkersUI();
}

void ResultsPage::updateMarkersUI() {
    m_tableMarkers->setRowCount(0);
    m_tableMarkers->setRowCount(static_cast<int>(m_markers.size()));

    std::vector<double> markerFreqsGhz;
    std::vector<double> markerFreqsHz;
    std::vector<QPointF> markerPolarPoints;

    auto setMarkerCell = [this](int r, int c, const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        m_tableMarkers->setItem(r, c, item);
    };

    for (size_t i = 0; i < m_markers.size(); ++i) {
        const auto &m = m_markers[i];
        setMarkerCell(i, 0, QString("M%1").arg(m.id));
        setMarkerCell(i, 1, QString::number(m.frequencyHz / 1e9, 'f', 3));
        setMarkerCell(i, 2, QString::number(m.magnitudeDb, 'f', 2));
        setMarkerCell(i, 3, QString::number(m.phaseDeg, 'f', 1));

        markerFreqsGhz.push_back(m.frequencyHz / 1e9);
        markerFreqsHz.push_back(m.frequencyHz);
        markerPolarPoints.push_back(QPointF(m.angleDeg, m.magnitudeDb));
    }

    if (m_magPlot) m_magPlot->setMarkers(markerFreqsGhz);
    if (m_phasePlot) m_phasePlot->setMarkers(markerFreqsGhz);
    if (m_polarPlot) m_polarPlot->setMarkers(markerPolarPoints);
    if (m_smithChart) m_smithChart->setMarkers(markerFreqsHz);
}

void ResultsPage::loadSessionMetadataOnly(const SessionMetadata &metadata) {
    m_isUpdatingUI = true;
    if (m_editProjectName) m_editProjectName->setText(QString::fromStdString(metadata.projectName));
    if (m_editOperator) m_editOperator->setText(QString::fromStdString(metadata.operatorName));
    if (m_editCompany) m_editCompany->setText(QString::fromStdString(metadata.company));
    if (m_editLab) m_editLab->setText(QString::fromStdString(metadata.laboratory));
    if (m_editReportTitle) m_editReportTitle->setText(QString::fromStdString(metadata.reportTitle));
    if (m_editComments) m_editComments->setPlainText(QString::fromStdString(metadata.comments));
    m_isUpdatingUI = false;
}

void ResultsPage::autoSaveSession() {
    if (!m_currentSession.sessionName.empty()) {
        QDir dir;
        dir.mkpath("Sessions");
        QString filePath = QString("Sessions/%1_session.txt").arg(QString::fromStdString(m_currentSession.sessionName));
        m_currentSession.serialize(filePath.toStdString());
    }
}

void ResultsPage::applyPlotSettings(bool gridVisible, bool markersVisible) {
    if (m_magPlot) {
        m_magPlot->setGridVisible(gridVisible);
        m_magPlot->setMarkersVisible(markersVisible);
    }
    if (m_phasePlot) {
        m_phasePlot->setGridVisible(gridVisible);
        m_phasePlot->setMarkersVisible(markersVisible);
    }
    if (m_polarPlot) {
        m_polarPlot->setGridVisible(gridVisible);
        m_polarPlot->setMarkersVisible(markersVisible);
    }
    if (m_smithChart) {
        m_smithChart->setGridVisible(gridVisible);
        m_smithChart->setMarkersVisible(markersVisible);
    }
}

bool ResultsPage::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_editComments && event->type() == QEvent::FocusOut) {
        if (!m_isUpdatingUI) {
            std::string oldValue = m_currentSession.metadata.comments;
            std::string newValue = m_editComments->toPlainText().toStdString();
            if (oldValue != newValue) {
                m_presenter->parentPresenter()->undoStack()->push(
                    new MetadataChangeCommand(this, m_currentSession, "Comments", oldValue, newValue)
                );
                m_currentSession.metadata.comments = newValue;
                autoSaveSession();
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace AMAS
