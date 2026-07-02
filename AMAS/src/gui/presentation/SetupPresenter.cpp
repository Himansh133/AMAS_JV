#include "SetupPresenter.h"
#include "MeasurementPresenter.h"

namespace AMAS {

SetupPresenter::SetupPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
}

void SetupPresenter::updateProfile(const MeasurementProfile &profile) {
    m_parent->setCurrentProfile(profile);
    emit profileUpdated(profile);
    emit m_parent->currentProfileChanged(QString::fromStdString(profile.profileName));
}

MeasurementProfile SetupPresenter::getProfile() const {
    return m_parent->currentProfile();
}

QStringList SetupPresenter::getAvailableCalibrations() const {
    return {
        "calchamber8_12ghz.sta",
        "calchamber12_18ghz.sta",
        "PCB_SOLT_Calibration.sta",
        "Waveguide_ShortShim.sta",
        "None"
    };
}

} // namespace AMAS
