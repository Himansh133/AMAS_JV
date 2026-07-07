#ifndef AMAS_MEASUREMENTPROGRESSPAGE_H
#define AMAS_MEASUREMENTPROGRESSPAGE_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QHBoxLayout>

namespace AMAS {

class ProgressPresenter;

class MeasurementProgressPage : public QWidget {
    Q_OBJECT
public:
    explicit MeasurementProgressPage(ProgressPresenter *presenter, QWidget *parent = nullptr);
    ~MeasurementProgressPage() override = default;

private:
    // Layout sections
    void createTopInfoBar(QWidget *container);
    void createProgressPanel(QGroupBox *box);
    void createInstrumentStatusPanel(QGroupBox *box);
    void createStatisticsPanel(QGroupBox *box);
    void createLiveLogPanel(QGroupBox *box);
    void createEmergencyControls(QHBoxLayout *layout);

    // Metadata widgets
    QLabel *m_lblMeasType;
    QLabel *m_lblProfile;
    QLabel *m_lblCal;
    QLabel *m_lblOperator;
    QLabel *m_lblSessionId;
    QLabel *m_lblStartTime;

    // Progress widgets
    QProgressBar *m_progressBar;
    QLabel *m_lblCurrAngle;
    QLabel *m_lblCurrFreq;
    QLabel *m_lblCurrSweep;
    QLabel *m_lblEstRemaining;
    QLabel *m_lblStatus;

    // Instrument Status widgets
    QLabel *m_lblVnaConn;
    QLabel *m_lblVnaFreq;
    QLabel *m_lblVnaTrigger;
    QLabel *m_lblPosConn;
    QLabel *m_lblPosAz;
    QLabel *m_lblPosEl;
    QLabel *m_lblPosPl;
    QLabel *m_lblPosMotion;

    // Statistics widgets
    QLabel *m_lblStatCompleted;
    QLabel *m_lblStatRemaining;
    QLabel *m_lblStatTotal;
    QLabel *m_lblStatAvgTime;
    QLabel *m_lblStatBand;
    QLabel *m_lblStatSize;

    // Log widget
    QTextEdit *m_logEdit;

    // Buttons
    QPushButton *m_btnPause;
    QPushButton *m_btnResume;
    QPushButton *m_btnStop;
    QPushButton *m_btnAbort;
    QPushButton *m_btnExport;

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onResumeClicked();
    void onStopClicked();
    void onAbortClicked();
    void onExportClicked();

    void updateProgress(float progressPercent, const QString &statusMessage);
    void handleMeasurementFinished(bool success);
    void appendLogMessage(const QString &msg);

private:
    ProgressPresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTPROGRESSPAGE_H
