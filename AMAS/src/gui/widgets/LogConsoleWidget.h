#ifndef AMAS_LOGCONSOLEWIDGET_H
#define AMAS_LOGCONSOLEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateTime>
#include <QList>

namespace AMAS {

struct LogEntry {
    QDateTime timestamp;
    QString severity;   // INFO, WARNING, ERROR, SUCCESS
    QString subsystem;  // GUI, VNA, POSITIONER, PROCESSOR, REPORT, SYSTEM
    QString message;
};

class LogConsoleWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogConsoleWidget(QWidget *parent = nullptr);
    ~LogConsoleWidget() override = default;

    void addLog(const QString &severity, const QString &subsystem, const QString &message);
    void clearLog();
    void saveLog();

private slots:
    void onSearchChanged(const QString &text);
    void onFilterChanged(int index);

private:
    void updateTable();

    QTableWidget *m_table;
    QLineEdit    *m_searchEdit;
    QComboBox    *m_severityCombo;
    QPushButton  *m_btnClear;
    QPushButton  *m_btnSave;

    QList<LogEntry> m_logs;
};

} // namespace AMAS

#endif // AMAS_LOGCONSOLEWIDGET_H
