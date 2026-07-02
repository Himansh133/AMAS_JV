#include "measurement/BandConfiguration.h"
#include <algorithm>

namespace AMAS {

const std::vector<BandInfo>& BandConfiguration::getPredefinedBands() {
    static const std::vector<BandInfo> bands = {
        { "8-12 GHz",   8.0e9,  12.0e9, 401, "calchamber8_12ghz.sta" },
        { "12-18 GHz",  12.0e9, 18.0e9, 601, "calchamber12_18ghz.sta" },
        { "18-26.5 GHz", 18.0e9, 26.5e9, 851, "calchamber18_26p5ghz.sta" },
        { "26.5-40 GHz", 26.5e9, 40.0e9, 136, "calchamber26p5g_40ghz.sta" }
    };
    return bands;
}

bool BandConfiguration::getBandByName(const std::string& name, BandInfo& outBand) {
    const auto& bands = getPredefinedBands();
    auto it = std::find_if(bands.begin(), bands.end(), [&name](const BandInfo& band) {
        return band.name == name;
    });
    if (it != bands.end()) {
        outBand = *it;
        return true;
    }
    return false;
}

bool BandConfiguration::getBandByFrequency(double freqHz, BandInfo& outBand) {
    const auto& bands = getPredefinedBands();
    auto it = std::find_if(bands.begin(), bands.end(), [freqHz](const BandInfo& band) {
        return (freqHz >= band.startFreqHz && freqHz <= band.stopFreqHz);
    });
    if (it != bands.end()) {
        outBand = *it;
        return true;
    }
    return false;
}

} // namespace AMAS
