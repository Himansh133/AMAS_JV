#include "Application.h"
#include "FieldFoxDriver.h"
#include "Logger.h"
#include "MockPositionerDriver.h"
#include "MockVnaDriver.h"
#include "TAP3001Driver.h"
#include <QCoreApplication>
#include <iostream>
#include <string>

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

  std::cout << "\nAMAS - Diagnostic HAL Test Interface" << std::endl;
  std::cout << "=====================================" << std::endl;
  std::cout << "1. Run with Mock Drivers (Simulation)" << std::endl;
  std::cout << "2. Run with Physical Hardware" << std::endl;
  std::cout << "Select mode [1-2, default: 2]: ";

  std::string choice;
  std::getline(std::cin, choice);
  bool usePhysical = true;
  if (choice == "1") {
    usePhysical = false;
  }

  Log::info("====================================================");
  Log::info(usePhysical
                ? "        RUNNING PHYSICAL HARDWARE SMOKE TEST        "
                : "             RUNNING MOCK HARDWARE SMOKE TEST       ");
  Log::info("====================================================");

  IVnaDriver *vna = nullptr;
  IPositionerDriver *positioner = nullptr;
  std::string vnaResource;
  std::string posPort;

  if (usePhysical) {
    vna = new FieldFoxDriver();
    positioner = new TAP3001Driver();
    vnaResource = "TCPIP0::192.168.113.206::inst0::INSTR";
    posPort = "COM3";
  } else {
    vna = new MockVnaDriver();
    positioner = new MockPositionerDriver();
    vnaResource = "GPIB0::18::INSTR";
    posPort = "COM3";
  }

  // 1. Test VNA
  Log::info("Connecting to VNA at " + vnaResource + " ...");
  if (vna->connect(vnaResource)) {
    SweepConfig config;
    if (usePhysical) {
      config.startFreq_Hz = 21.9e9;
      config.stopFreq_Hz = 22.1e9;
    } else {
      config.startFreq_Hz = 2.4e9;
      config.stopFreq_Hz = 2.5e9;
    }
    config.numPoints = 11; // 11 points for visibility
    config.power_dBm = -15.0;
    config.ifbw_Hz = 1000.0;
    config.sparams = {SParamType::S11, SParamType::S21};

    Log::info("Configuring VNA...");
    if (vna->configure(config)) {
      Log::info("Triggering Sweep...");
      if (vna->triggerSweep()) {
        Log::info("Reading S-parameters...");
        auto results = vna->readSParameters();

        for (const auto &trace : results) {
          std::string traceType =
              (trace.type == SParamType::S11) ? "S11" : "S21";
          Log::info("Trace: " + traceType + ", Data points: " +
                    std::to_string(trace.magnitudes_dB.size()));
          if (trace.magnitudes_dB.size() >= 3) {
            Log::info(
                "  Start: Freq=" +
                std::to_string(trace.frequencies_Hz.front() / 1e9) +
                " GHz, Mag=" + std::to_string(trace.magnitudes_dB.front()) +
                " dB, Phase=" + std::to_string(trace.phases_deg.front()) +
                " deg");
            Log::info("  Mid:   Freq=" +
                      std::to_string(trace.frequencies_Hz[5] / 1e9) +
                      " GHz, Mag=" + std::to_string(trace.magnitudes_dB[5]) +
                      " dB, Phase=" + std::to_string(trace.phases_deg[5]) +
                      " deg");
            Log::info(
                "  End:   Freq=" +
                std::to_string(trace.frequencies_Hz.back() / 1e9) +
                " GHz, Mag=" + std::to_string(trace.magnitudes_dB.back()) +
                " dB, Phase=" + std::to_string(trace.phases_deg.back()) +
                " deg");
          }
        }
      }
    }
    vna->disconnect();
  } else {
    Log::error("VNA connection failed.");
  }

  // 2. Test Positioner
  Log::info("Connecting to Positioner at " + posPort + " ...");
  if (positioner->connect(posPort)) {
    Log::info("Moving turntable to 45.0 degrees...");
    if (positioner->moveTo(45.0f)) {
      positioner->waitForSettle(10000);
    }

    Log::info("Homing positioner...");
    positioner->home();
    if (usePhysical) {
      positioner->waitForSettle(10000);
    }

    positioner->disconnect();
  } else {
    Log::error("Positioner connection failed.");
  }

  delete vna;
  delete positioner;

  Log::info("====================================================");
  Log::info("             HAL SMOKE TEST COMPLETED               ");
  Log::info("====================================================");

  return 0;
}
