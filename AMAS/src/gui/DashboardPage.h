#ifndef AMAS_DASHBOARDPAGE_H
#define AMAS_DASHBOARDPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QListWidget>
#include "widgets/DeviceStatusCard.h"
#include "widgets/MeasProfileCard.h"
#include "widgets/QuickActionButton.h"

namespace AMAS {

class DashboardPresenter;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(DashboardPresenter *presenter, QWidget *parent = nullptr);
    ~DashboardPage() override = default;

private:
    void setupTopRow();
    void setupQuickActions();
    void setupRecentTable();
    void setupLogPanel();

    // Top Row Widgets
    DeviceStatusCard *m_cardVna;
    DeviceStatusCard *m_cardPositioner;
    MeasProfileCard  *m_cardProfile;

    // Quick Action Tiles
    QuickActionButton *m_btnConnect;
    QuickActionButton *m_btnSetup;
    QuickActionButton *m_btnCal;
    QuickActionButton *m_btnResults;
    QuickActionButton *m_btnReport;

    // Data lists
    QTableWidget *m_tableRecent;
    QListWidget  *m_logList;

private slots:
    void updateDashboardState();

private:
    DashboardPresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_DASHBOARDPAGE_H
