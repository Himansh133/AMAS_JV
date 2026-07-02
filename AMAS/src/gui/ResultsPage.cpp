#include "ResultsPage.h"
#include "presentation/ResultsPresenter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QFrame>
#include <QSettings>

namespace AMAS {

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

    // Bottom Status Bar
    auto *statusBarLayout = new QHBoxLayout();
    createBottomStatusBar(statusBarLayout);
    mainLayout->addLayout(statusBarLayout);
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

    // Populate mock items
    auto *rootItem = new QTreeWidgetItem(m_treeBrowser);
    rootItem->setText(0, tr("Sessions"));
    rootItem->setExpanded(true);

    auto *sess1 = new QTreeWidgetItem(rootItem);
    sess1->setText(0, tr("Session_001"));
    sess1->setExpanded(true);
    
    auto *itemS11_1 = new QTreeWidgetItem(sess1);
    itemS11_1->setText(0, tr("S11 Reflection"));
    auto *itemGain_1 = new QTreeWidgetItem(sess1);
    itemGain_1->setText(0, tr("Gain Sweep"));
    auto *itemPat2D_1 = new QTreeWidgetItem(sess1);
    itemPat2D_1->setText(0, tr("Pattern 2D (Horn)"));

    auto *sess2 = new QTreeWidgetItem(rootItem);
    sess2->setText(0, tr("Session_002"));
    sess2->setExpanded(true);

    auto *itemS11_2 = new QTreeWidgetItem(sess2);
    itemS11_2->setText(0, tr("S11 Patch"));
    auto *itemPat3D_2 = new QTreeWidgetItem(sess2);
    itemPat3D_2->setText(0, tr("Pattern 3D (Array)"));

    layout->addWidget(m_treeBrowser);
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
    m_tabWidget->addTab(createGraphsTab(m_tabWidget), tr("Graphs"));
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

QWidget* ResultsPage::createGraphsTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QHBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(16);

    auto createPlaceholderFrame = [tab](const QString &title) -> QFrame* {
        auto *frame = new QFrame(tab);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setStyleSheet(
            "QFrame { "
            "  border: 1px dashed #555555; "
            "  background-color: #1E1E1E; "
            "  border-radius: 4px; "
            "} "
        );
        auto *frameLayout = new QVBoxLayout(frame);
        auto *lbl = new QLabel(title, frame);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #777777; font-weight: bold; border: none;");
        frameLayout->addWidget(lbl);
        return frame;
    };

    layout->addWidget(createPlaceholderFrame(tr("S11 Plot")));
    layout->addWidget(createPlaceholderFrame(tr("Gain Plot")));
    layout->addWidget(createPlaceholderFrame(tr("Radiation Pattern")));
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

    formLayout->addRow(tr("Maximum Gain:"), new QLabel(tr("12.4 dBi"), statsBox));
    formLayout->addRow(tr("Minimum Gain:"), new QLabel(tr("-25.3 dBi"), statsBox));
    formLayout->addRow(tr("Average Gain:"), new QLabel(tr("3.1 dBi"), statsBox));
    formLayout->addRow(tr("Peak Frequency:"), new QLabel(tr("10.12 GHz"), statsBox));
    formLayout->addRow(tr("3dB Beam Width:"), new QLabel(tr("42.5 deg"), statsBox));
    formLayout->addRow(tr("Radiation Efficiency:"), new QLabel(tr("88.4 %"), statsBox));
    formLayout->addRow(tr("Axial Ratio:"), new QLabel(tr("1.1 dB"), statsBox));

    layout->addWidget(statsBox);
    layout->addStretch();
    return tab;
}

QWidget* ResultsPage::createReportTab(QWidget *parent) {
    auto *tab = new QWidget(parent);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *previewBox = new QGroupBox(tr("Report Document Preview"), tab);
    auto *previewLayout = new QVBoxLayout(previewBox);
    
    auto *previewText = new QTextEdit(previewBox);
    previewText->setReadOnly(true);
    previewText->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1E1E1E; "
        "  border: 1px solid #3F3F46; "
        "  font-family: Consolas, monospace; "
        "  color: #C8C8C8; "
        "} "
    );
    previewText->append(tr("=================================================="));
    previewText->append(tr("          AMAS ANTENNA MEASUREMENT REPORT         "));
    previewText->append(tr("=================================================="));
    previewText->append(tr("Session ID: SESS-2026-0701"));
    previewText->append(tr("Operator: Operator-01"));
    previewText->append(tr("Calibration state: Coaxial 2-Port SOLT valid"));
    previewText->append(tr("Frequency band: 8-12 GHz"));
    previewText->append(tr("Peak Gain: 12.4 dBi @ 10.12 GHz"));
    previewText->append(tr("Radiation efficiency: 88.4%"));
    previewText->append(tr("=================================================="));

    previewLayout->addWidget(previewText);
    layout->addWidget(previewBox);

    // Export controls
    auto *exportBar = new QHBoxLayout();
    exportBar->setSpacing(10);

    auto *btnPdf = new QPushButton(tr("Generate PDF"), tab);
    btnPdf->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    btnPdf->setMinimumWidth(140);
    btnPdf->setMinimumHeight(32);

    auto *btnCsv = new QPushButton(tr("Export CSV"), tab);
    btnCsv->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    btnCsv->setMinimumWidth(140);
    btnCsv->setMinimumHeight(32);

    auto *btnImg = new QPushButton(tr("Export Image"), tab);
    btnImg->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    btnImg->setMinimumWidth(140);
    btnImg->setMinimumHeight(32);

    auto *btnPrint = new QPushButton(tr("Print"), tab);
    btnPrint->setIcon(style()->standardIcon(QStyle::SP_CommandLink));
    btnPrint->setMinimumWidth(140);
    btnPrint->setMinimumHeight(32);

    exportBar->addWidget(btnPdf);
    exportBar->addWidget(btnCsv);
    exportBar->addWidget(btnImg);
    exportBar->addWidget(btnPrint);
    exportBar->addStretch();

    layout->addLayout(exportBar);
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

        // Update Data Table
        m_dataTable->setRowCount(0);
        m_dataTable->setRowCount(session.results.size());
        
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
    }
}

} // namespace AMAS
