#include "logger/DataLogger.h"
#include "Logger.h"
#include <fstream>
#include <iomanip>
#include <filesystem>

namespace AMAS {

DataLogger::DataLogger()
    : m_baseDirectory("measurements")
{
}

void DataLogger::setBaseDirectory(const std::string& baseDir) {
    m_baseDirectory = baseDir;
}

std::string DataLogger::getBaseDirectory() const {
    return m_baseDirectory;
}

bool DataLogger::logSession(const MeasurementSession& session, std::string& outSessionDir) {
    // 1. Create a safe directory name based on session name and timestamp
    std::string safeSessionName = session.sessionName;
    std::replace(safeSessionName.begin(), safeSessionName.end(), ' ', '_');
    std::replace(safeSessionName.begin(), safeSessionName.end(), '/', '_');
    std::replace(safeSessionName.begin(), safeSessionName.end(), '\\', '_');

    std::string safeTimestamp = session.timestamp;
    std::replace(safeTimestamp.begin(), safeTimestamp.end(), ':', '-');
    std::replace(safeTimestamp.begin(), safeTimestamp.end(), ' ', '_');

    std::string folderName = safeSessionName;
    if (!safeTimestamp.empty()) {
        folderName += "_" + safeTimestamp;
    }

    std::filesystem::path base(m_baseDirectory);
    std::filesystem::path sessionPath = base / folderName;
    outSessionDir = sessionPath.string();

    Log::info("DataLogger: Creating session folder at: " + outSessionDir);
    
    std::error_code ec;
    if (!std::filesystem::create_directories(sessionPath, ec) && ec) {
        Log::error("DataLogger: Failed to create session directory. System error: " + ec.message());
        return false;
    }

    // 2. Export raw S-parameter sweeps to CSV
    std::string csvPath = (sessionPath / "measurement_results.csv").string();
    if (!exportToCSV(session, csvPath)) {
        Log::error("DataLogger: Failed to write CSV file inside session folder.");
        return false;
    }

    // 3. Serialize metadata to summary text file
    std::string metaPath = (sessionPath / "session_metadata.txt").string();
    if (!session.serialize(metaPath)) {
        Log::error("DataLogger: Failed to write session metadata file.");
        return false;
    }

    Log::info("DataLogger: Successfully logged session files.");
    return true;
}

bool DataLogger::exportToCSV(const MeasurementSession& session, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Log::error("DataLogger: Failed to open CSV file for writing: " + filePath);
        return false;
    }

    // Write header including calculated engineering results
    file << "Angle_deg,Frequency_Hz,Magnitude_dB,Phase_deg,ReturnLoss_dB,VSWR\n";

    for (const auto& pt : session.results) {
        file << std::fixed << std::setprecision(1) << pt.angleDeg << ","
             << std::fixed << std::setprecision(0) << pt.frequencyHz << ","
             << std::fixed << std::setprecision(4) << pt.magnitudeDb << ","
             << std::fixed << std::setprecision(4) << pt.phaseDeg << ","
             << std::fixed << std::setprecision(4) << pt.returnLossDb << ","
             << std::fixed << std::setprecision(4) << pt.vswr << "\n";
    }

    file.close();
    return true;
}

} // namespace AMAS
