#ifndef MOCKVNADRIVER_H
#define MOCKVNADRIVER_H

#include "IVnaDriver.h"
#include "SweepConfig.h"

namespace AMAS {

class MockVnaDriver : public IVnaDriver {
public:
    MockVnaDriver();
    ~MockVnaDriver() override;

    // IVnaDriver interface implementation
    bool connect(const std::string& resource) override;
    void disconnect() override;
    bool configure(const SweepConfig& config) override;
    bool triggerSweep() override;
    std::vector<SParamData> readSParameters() override;
    bool isConnected() const override;
    std::string getIdnString() override;

private:
    bool m_isConnected;
    SweepConfig m_config;
    std::string m_idnString;
};

} // namespace AMAS

#endif // MOCKVNADRIVER_H
