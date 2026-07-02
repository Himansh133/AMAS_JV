#ifndef AMAS_DEVICESPAGE_H
#define AMAS_DEVICESPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>

namespace AMAS {

class DevicesPresenter;

class DevicesPage : public QWidget {
    Q_OBJECT
public:
    explicit DevicesPage(DevicesPresenter *presenter, QWidget *parent = nullptr);
    ~DevicesPage() override = default;

private:
    void createFieldFoxPanel(QGroupBox *box);
    void createPositionerPanel(QGroupBox *box);
    void createSummaryPanel(QGroupBox *box);
    void createLogPanel(QGroupBox *box);

    // Helpers to create styled status lights
    QWidget* createStatusRow(const QString &label, QLabel *&indicator, QLabel *&value, bool isActive, const QString &activeText, const QString &inactiveText);

    // VNA Panel Widgets
    QLabel *m_lblVnaStatus;
    QPushButton *m_btnVnaConnect;
    QPushButton *m_btnVnaDisconnect;
    QPushButton *m_btnVnaRefresh;
    QPushButton *m_btnVnaIdentify;

    // Positioner Panel Widgets
    QLabel *m_lblPosStatus;
    QPushButton *m_btnPosConnect;
    QPushButton *m_btnPosDisconnect;
    QPushButton *m_btnPosHome;
    QPushButton *m_btnPosRefresh;
    QPushButton *m_btnPosMove;

    // Summary Indicators
    QLabel *m_indVna;
    QLabel *m_indPos;
    QLabel *m_indReady;
    QLabel *m_valVna;
    QLabel *m_valPos;
    QLabel *m_valReady;

    // Log Widget
    QTextEdit *m_logEdit;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void updateConnectionStatus(bool vnaConnected, bool posConnected);
    void appendLogMessage(const QString &msg);

private:
    DevicesPresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_DEVICESPAGE_H
