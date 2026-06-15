#ifndef MOCKPOSITIONERDRIVER_H
#define MOCKPOSITIONERDRIVER_H

#include "IPositionerDriver.h"

namespace AMAS {

class MockPositionerDriver : public IPositionerDriver {
public:
    MockPositionerDriver();
    ~MockPositionerDriver() override;

    bool connect(const std::string& port) override;
    void disconnect() override;
    bool moveTo(float angle_deg) override;
    float getCurrentAngle() override;
    bool waitForSettle(int timeout_ms) override;
    void home() override;
    void emergencyStop() override;
    bool isConnected() const override;

private:
    bool m_isConnected;
    float m_currentAngle;
    float m_targetAngle;
    bool m_isMoving;
};

} // namespace AMAS

#endif // MOCKPOSITIONERDRIVER_H
