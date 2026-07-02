#ifndef MEASUREMENTPROCESSOR_H
#define MEASUREMENTPROCESSOR_H

#include <vector>
#include <string>

namespace AMAS {

class MeasurementProcessor {
public:
    static bool parseRawSDATA(const std::string& rawData,
                             std::vector<double>& outMagDb,
                             std::vector<double>& outPhaseDeg);
};

} // namespace AMAS

#endif // MEASUREMENTPROCESSOR_H
