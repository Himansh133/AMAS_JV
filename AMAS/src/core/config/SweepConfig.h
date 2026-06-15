#ifndef SWEEPCONFIG_H
#define SWEEPCONFIG_H

#include <vector>

namespace AMAS {

enum class SParamType {
    S11,
    S21,
    S12,
    S22
};

struct SweepConfig {
    double startFreq_Hz = 2.4e9;
    double stopFreq_Hz = 2.5e9;
    int    numPoints = 201;
    double ifbw_Hz = 1000.0;
    double power_dBm = -10.0;
    std::vector<SParamType> sparams = { SParamType::S11, SParamType::S21 };
    
    float  startAngle_deg = 0.0f;
    float  stopAngle_deg = 360.0f;
    float  stepAngle_deg = 5.0f;
    int    settleTime_ms = 100;

    bool validate() const {
        if (startFreq_Hz <= 0.0 || stopFreq_Hz <= 0.0 || startFreq_Hz >= stopFreq_Hz) {
            return false;
        }
        if (numPoints <= 1 || numPoints > 10001) { // FieldFox typical range
            return false;
        }
        if (ifbw_Hz <= 0.0 || ifbw_Hz > 100000.0) { // FieldFox typical IF bandwidth limits
            return false;
        }
        if (power_dBm < -50.0 || power_dBm > 20.0) { // Safe hardware limits
            return false;
        }
        if (sparams.empty()) {
            return false;
        }
        if (startAngle_deg < -360.0f || startAngle_deg > 360.0f) {
            return false;
        }
        if (stopAngle_deg < -360.0f || stopAngle_deg > 360.0f) {
            return false;
        }
        if (stepAngle_deg <= 0.0f || stepAngle_deg > 360.0f) {
            return false;
        }
        if (settleTime_ms < 0) {
            return false;
        }
        return true;
    }
};

} // namespace AMAS

#endif // SWEEPCONFIG_H
