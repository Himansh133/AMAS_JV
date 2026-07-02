#ifndef AMAS_MEASUREMENTPROCESSOR_H
#define AMAS_MEASUREMENTPROCESSOR_H

#include <vector>
#include <string>
#include <complex>

namespace AMAS {

struct RadiationPatternMetrics {
    double peakGainDb = 0.0;
    double beamDirectionDeg = 0.0;
    double beamwidth3dB = 0.0;
    double beamwidth10dB = 0.0;
    double frontToBackRatioDb = 0.0;
    double sideLobeLevelDb = 0.0;
};

class MeasurementProcessor {
public:
    // Basic conversions
    static double linearToDb(double linear);
    static double dbToLinear(double db);
    static double calculateVSWR(double s11Db);
    static double calculateReturnLoss(double s11Db);

    // Compute complex data (real, imag) into magnitude (dB) and phase (degrees)
    static bool parseRawSDATA(const std::string& rawData,
                             std::vector<double>& outMagDb,
                             std::vector<double>& outPhaseDeg);

    // Analyze radiation pattern from angular sweep results
    static RadiationPatternMetrics analyzePattern(const std::vector<float>& anglesDeg,
                                                   const std::vector<double>& magnitudesDb);

    // Compute relative or absolute gain
    static double calculateGain(double measuredPowerDb, double calFactorDb);

    // Compute Axial Ratio from horizontal and vertical co-polarization measurements
    static double calculateAxialRatio(double magHorizontalDb, double magVerticalDb);
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTPROCESSOR_H
