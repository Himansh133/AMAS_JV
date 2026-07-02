#ifndef AMAS_SETUPPRESENTER_H
#define AMAS_SETUPPRESENTER_H

#include <QObject>
#include "measurement/MeasurementProfile.h"

namespace AMAS {

class MeasurementPresenter;

class SetupPresenter : public QObject {
    Q_OBJECT
public:
    explicit SetupPresenter(MeasurementPresenter *parent);
    ~SetupPresenter() override = default;

    // Save and Load Profile
    void updateProfile(const MeasurementProfile &profile);
    MeasurementProfile getProfile() const;

    QStringList getAvailableCalibrations() const;

signals:
    void profileUpdated(const MeasurementProfile &profile);

private:
    MeasurementPresenter *m_parent;
};

} // namespace AMAS

#endif // AMAS_SETUPPRESENTER_H
