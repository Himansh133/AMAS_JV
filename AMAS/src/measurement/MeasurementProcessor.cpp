#include "measurement/MeasurementProcessor.h"
#include "Logger.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace AMAS {

const double PI = 3.14159265358979323846;

double MeasurementProcessor::linearToDb(double linear) {
    if (linear <= 1e-15) {
        return -150.0;
    }
    return 20.0 * std::log10(linear);
}

double MeasurementProcessor::dbToLinear(double db) {
    return std::pow(10.0, db / 20.0);
}

double MeasurementProcessor::calculateVSWR(double s11Db) {
    // S11 must be <= 0 dB in passive networks, but clamp for safety
    if (s11Db > 0.0) s11Db = 0.0;
    double rho = dbToLinear(s11Db);
    if (rho >= 0.999) {
        return 999.0; // clamp near open/short infinity
    }
    return (1.0 + rho) / (1.0 - rho);
}

double MeasurementProcessor::calculateReturnLoss(double s11Db) {
    return -s11Db;
}

bool MeasurementProcessor::parseRawSDATA(const std::string& rawData,
                                         std::vector<double>& outMagDb,
                                         std::vector<double>& outPhaseDeg) {
    std::stringstream ss(rawData);
    std::string valStr;
    std::vector<double> rawValues;

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

    for (size_t i = 0; i < numPoints; ++i) {
        double real = rawValues[2 * i];
        double imag = rawValues[2 * i + 1];

        double mag = std::sqrt(real * real + imag * imag);
        double magDb = linearToDb(mag);
        double phaseDeg = std::atan2(imag, real) * 180.0 / PI;

        outMagDb.push_back(magDb);
        outPhaseDeg.push_back(phaseDeg);
    }

    return true;
}

RadiationPatternMetrics MeasurementProcessor::analyzePattern(const std::vector<float>& anglesDeg,
                                                              const std::vector<double>& magnitudesDb) {
    RadiationPatternMetrics metrics;
    if (anglesDeg.empty() || magnitudesDb.empty() || anglesDeg.size() != magnitudesDb.size()) {
        return metrics;
    }

    // 1. Peak Gain and Direction
    auto maxIt = std::max_element(magnitudesDb.begin(), magnitudesDb.end());
    size_t maxIdx = std::distance(magnitudesDb.begin(), maxIt);
    metrics.peakGainDb = *maxIt;
    metrics.beamDirectionDeg = anglesDeg[maxIdx];

    // 2. Beamwidth (3dB and 10dB)
    double target3dB = metrics.peakGainDb - 3.0;
    double target10dB = metrics.peakGainDb - 10.0;

    // Simple search for crossings left and right of peak
    int left3 = -1, right3 = -1;
    int left10 = -1, right10 = -1;

    // Search left
    for (int i = static_cast<int>(maxIdx); i >= 0; --i) {
        if (magnitudesDb[i] < target3dB && left3 == -1) {
            left3 = i;
        }
        if (magnitudesDb[i] < target10dB && left10 == -1) {
            left10 = i;
        }
    }
    // Search right
    for (size_t i = maxIdx; i < magnitudesDb.size(); ++i) {
        if (magnitudesDb[i] < target3dB && right3 == -1) {
            right3 = static_cast<int>(i);
        }
        if (magnitudesDb[i] < target10dB && right10 == -1) {
            right10 = static_cast<int>(i);
        }
    }

    if (left3 != -1 && right3 != -1) {
        metrics.beamwidth3dB = std::abs(anglesDeg[right3] - anglesDeg[left3]);
    } else {
        metrics.beamwidth3dB = 0.0;
    }

    if (left10 != -1 && right10 != -1) {
        metrics.beamwidth10dB = std::abs(anglesDeg[right10] - anglesDeg[left10]);
    } else {
        metrics.beamwidth10dB = 0.0;
    }

    // 3. Front-to-Back Ratio
    // Back angle is 180 degrees opposite to peak direction
    float backAngle = metrics.beamDirectionDeg + 180.0f;
    if (backAngle > 360.0f) backAngle -= 360.0f;
    else if (backAngle < 0.0f) backAngle += 360.0f;

    // Find the element closest to backAngle
    double minDiff = 360.0;
    double backGainDb = magnitudesDb[0];
    for (size_t i = 0; i < anglesDeg.size(); ++i) {
        double diff = std::abs(anglesDeg[i] - backAngle);
        if (diff > 180.0) diff = 360.0 - diff;
        if (diff < minDiff) {
            minDiff = diff;
            backGainDb = magnitudesDb[i];
        }
    }
    metrics.frontToBackRatioDb = metrics.peakGainDb - backGainDb;

    // 4. Side Lobe Level (SLL)
    // SLL is the maximum level of any peak outside the main lobe (defined roughly by 10dB beamwidth bounds)
    double peakSideLobe = -150.0;
    int startMainLobe = (left10 != -1) ? left10 : 0;
    int endMainLobe = (right10 != -1) ? right10 : static_cast<int>(magnitudesDb.size() - 1);

    for (int i = 0; i < static_cast<int>(magnitudesDb.size()); ++i) {
        if (i >= startMainLobe && i <= endMainLobe) {
            continue; // Skip the main lobe region
        }
        
        // Simple peak detection (local maxima)
        bool isLocalMax = true;
        if (i > 0 && magnitudesDb[i] < magnitudesDb[i - 1]) isLocalMax = false;
        if (i < static_cast<int>(magnitudesDb.size() - 1) && magnitudesDb[i] < magnitudesDb[i + 1]) isLocalMax = false;

        if (isLocalMax && magnitudesDb[i] > peakSideLobe) {
            peakSideLobe = magnitudesDb[i];
        }
    }

    metrics.sideLobeLevelDb = (peakSideLobe > -150.0) ? (peakSideLobe - metrics.peakGainDb) : -100.0;

    return metrics;
}

double MeasurementProcessor::calculateGain(double measuredPowerDb, double calFactorDb) {
    return measuredPowerDb + calFactorDb;
}

double MeasurementProcessor::calculateAxialRatio(double magHorizontalDb, double magVerticalDb) {
    return std::abs(magHorizontalDb - magVerticalDb);
}

} // namespace AMAS
