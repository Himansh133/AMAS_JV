#ifndef MEASUREMENTSESSION_H
#define MEASUREMENTSESSION_H

#include <vector>
#include <string>
#include "MeasurementProfile.h"

namespace AMAS {

struct MeasurementPoint {
    float angle_deg;
    double frequency_Hz;
    double magnitude_dB;
    double phase_deg;
};

class MeasurementSession {
public:
    MeasurementProfile profile;
    std::vector<MeasurementPoint> points;
    std::string timestamp;
    
    void clear() {
        points.clear();
    }
};

} // namespace AMAS

#endif // MEASUREMENTSESSION_H
