#ifndef AMAS_CALIBRATIONMANAGER_H
#define AMAS_CALIBRATIONMANAGER_H

#include <string>
#include <map>
#include "measurement/MeasurementProfile.h"

namespace AMAS {

class CalibrationManager {
public:
    CalibrationManager();
    ~CalibrationManager() = default;

    // Registers a calibration file mapping for a specific measurement type and band
    void registerCalibration(const std::string& measurementType,
                             const std::string& bandName,
                             const std::string& filePath);

    // Retrieves the registered calibration file path
    std::string getCalibrationFile(const std::string& measurementType,
                                   const std::string& bandName) const;

    // Validates if the calibration file exists and is accessible
    bool validateCalibrationFile(const std::string& filePath) const;

private:
    // Nested map structure: MeasurementType string -> (BandName -> FilePath)
    std::map<std::string, std::map<std::string, std::string>> m_mappings;

    // Prepopulate the standard calibration mappings
    void loadDefaultMappings();
};

} // namespace AMAS

#endif // AMAS_CALIBRATIONMANAGER_H
