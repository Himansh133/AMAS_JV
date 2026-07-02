#ifndef MEASUREMENTCONTROLLER_H
#define MEASUREMENTCONTROLLER_H

#include "IVnaDriver.h"
#include "IPositionerDriver.h"
#include "MeasurementSession.h"
#include <string>

namespace AMAS {

class MeasurementController {
public:
    MeasurementController(IVnaDriver* vna, IPositionerDriver* positioner);
    ~MeasurementController() = default;

    bool connectHardware(const std::string& vnaResource, const std::string& posPort);
    void disconnectHardware();

    bool runMeasurement(const MeasurementProfile& profile, MeasurementSession& outSession);

private:
    IVnaDriver* m_vna;
    IPositionerDriver* m_positioner;
    bool m_vnaConnected;
    bool m_posConnected;

    std::string getSParamChoiceString(SParamType type) const;
};

} // namespace AMAS

#endif // MEASUREMENTCONTROLLER_H
