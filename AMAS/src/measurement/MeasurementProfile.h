#ifndef AMAS_MEASUREMENTPROFILE_H
#define AMAS_MEASUREMENTPROFILE_H

#include <string>

namespace AMAS {

struct PositionerSettings {
    bool usePositioner = true;
    float startAngleDeg = 0.0f;
    float stopAngleDeg = 180.0f;
    float stepAngleDeg = 10.0f;
    int settleTimeMs = 150;
};

class MeasurementProfile {
public:
    MeasurementProfile();
    ~MeasurementProfile() = default;

    std::string profileName;
    std::string measurementType; // "S11", "S21", "Gain", "RadiationPattern"
    double startFrequencyHz;
    double stopFrequencyHz;
    int sweepPoints;
    double outputPowerDbm;
    double ifBandwidthHz;
    std::string calibrationFile;
    PositionerSettings positioner;

    // Load profile from a simple key-value config file
    bool loadFromFile(const std::string& filePath);

    // Save profile to a simple key-value config file
    bool saveToFile(const std::string& filePath) const;
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTPROFILE_H
