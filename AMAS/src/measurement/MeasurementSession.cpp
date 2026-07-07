#include "measurement/MeasurementSession.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace AMAS {

static std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : "";
}

MeasurementSession::MeasurementSession()
    : sessionName("New Session")
    , measurementType("S11")
    , timestamp("")
    , bandName("8-12 GHz")
    , calibrationFile("")
    , outputFolder("")
{
}

void MeasurementSession::clear() {
    results.clear();
}

bool MeasurementSession::serialize(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << "# AMAS Measurement Session Summary\n";
    file << "sessionName=" << sessionName << "\n";
    file << "measurementType=" << measurementType << "\n";
    file << "timestamp=" << timestamp << "\n";
    file << "bandName=" << bandName << "\n";
    file << "calibrationFile=" << calibrationFile << "\n";
    file << "outputFolder=" << outputFolder << "\n";
    file << "operatorName=" << metadata.operatorName << "\n";
    file << "notes=" << metadata.notes << "\n";
    file << "antennaModel=" << metadata.antennaModel << "\n";
    file << "projectName=" << metadata.projectName << "\n";
    file << "company=" << metadata.company << "\n";
    file << "laboratory=" << metadata.laboratory << "\n";
    file << "reportTitle=" << metadata.reportTitle << "\n";
    file << "comments=" << metadata.comments << "\n";
    file << "resultsCount=" << results.size() << "\n";

    return true;
}

bool MeasurementSession::deserialize(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    clear();

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string key, val;
        if (std::getline(ss, key, '=') && std::getline(ss, val)) {
            key = trim(key);
            val = trim(val);

            if (key == "sessionName") sessionName = val;
            else if (key == "measurementType") measurementType = val;
            else if (key == "timestamp") timestamp = val;
            else if (key == "bandName") bandName = val;
            else if (key == "calibrationFile") calibrationFile = val;
            else if (key == "outputFolder") outputFolder = val;
            else if (key == "operatorName") metadata.operatorName = val;
            else if (key == "notes") metadata.notes = val;
            else if (key == "antennaModel") metadata.antennaModel = val;
            else if (key == "projectName") metadata.projectName = val;
            else if (key == "company") metadata.company = val;
            else if (key == "laboratory") metadata.laboratory = val;
            else if (key == "reportTitle") metadata.reportTitle = val;
            else if (key == "comments") metadata.comments = val;
        }
    }
    return true;
}

} // namespace AMAS
