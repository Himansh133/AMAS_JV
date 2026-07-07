#include "LogConsoleWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QLabel>
#include <QStyle>
#include <QApplication>

namespace AMAS {

LogConsoleWidget::LogConsoleWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // Toolbar row
    auto *toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    toolbar->addWidget(new QLabel(tr("Search:"), this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search messages..."));
    m_searchEdit->setMinimumWidth(180);
    m_searchEdit->setStyleSheet("color: #FFFFFF; background-color: #2D2D30; border: 1px solid #3F3F46; padding: 2px 6px; border-radius: 4px;");
    toolbar->addWidget(m_searchEdit);

    toolbar->addWidget(new QLabel(tr("Severity:"), this));
    m_severityCombo = new QComboBox(this);
    m_severityCombo->addItems({tr("ALL"), tr("INFO"), tr("WARNING"), tr("ERROR"), tr("SUCCESS")});
    m_severityCombo->setStyleSheet("color: #FFFFFF; background-color: #2D2D30; border: 1px solid #3F3F46; border-radius: 4px; padding: 2px 6px;");
    toolbar->addWidget(m_severityCombo);

    toolbar->addStretch();

    m_btnClear = new QPushButton(tr("Clear"), this);
    m_btnClear->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogResetButton));
    m_btnClear->setStyleSheet("padding: 2px 10px; border-radius: 4px;");
    toolbar->addWidget(m_btnClear);

    m_btnSave = new QPushButton(tr("Save Log..."), this);
    m_btnSave->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_btnSave->setStyleSheet("padding: 2px 10px; border-radius: 4px;");
    toolbar->addWidget(m_btnSave);

    mainLayout->addLayout(toolbar);

    // Table view
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Timestamp"), tr("Severity"), tr("Subsystem"), tr("Message")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { "
        "  background-color: #1E1E1E; "
        "  alternate-background-color: #252526; "
        "  border: 1px solid #3F3F46; "
        "  font-family: Consolas, monospace; "
        "  font-size: 11px; "
        "} "
        "QHeaderView::section { "
        "  background-color: #2D2D30; "
        "  color: #FFFFFF; "
        "  border: 1px solid #3F3F46; "
        "  padding: 4px; "
        "} "
    );

    mainLayout->addWidget(m_table);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &LogConsoleWidget::onSearchChanged);
    connect(m_severityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LogConsoleWidget::onFilterChanged);
    connect(m_btnClear, &QPushButton::clicked, this, &LogConsoleWidget::clearLog);
    connect(m_btnSave, &QPushButton::clicked, this, &LogConsoleWidget::saveLog);

    // Add initial startup log entries
    addLog("INFO", "SYSTEM", tr("Structured Application Log Console Initialized."));
}

void LogConsoleWidget::addLog(const QString &severity, const QString &subsystem, const QString &message) {
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.severity = severity.toUpper();
    entry.subsystem = subsystem.toUpper();
    entry.message = message;

    m_logs.append(entry);
    updateTable();
}

void LogConsoleWidget::clearLog() {
    m_logs.clear();
    updateTable();
}

void LogConsoleWidget::saveLog() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Application Log"), "", tr("Log Files (*.log);;Text Files (*.txt)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const auto &entry : m_logs) {
            out << QString("[%1] [%2] [%3] %4\n")
                   .arg(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                   .arg(entry.severity.leftJustified(7, ' '))
                   .arg(entry.subsystem.leftJustified(10, ' '))
                   .arg(entry.message);
        }
        file.close();
        QMessageBox::information(this, tr("Logs Exported"), tr("Application logs saved successfully."));
    } else {
        QMessageBox::warning(this, tr("Save Failure"), tr("Failed to write log file."));
    }
}

void LogConsoleWidget::onSearchChanged(const QString &text) {
    Q_UNUSED(text);
    updateTable();
}

void LogConsoleWidget::onFilterChanged(int index) {
    Q_UNUSED(index);
    updateTable();
}

void LogConsoleWidget::updateTable() {
    m_table->setRowCount(0);

    QString filterSeverity = m_severityCombo->currentText();
    QString filterSearch = m_searchEdit->text().trimmed();

    for (const auto &entry : m_logs) {
        // Apply severity filter
        if (filterSeverity != tr("ALL") && entry.severity != filterSeverity) {
            continue;
        }

        // Apply keyword search filter
        if (!filterSearch.isEmpty() && !entry.message.contains(filterSearch, Qt::CaseInsensitive) && !entry.subsystem.contains(filterSearch, Qt::CaseInsensitive)) {
            continue;
        }

        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Col 0: Timestamp
        auto *itemTime = new QTableWidgetItem(entry.timestamp.toString("hh:mm:ss.zzz"));
        itemTime->setForeground(QBrush(QColor("#808080")));
        m_table->setItem(row, 0, itemTime);

        // Col 1: Severity
        auto *itemSev = new QTableWidgetItem(entry.severity);
        if (entry.severity == "ERROR") {
            itemSev->setForeground(QBrush(QColor("#FF1744")));
            itemSev->setFont(QFont("Consolas", 11, QFont::Bold));
        } else if (entry.severity == "WARNING") {
            itemSev->setForeground(QBrush(QColor("#FFD600")));
            itemSev->setFont(QFont("Consolas", 11, QFont::Bold));
        } else if (entry.severity == "SUCCESS") {
            itemSev->setForeground(QBrush(QColor("#00E676")));
            itemSev->setFont(QFont("Consolas", 11, QFont::Bold));
        } else {
            itemSev->setForeground(QBrush(QColor("#00E5FF"))); // INFO
        }
        m_table->setItem(row, 1, itemSev);

        // Col 2: Subsystem
        auto *itemSub = new QTableWidgetItem(entry.subsystem);
        itemSub->setForeground(QBrush(QColor("#C8C8C8")));
        m_table->setItem(row, 2, itemSub);

        // Col 3: Message
        auto *itemMsg = new QTableWidgetItem(entry.message);
        itemMsg->setForeground(QBrush(QColor("#FFFFFF")));
        m_table->setItem(row, 3, itemMsg);
    }
    m_table->scrollToBottom();
}

} // namespace AMAS
