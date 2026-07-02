#ifndef AMAS_DATALOGGER_H
#define AMAS_DATALOGGER_H

#include <string>
#include "measurement/MeasurementSession.h"

namespace AMAS {

class DataLogger {
public:
    DataLogger();
    ~DataLogger() = default;

    // Sets the base folder directory where all sessions will be saved
    void setBaseDirectory(const std::string& baseDir);
    std::string getBaseDirectory() const;

    // Creates a folder for this session and exports all data (CSV + Session Metadata summary)
    bool logSession(const MeasurementSession& session, std::string& outSessionDir);

    // Static helper to export session coordinates directly to a target CSV path
    static bool exportToCSV(const MeasurementSession& session, const std::string& filePath);

private:
    std::string m_baseDirectory;
};

} // namespace AMAS

#endif // AMAS_DATALOGGER_H
