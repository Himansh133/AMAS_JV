#include "measurement/MeasurementProfile.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace AMAS {

// Helper function to trim whitespace
static std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : "";
}

MeasurementProfile::MeasurementProfile()
    : profileName("Default Profile")
    , measurementType("S11")
    , startFrequencyHz(8.0e9)
    , stopFrequencyHz(12.0e9)
    , sweepPoints(401)
    , outputPowerDbm(-15.0)
    , ifBandwidthHz(1000.0)
    , calibrationFile("")
{
}

bool MeasurementProfile::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue; // Skip comments and empty lines
        }

        std::stringstream ss(line);
        std::string key, val;
        if (std::getline(ss, key, '=') && std::getline(ss, val)) {
            key = trim(key);
            val = trim(val);

            if (key == "profileName") profileName = val;
            else if (key == "measurementType") measurementType = val;
            else if (key == "startFrequencyHz") startFrequencyHz = std::stod(val);
            else if (key == "stopFrequencyHz") stopFrequencyHz = std::stod(val);
            else if (key == "sweepPoints") sweepPoints = std::stoi(val);
            else if (key == "outputPowerDbm") outputPowerDbm = std::stod(val);
            else if (key == "ifBandwidthHz") ifBandwidthHz = std::stod(val);
            else if (key == "calibrationFile") calibrationFile = val;
            else if (key == "usePositioner") positioner.usePositioner = (val == "true" || val == "1");
            else if (key == "startAngleDeg") positioner.startAngleDeg = std::stof(val);
            else if (key == "stopAngleDeg") positioner.stopAngleDeg = std::stof(val);
            else if (key == "stepAngleDeg") positioner.stepAngleDeg = std::stof(val);
            else if (key == "settleTimeMs") positioner.settleTimeMs = std::stoi(val);
        }
    }
    return true;
}

bool MeasurementProfile::saveToFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << "# AMAS Measurement Profile Configuration\n";
    file << "profileName=" << profileName << "\n";
    file << "measurementType=" << measurementType << "\n";
    file << "startFrequencyHz=" << std::fixed << startFrequencyHz << "\n";
    file << "stopFrequencyHz=" << std::fixed << stopFrequencyHz << "\n";
    file << "sweepPoints=" << sweepPoints << "\n";
    file << "outputPowerDbm=" << std::fixed << outputPowerDbm << "\n";
    file << "ifBandwidthHz=" << std::fixed << ifBandwidthHz << "\n";
    file << "calibrationFile=" << calibrationFile << "\n";
    file << "usePositioner=" << (positioner.usePositioner ? "true" : "false") << "\n";
    file << "startAngleDeg=" << std::fixed << positioner.startAngleDeg << "\n";
    file << "stopAngleDeg=" << std::fixed << positioner.stopAngleDeg << "\n";
    file << "stepAngleDeg=" << std::fixed << positioner.stepAngleDeg << "\n";
    file << "settleTimeMs=" << positioner.settleTimeMs << "\n";

    return true;
}

} // namespace AMAS
