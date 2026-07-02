#include "DataLogger.h"
#include "Logger.h"
#include <fstream>
#include <iomanip>

namespace AMAS {

bool DataLogger::exportToCSV(const MeasurementSession& session, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Log::error("DataLogger: Failed to open file for export: " + filename);
        return false;
    }

    // Determine columns based on type
    bool hasAngle = (session.profile.type == MeasurementType::RadiationPattern ||
                     session.profile.type == MeasurementType::Polarization);

    if (hasAngle) {
        file << "Angle_deg,Frequency_Hz,Magnitude_dB,Phase_deg\n";
        for (const auto& pt : session.points) {
            file << std::fixed << std::setprecision(1) << pt.angle_deg << ","
                 << std::fixed << std::setprecision(0) << pt.frequency_Hz << ","
                 << std::fixed << std::setprecision(4) << pt.magnitude_dB << ","
                 << std::fixed << std::setprecision(4) << pt.phase_deg << "\n";
        }
    } else {
        file << "Frequency_Hz,Magnitude_dB,Phase_deg\n";
        for (const auto& pt : session.points) {
            file << std::fixed << std::setprecision(0) << pt.frequency_Hz << ","
                 << std::fixed << std::setprecision(4) << pt.magnitude_dB << ","
                 << std::fixed << std::setprecision(4) << pt.phase_deg << "\n";
        }
    }

    file.close();
    Log::info("DataLogger: Successfully exported session data to: " + filename);
    return true;
}

} // namespace AMAS
