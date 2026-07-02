#include "MeasurementProcessor.h"
#include "Logger.h"
#include <cmath>
#include <sstream>

namespace AMAS {

bool MeasurementProcessor::parseRawSDATA(const std::string& rawData,
                                         std::vector<double>& outMagDb,
                                         std::vector<double>& outPhaseDeg) {
    std::stringstream ss(rawData);
    std::string valStr;
    std::vector<double> rawValues;
    
    // Split by comma
    while (std::getline(ss, valStr, ',')) {
        if (valStr.empty()) continue;
        try {
            rawValues.push_back(std::stod(valStr));
        } catch (...) {
            Log::error("MeasurementProcessor: Double parsing failed for value: '" + valStr + "'");
            return false;
        }
    }

    if (rawValues.size() % 2 != 0) {
        Log::error("MeasurementProcessor: Complex data must contain even number of values. Found: " + 
                   std::to_string(rawValues.size()));
        return false;
    }

    size_t numPoints = rawValues.size() / 2;
    outMagDb.clear();
    outPhaseDeg.clear();
    outMagDb.reserve(numPoints);
    outPhaseDeg.reserve(numPoints);

    const double PI = 3.14159265358979323846;

    for (size_t i = 0; i < numPoints; ++i) {
        double real = rawValues[2 * i];
        double imag = rawValues[2 * i + 1];

        double mag = std::sqrt(real * real + imag * imag);
        double magDb = -150.0;
        if (mag > 1e-15) {
            magDb = 20.0 * std::log10(mag);
        }
        
        double phaseDeg = std::atan2(imag, real) * 180.0 / PI;

        outMagDb.push_back(magDb);
        outPhaseDeg.push_back(phaseDeg);
    }

    return true;
}

} // namespace AMAS
