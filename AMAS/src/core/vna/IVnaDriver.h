#ifndef IVNADRIVER_H
#define IVNADRIVER_H

#include <string>
#include <vector>
#include "SweepConfig.h"

namespace AMAS {

struct SParamData {
    SParamType type;
    std::vector<double> frequencies_Hz;
    std::vector<double> magnitudes_dB;
    std::vector<double> phases_deg;
};

class IVnaDriver {
public:
    virtual ~IVnaDriver() = default;
    
    virtual bool connect(const std::string& resource) = 0;
    virtual void disconnect() = 0;
    virtual bool loadCalibrationState(const std::string& filename) { return true; }
    virtual bool configure(const SweepConfig& config) = 0;
    virtual bool triggerSweep() = 0;
    virtual std::vector<SParamData> readSParameters() = 0;
    virtual bool isConnected() const = 0;
    virtual std::string getIdnString() = 0;
};

} // namespace AMAS

#endif // IVNADRIVER_H
