#ifndef MEASUREMENTPROFILE_H
#define MEASUREMENTPROFILE_H

#include <string>
#include <vector>
#include "SweepConfig.h"

namespace AMAS {

enum class MeasurementType {
    S11,
    S21,
    Gain,
    RadiationPattern,
    Polarization
};

struct BandConfig {
    double startFreq_Hz;
    double stopFreq_Hz;
    int sweepPoints;
    std::string calibrationFile;
};

struct MeasurementProfile {
    MeasurementType type;
    BandConfig band;
    double ifbw_Hz = 1000.0;
    double power_dBm = -15.0;
    SParamType sparam = SParamType::S21;
    
    // Positioner settings
    float startAngle_deg = 0.0f;
    float stopAngle_deg = 90.0f;
    float stepAngle_deg = 10.0f;
    int settleTime_ms = 150;
};

// Preset configurations based on calibration.txt
inline BandConfig getS11BandPreset(int choice) {
    switch (choice) {
        case 1: // 8-12 GHz
            return { 8.0e9, 12.0e9, 401, "calchamber_s11_1_18ghz.sta" };
        case 2: // 12-18 GHz
            return { 12.0e9, 18.0e9, 601, "calchamber_s11_1_18ghz.sta" };
        case 3: // 18-26.5 GHz
            return { 18.0e9, 26.5e9, 851, "calchamber_s11_18_26p5ghz.sta" };
        case 4: // 26.5-40 GHz
            return { 26.5e9, 40.0e9, 136, "calchamber_s11_26p5_40ghz.sta" };
        default:
            return { 18.0e9, 26.5e9, 851, "calchamber_s11_18_26p5ghz.sta" };
    }
}

inline BandConfig getGainS21BandPreset(int choice) {
    switch (choice) {
        case 1: // 8-12 GHz
            return { 8.0e9, 12.0e9, 401, "calchamber8_12ghz.sta" };
        case 2: // 12-18 GHz
            return { 12.0e9, 18.0e9, 601, "calchamber12_18ghz.sta" };
        case 3: // 18-26.5 GHz
            return { 18.0e9, 26.5e9, 851, "calchamber18_26p5ghz.sta" };
        case 4: // 26.5-40 GHz
            return { 26.5e9, 40.0e9, 136, "calchamber26p5g_40ghz.sta" };
        case 5: // 1-18 GHz
            return { 1.0e9, 18.0e9, 1701, "calchamber1_18ghz.sta" };
        default:
            return { 18.0e9, 26.5e9, 851, "calchamber18_26p5ghz.sta" };
    }
}

} // namespace AMAS

#endif // MEASUREMENTPROFILE_H
