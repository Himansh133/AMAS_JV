#ifndef AMAS_BANDCONFIGURATION_H
#define AMAS_BANDCONFIGURATION_H

#include <string>
#include <vector>

namespace AMAS {

struct BandInfo {
    std::string name;
    double startFreqHz;
    double stopFreqHz;
    int sweepPoints;
    std::string defaultCalibrationFile;
};

class BandConfiguration {
public:
    // Retrieve all standard predefined RF bands
    static const std::vector<BandInfo>& getPredefinedBands();

    // Query band information by name (e.g. "K-Band")
    static bool getBandByName(const std::string& name, BandInfo& outBand);

    // Query band information by a frequency within its range
    static bool getBandByFrequency(double freqHz, BandInfo& outBand);
};

} // namespace AMAS

#endif // AMAS_BANDCONFIGURATION_H
