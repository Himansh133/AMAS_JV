#include "measurement/CalibrationManager.h"
#include <fstream>
#include <filesystem>

namespace AMAS {

CalibrationManager::CalibrationManager() {
    loadDefaultMappings();
}

void CalibrationManager::registerCalibration(const std::string& measurementType,
                                             const std::string& bandName,
                                             const std::string& filePath) {
    m_mappings[measurementType][bandName] = filePath;
}

std::string CalibrationManager::getCalibrationFile(const std::string& measurementType,
                                                   const std::string& bandName) const {
    auto typeIt = m_mappings.find(measurementType);
    if (typeIt != m_mappings.end()) {
        auto bandIt = typeIt->second.find(bandName);
        if (bandIt != typeIt->second.end()) {
            return bandIt->second;
        }
    }
    return "";
}

bool CalibrationManager::validateCalibrationFile(const std::string& filePath) const {
    if (filePath.empty()) {
        return false;
    }
    
    // Check if the file exists on the local file system
    // Sometimes files might be loaded remotely, but for local state verification:
    std::error_code ec;
    return std::filesystem::exists(filePath, ec);
}

void CalibrationManager::loadDefaultMappings() {
    // S11 default calibration presets
    registerCalibration("S11", "8-12 GHz", "calchamber_s11_1_18ghz.sta");
    registerCalibration("S11", "12-18 GHz", "calchamber_s11_1_18ghz.sta");
    registerCalibration("S11", "18-26.5 GHz", "calchamber_s11_18_26p5ghz.sta");
    registerCalibration("S11", "26.5-40 GHz", "calchamber_s11_26p5_40ghz.sta");

    // S21 / Gain default calibration presets
    registerCalibration("S21", "8-12 GHz", "calchamber8_12ghz.sta");
    registerCalibration("S21", "12-18 GHz", "calchamber12_18ghz.sta");
    registerCalibration("S21", "18-26.5 GHz", "calchamber18_26p5ghz.sta");
    registerCalibration("S21", "26.5-40 GHz", "calchamber26p5g_40ghz.sta");
    registerCalibration("S21", "1-18 GHz", "calchamber1_18ghz.sta");
}

} // namespace AMAS
