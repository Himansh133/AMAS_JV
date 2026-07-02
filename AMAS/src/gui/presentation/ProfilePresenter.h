#ifndef AMAS_PROFILEPRESENTER_H
#define AMAS_PROFILEPRESENTER_H

#include <QObject>
#include "measurement/MeasurementProfile.h"

namespace AMAS {

class MeasurementPresenter;

class ProfilePresenter : public QObject {
    Q_OBJECT
public:
    explicit ProfilePresenter(MeasurementPresenter *parent);
    ~ProfilePresenter() override = default;

    QStringList getProfiles() const;
    bool loadProfile(const QString &profileName, MeasurementProfile &outProfile);
    bool saveProfile(const QString &profileName, const MeasurementProfile &profile);

signals:
    void profilesChanged();
    void profileLoaded(const QString &profileName);
    void profileSaved(const QString &profileName);

private:
    MeasurementPresenter *m_parent;
};

} // namespace AMAS

#endif // AMAS_PROFILEPRESENTER_H
