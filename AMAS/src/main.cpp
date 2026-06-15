#include <QCoreApplication>
#include "Application.h"
#include "Logger.h"
#include "MockVnaDriver.h"
#include "MockPositionerDriver.h"

using namespace AMAS;

int main(int argc, char *argv[]) {
    // We use QCoreApplication for now since we are in console mode.
    // This will easily change to QApplication when we build the GUI.
    QCoreApplication a(argc, argv);

    Application app;
    if (!app.initialize()) {
        return 1;
    }
    app.run();

    // Run a quick HAL smoke test to verify drivers function correctly in isolation
    Log::info("====================================================");
    Log::info("                RUNNING HAL SMOKE TEST              ");
    Log::info("====================================================");

    // 1. Test Mock VNA
    MockVnaDriver vna;
    Log::info("Connecting to VNA...");
    if (vna.connect("GPIB0::18::INSTR")) {
        SweepConfig config;
        config.startFreq_Hz = 2.4e9;
        config.stopFreq_Hz = 2.5e9;
        config.numPoints = 11; // 11 points for visibility
        config.power_dBm = -10.0;
        config.ifbw_Hz = 1000.0;
        config.sparams = { SParamType::S11, SParamType::S21 };

        Log::info("Configuring VNA...");
        if (vna.configure(config)) {
            Log::info("Triggering Sweep...");
            if (vna.triggerSweep()) {
                Log::info("Reading S-parameters...");
                auto results = vna.readSParameters();
                
                for (const auto& trace : results) {
                    std::string traceType = (trace.type == SParamType::S11) ? "S11" : "S21";
                    Log::info("Trace: " + traceType + ", Data points: " + std::to_string(trace.magnitudes_dB.size()));
                    if (trace.magnitudes_dB.size() >= 3) {
                        Log::info("  Start: Freq=" + std::to_string(trace.frequencies_Hz.front() / 1e9) + 
                                  " GHz, Mag=" + std::to_string(trace.magnitudes_dB.front()) + " dB, Phase=" +
                                  std::to_string(trace.phases_deg.front()) + " deg");
                        Log::info("  Mid:   Freq=" + std::to_string(trace.frequencies_Hz[5] / 1e9) + 
                                  " GHz, Mag=" + std::to_string(trace.magnitudes_dB[5]) + " dB, Phase=" +
                                  std::to_string(trace.phases_deg[5]) + " deg");
                        Log::info("  End:   Freq=" + std::to_string(trace.frequencies_Hz.back() / 1e9) + 
                                  " GHz, Mag=" + std::to_string(trace.magnitudes_dB.back()) + " dB, Phase=" +
                                  std::to_string(trace.phases_deg.back()) + " deg");
                    }
                }
            }
        }
        vna.disconnect();
    }

    // 2. Test Mock Positioner
    MockPositionerDriver positioner;
    Log::info("Connecting to Positioner...");
    if (positioner.connect("COM3")) {
        Log::info("Moving turntable to 45.0 degrees...");
        if (positioner.moveTo(45.0f)) {
            positioner.waitForSettle(5000);
        }
        
        Log::info("Homing positioner...");
        positioner.home();
        
        positioner.disconnect();
    }

    Log::info("====================================================");
    Log::info("             HAL SMOKE TEST COMPLETED               ");
    Log::info("====================================================");

    // QCoreApplication is not exec'd here as this is a sequential console smoke test.
    // We can exit normally.
    return 0;
}
