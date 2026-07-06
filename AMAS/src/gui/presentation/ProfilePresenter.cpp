#include "ProfilePresenter.h"
#include "MeasurementPresenter.h"

namespace AMAS {

ProfilePresenter::ProfilePresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
    // Synchronize profiles list when changes occur in controller
    connect(m_parent->controller().get(), &MeasurementController::profileLoaded, this, [this]() {
        emit profilesChanged();
    });
    connect(m_parent->controller().get(), &MeasurementController::profileSaved, this, [this]() {
        emit profilesChanged();
    });
    connect(m_parent->controller().get(), &MeasurementController::profileDeleted, this, [this]() {
        emit profilesChanged();
    });
}

QStringList ProfilePresenter::getProfiles() const {
    QStringList list;
    for (const auto& p : m_parent->controller()->getProfiles()) {
        list.append(QString::fromStdString(p.profileName));
    }
    return list;
}

bool ProfilePresenter::loadProfile(const QString &profileName, MeasurementProfile &outProfile) {
    for (const auto& p : m_parent->controller()->getProfiles()) {
        if (p.profileName == profileName.toStdString()) {
            outProfile = p;
            emit profileLoaded(profileName);
            return true;
        }
    }
    return false;
}

bool ProfilePresenter::saveProfile(const QString &profileName, const MeasurementProfile &profile) {
    MeasurementProfile p = profile;
    p.profileName = profileName.toStdString();
    bool success = m_parent->controller()->saveProfile(p);
    if (success) {
        emit profileSaved(profileName);
        emit profilesChanged();
    }
    return success;
}

bool ProfilePresenter::deleteProfile(const QString &profileName) {
    bool success = m_parent->controller()->deleteProfile(profileName.toStdString());
    if (success) {
        emit profilesChanged();
    }
    return success;
}

bool ProfilePresenter::duplicateProfile(const QString &sourceName, const QString &destName) {
    MeasurementProfile source;
    if (loadProfile(sourceName, source)) {
        source.profileName = destName.toStdString();
        return saveProfile(destName, source);
    }
    return false;
}

void ProfilePresenter::setActiveProfile(const MeasurementProfile &profile) {
    m_parent->controller()->setActiveProfile(profile);
}

} // namespace AMAS
