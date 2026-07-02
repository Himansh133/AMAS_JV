#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include "VISASession.h"
#include "FieldFoxDriver.h"
#include "Logger.h"

using namespace AMAS;

int main() {
    Log::init();
    Log::info("AMAS VNA Connection Test Utility");
    Log::info("=================================");

    const std::string resource = "TCPIP0::192.168.113.206::inst0::INSTR";
    Log::info("Using target VNA resource string: " + resource);

    // 1. Raw SCPI commands verification using VISASession directly
    VISASession rawSession;
    Log::info("Verifying raw SCPI communications...");
    if (rawSession.connect(resource)) {
        std::string idn;
        if (rawSession.query("*IDN?", idn)) {
            Log::info("[RAW SCPI] *IDN? result: " + idn);
        } else {
            Log::error("[RAW SCPI] Failed to query *IDN?");
        }

        std::string version;
        if (rawSession.query(":SYST:VERS?", version)) {
            Log::info("[RAW SCPI] :SYST:VERS? result: " + version);
        } else {
            Log::error("[RAW SCPI] Failed to query :SYST:VERS?");
        }

        std::string error;
        if (rawSession.query(":SYST:ERR?", error)) {
            Log::info("[RAW SCPI] :SYST:ERR? result: " + error);
        } else {
            Log::error("[RAW SCPI] Failed to query :SYST:ERR?");
        }

        std::string centerFreq;
        if (rawSession.query(":SENS:FREQ:CENT?", centerFreq)) {
            Log::info("[RAW SCPI] :SENS:FREQ:CENT? result: " + centerFreq);
        } else {
            Log::error("[RAW SCPI] Failed to query :SENS:FREQ:CENT?");
        }

        rawSession.disconnect();
    } else {
        Log::error("Failed to establish raw VISA session with VNA.");
        return 1;
    }

    Log::info("---------------------------------");
    Log::info("Verifying FieldFoxDriver integration...");

    // 2. High-level Driver integration verification
    FieldFoxDriver driver;
    if (!driver.connect(resource)) {
        Log::error("FieldFoxDriver failed to connect to VNA.");
        return 1;
    }

    Log::info("FieldFoxDriver connected successfully. IDN: " + driver.getIdnString());

    // Configure a test sweep
    SweepConfig config;
    config.startFreq_Hz = 21.0e9; // 21 GHz
    config.stopFreq_Hz = 23.0e9;  // 23 GHz
    config.numPoints = 101;       // 101 points
    config.power_dBm = -15.0;     // -15 dBm
    config.ifbw_Hz = 1000.0;      // 1 kHz IF bandwidth
    config.sparams = { SParamType::S11, SParamType::S21 };

    if (!driver.configure(config)) {
        Log::error("FieldFoxDriver configuration failed.");
        driver.disconnect();
        return 1;
    }

    if (!driver.triggerSweep()) {
        Log::error("FieldFoxDriver failed to trigger sweep.");
        driver.disconnect();
        return 1;
    }

    Log::info("Sweep completed. Reading S-parameters...");
    std::vector<SParamData> results = driver.readSParameters();
    if (results.empty()) {
        Log::error("No S-parameters returned from driver.");
        driver.disconnect();
        return 1;
    }

    for (const auto& trace : results) {
        std::string traceType;
        switch (trace.type) {
            case SParamType::S11: traceType = "S11"; break;
            case SParamType::S21: traceType = "S21"; break;
            case SParamType::S12: traceType = "S12"; break;
            case SParamType::S22: traceType = "S22"; break;
            default: traceType = "Unknown"; break;
        }

        Log::info("Trace " + traceType + " parsed successfully:");
        Log::info("  Frequencies count: " + std::to_string(trace.frequencies_Hz.size()));
        Log::info("  Magnitudes count  : " + std::to_string(trace.magnitudes_dB.size()));
        Log::info("  Phases count      : " + std::to_string(trace.phases_deg.size()));

        if (!trace.frequencies_Hz.empty()) {
            // Print start, mid, end points
            size_t midIdx = trace.frequencies_Hz.size() / 2;
            std::stringstream ssStart, ssMid, ssEnd;
            ssStart << std::fixed << std::setprecision(4) 
                    << "    Start: " << (trace.frequencies_Hz.front() / 1e9) << " GHz | " 
                    << trace.magnitudes_dB.front() << " dB | " << trace.phases_deg.front() << " deg";
            ssMid << std::fixed << std::setprecision(4) 
                  << "    Mid:   " << (trace.frequencies_Hz[midIdx] / 1e9) << " GHz | " 
                  << trace.magnitudes_dB[midIdx] << " dB | " << trace.phases_deg[midIdx] << " deg";
            ssEnd << std::fixed << std::setprecision(4) 
                  << "    End:   " << (trace.frequencies_Hz.back() / 1e9) << " GHz | " 
                  << trace.magnitudes_dB.back() << " dB | " << trace.phases_deg.back() << " deg";
            Log::info(ssStart.str());
            Log::info(ssMid.str());
            Log::info(ssEnd.str());
        }
    }

    driver.disconnect();
    Log::info("VNA Connection Test completed successfully!");
    return 0;
}
