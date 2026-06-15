#ifndef IPOSITIONERDRIVER_H
#define IPOSITIONERDRIVER_H

#include <string>

namespace AMAS {

class IPositionerDriver {
public:
    virtual ~IPositionerDriver() = default;

    virtual bool connect(const std::string& port) = 0;
    virtual void disconnect() = 0;
    virtual bool moveTo(float angle_deg) = 0;
    virtual float getCurrentAngle() = 0;
    virtual bool waitForSettle(int timeout_ms) = 0;
    virtual void home() = 0;
    virtual void emergencyStop() = 0;
    virtual bool isConnected() const = 0;
};

} // namespace AMAS

#endif // IPOSITIONERDRIVER_H
