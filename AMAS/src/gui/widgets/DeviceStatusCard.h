#ifndef AMAS_DEVICESTATUSCARD_H
#define AMAS_DEVICESTATUSCARD_H

#include <QFrame>
#include <QLabel>
#include <QString>

namespace AMAS {

class DeviceStatusCard : public QFrame {
    Q_OBJECT
public:
    explicit DeviceStatusCard(const QString &title, QWidget *parent = nullptr);
    ~DeviceStatusCard() override = default;

    // Set connection state parameters
    void setConnectionStatus(bool isConnected, const QString &statusText);
    void setDetails(const QString &connectionDetail, const QString &extraLabel, const QString &extraValue);

private:
    QLabel *m_lblTitle;
    QLabel *m_lblIndicator;
    QLabel *m_lblStatus;
    QLabel *m_lblConnDetail;
    QLabel *m_lblExtraLabel;
    QLabel *m_lblExtraValue;
};

} // namespace AMAS

#endif // AMAS_DEVICESTATUSCARD_H
