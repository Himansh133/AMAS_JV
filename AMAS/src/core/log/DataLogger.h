#ifndef DATALOGGER_H
#define DATALOGGER_H

#include "MeasurementSession.h"
#include <string>

namespace AMAS {

class DataLogger {
public:
    static bool exportToCSV(const MeasurementSession& session, const std::string& filename);
};

} // namespace AMAS

#endif // DATALOGGER_H
