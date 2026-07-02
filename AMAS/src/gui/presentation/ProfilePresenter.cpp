#include "ProfilePresenter.h"
#include "MeasurementPresenter.h"

namespace AMAS {

ProfilePresenter::ProfilePresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
}

QStringList ProfilePresenter::getProfiles() const {
    return {
        "Horn 8-12 GHz",
        "Patch 12-18 GHz",
        "PCB Antenna",
        "Waveguide"
    };
}

bool ProfilePresenter::loadProfile(const QString &profileName, MeasurementProfile &outProfile) {
    outProfile.profileName = profileName.toStdString();
    outProfile.calibrationFile = "calchamber8_12ghz.sta";
    outProfile.sweepPoints = 401;
    outProfile.outputPowerDbm = -10.0;
    outProfile.ifBandwidthHz = 1000.0;

    if (profileName.contains("Horn")) {
        outProfile.measurementType = "Gain";
        outProfile.startFrequencyHz = 8.0e9;
        outProfile.stopFrequencyHz = 12.0e9;
        outProfile.positioner.usePositioner = true;
        outProfile.positioner.startAngleDeg = 0.0f;
        outProfile.positioner.stopAngleDeg = 180.0f;
        outProfile.positioner.stepAngleDeg = 10.0f;
    } else {
        outProfile.measurementType = "S11";
        outProfile.startFrequencyHz = 12.0e9;
        outProfile.stopFrequencyHz = 18.0e9;
        outProfile.positioner.usePositioner = false;
    }

    emit profileLoaded(profileName);
    return true;
}

bool ProfilePresenter::saveProfile(const QString &profileName, const MeasurementProfile &profile) {
    m_parent->setCurrentProfile(profile);
    emit profileSaved(profileName);
    emit profilesChanged();
    return true;
}

} // namespace AMAS
