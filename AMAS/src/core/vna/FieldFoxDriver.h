#ifndef FIELDFOXDRIVER_H
#define FIELDFOXDRIVER_H

#include "IVnaDriver.h"
#include "VISASession.h"
#include "SweepConfig.h"

namespace AMAS {

class FieldFoxDriver : public IVnaDriver {
public:
    FieldFoxDriver();
    ~FieldFoxDriver() override;

    // IVnaDriver interface implementation
    bool connect(const std::string& resource) override;
    void disconnect() override;
    bool configure(const SweepConfig& config) override;
    bool triggerSweep() override;
    std::vector<SParamData> readSParameters() override;
    bool isConnected() const override;
    std::string getIdnString() override;

private:
    VISASession m_session;
    SweepConfig m_currentConfig;
    std::string m_idnString;
    bool m_isConnected;

    // Helper for parsing raw SCPI real/imaginary comma-separated arrays
    bool parseSDataString(const std::string& rawData, 
                          std::vector<double>& outMagDb, 
                          std::vector<double>& outPhaseDeg) const;
};

} // namespace AMAS

#endif // FIELDFOXDRIVER_H
