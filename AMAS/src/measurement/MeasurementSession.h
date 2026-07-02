#ifndef AMAS_MEASUREMENTSESSION_H
#define AMAS_MEASUREMENTSESSION_H

#include <string>
#include <vector>
#include "MeasurementProfile.h"

namespace AMAS {

struct ProcessedMeasurementPoint {
    float angleDeg = 0.0f;
    double frequencyHz = 0.0;
    double magnitudeDb = 0.0;
    double phaseDeg = 0.0;
    double returnLossDb = 0.0;
    double vswr = 1.0;
};

struct SessionMetadata {
    std::string operatorName = "Engineer";
    std::string notes = "";
    std::string antennaModel = "AUT-001";
};

class MeasurementSession {
public:
    MeasurementSession();
    ~MeasurementSession() = default;

    std::string sessionName;
    std::string measurementType; // "S11", "S21", etc.
    std::string timestamp;
    std::string bandName;
    std::string calibrationFile;
    MeasurementProfile profile;
    std::string outputFolder;
    SessionMetadata metadata;

    std::vector<ProcessedMeasurementPoint> results;

    void clear();

    // Serialize session summary and metadata to file
    bool serialize(const std::string& filePath) const;

    // Load session summary from file
    bool deserialize(const std::string& filePath);
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTSESSION_H
