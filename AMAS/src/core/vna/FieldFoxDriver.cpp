#include "FieldFoxDriver.h"
#include "Logger.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace AMAS {

FieldFoxDriver::FieldFoxDriver()
    : m_isConnected(false) {
}

FieldFoxDriver::~FieldFoxDriver() {
    disconnect();
}

bool FieldFoxDriver::connect(const std::string& resource) {
    disconnect();
    
    if (m_session.connect(resource, 5000)) {
        // Query identification string to verify connection
        if (m_session.query("*IDN?", m_idnString)) {
            Log::info("FieldFox VNA connected successfully. IDN: " + m_idnString);
            m_isConnected = true;
            return true;
        } else {
            Log::error("Failed to query IDN string from FieldFox VNA.");
            m_session.disconnect();
        }
    }
    return false;
}

void FieldFoxDriver::disconnect() {
    if (m_isConnected) {
        m_session.disconnect();
        m_isConnected = false;
        m_idnString.clear();
        Log::info("FieldFox VNA disconnected.");
    }
}

bool FieldFoxDriver::loadCalibrationState(const std::string& filename) {
    if (!m_isConnected) {
        Log::error("FieldFoxDriver: Cannot load state, not connected.");
        return false;
    }
    if (filename.empty()) {
        return true;
    }

    Log::info("FieldFoxDriver: Loading calibration state file: " + filename);
    if (!m_session.write("MMEM:LOAD:STAT \"" + filename + "\"")) {
        Log::error("FieldFoxDriver: Failed to write state load command.");
        return false;
    }

    // Wait for the state file to be fully loaded
    m_session.setTimeout(10000); // 10 seconds timeout for large state files
    std::string response;
    bool opcOk = m_session.query("*OPC?", response);
    m_session.setTimeout(5000); // Restore default 5 seconds timeout

    if (!opcOk || response != "1") {
        Log::error("FieldFoxDriver: Calibration state load timed out or failed.");
        return false;
    }

    // Phase 1 diagnostics: query and log trace settings and errors
    std::string catResponse, formResponse, errResponse;
    m_session.query("CALC:PAR:CAT?", catResponse);
    m_session.query("CALC:FORM?", formResponse);
    m_session.query("SYST:ERR?", errResponse);

    Log::info("[Diagnostics] Calibration loaded. Trace catalog: " + catResponse);
    Log::info("[Diagnostics] Active trace format: " + formResponse);
    Log::info("[Diagnostics] Instrument Error queue: " + errResponse);

    return true;
}

bool FieldFoxDriver::configure(const SweepConfig& config) {
    if (!m_isConnected) {
        Log::error("FieldFoxDriver: Cannot configure VNA, not connected.");
        return false;
    }

    if (!config.validate()) {
        Log::error("FieldFoxDriver: Configuration validation failed.");
        return false;
    }

    Log::info("FieldFoxDriver: Configuring VNA sweep parameters...");

    // 1. Clear status
    if (!m_session.write("*CLS")) return false;

    // 2. Set single sweep mode
    if (!m_session.write("INIT:CONT OFF")) return false;

    // 3. Set trigger source to Internal (since BUS is unsupported/fails in VNA mode)
    if (!m_session.write("TRIG:SOUR INT")) return false;

    // 4. Set frequency parameters
    if (!m_session.write("SENS1:FREQ:STAR " + std::to_string(config.startFreq_Hz))) return false;
    if (!m_session.write("SENS1:FREQ:STOP " + std::to_string(config.stopFreq_Hz))) return false;

    // 5. Set number of sweep points
    if (!m_session.write("SENS1:SWE:POIN " + std::to_string(config.numPoints))) return false;

    // 6. Set IF Bandwidth
    if (!m_session.write("SENS1:BWID " + std::to_string(config.ifbw_Hz))) return false;

    // 7. Set source power level
    if (!m_session.write("SOUR1:POW " + std::to_string(config.power_dBm))) return false;

    // 8. Configure measurement traces (S-parameters)
    // FieldFox allows configuring trace count and assigning S-parameters to each
    size_t numTraces = config.sparams.size();
    if (numTraces > 4) {
        Log::error("FieldFoxDriver: Maximum 4 traces supported. Requested: " + std::to_string(numTraces));
        return false;
    }

    if (!m_session.write("CALC1:PAR:COUN " + std::to_string(numTraces))) return false;

    for (size_t i = 0; i < numTraces; ++i) {
        std::string sparamStr;
        switch (config.sparams[i]) {
            case SParamType::S11: sparamStr = "S11"; break;
            case SParamType::S21: sparamStr = "S21"; break;
            case SParamType::S12: sparamStr = "S12"; break;
            case SParamType::S22: sparamStr = "S22"; break;
            default:
                Log::error("FieldFoxDriver: Unknown S-parameter type.");
                return false;
        }

        std::string traceNum = std::to_string(i + 1);
        if (!m_session.write("CALC1:PAR" + traceNum + ":DEF " + sparamStr)) {
            Log::error("FieldFoxDriver: Failed to define trace " + traceNum + " as " + sparamStr);
            return false;
        }
    }

    m_currentConfig = config;
    Log::info("FieldFoxDriver: VNA successfully configured.");
    return true;
}

bool FieldFoxDriver::triggerSweep() {
    if (!m_isConnected) {
        Log::error("FieldFoxDriver: Cannot trigger sweep, not connected.");
        return false;
    }

    Log::info("FieldFoxDriver: Triggering single sweep...");

    // Send single sweep trigger command
    if (!m_session.write("INIT:IMM")) {
        return false;
    }

    // Set a long timeout (e.g. 30s) during *OPC? query to wait for sweep completion
    m_session.setTimeout(30000);
    
    std::string response;
    bool sweepOk = m_session.query("*OPC?", response);
    
    // Restore default timeout (5s)
    m_session.setTimeout(5000);

    if (!sweepOk || response != "1") {
        Log::error("FieldFoxDriver: Sweep did not complete successfully. OPC response: " + response);
        return false;
    }

    Log::info("FieldFoxDriver: Sweep completed.");
    return true;
}

std::vector<SParamData> FieldFoxDriver::readSParameters() {
    std::vector<SParamData> results;

    if (!m_isConnected) {
        Log::error("FieldFoxDriver: Cannot read S-parameters, not connected.");
        return results;
    }

    size_t numTraces = m_currentConfig.sparams.size();
    results.reserve(numTraces);

    // Generate frequencies vector
    std::vector<double> freq_vector(m_currentConfig.numPoints);
    double step = 0.0;
    if (m_currentConfig.numPoints > 1) {
        step = (m_currentConfig.stopFreq_Hz - m_currentConfig.startFreq_Hz) / (m_currentConfig.numPoints - 1);
    }
    for (int i = 0; i < m_currentConfig.numPoints; ++i) {
        freq_vector[i] = m_currentConfig.startFreq_Hz + i * step;
    }

    for (size_t i = 0; i < numTraces; ++i) {
        std::string traceNum = std::to_string(i + 1);

        // Select the active trace
        if (!m_session.write("CALC1:PAR" + traceNum + ":SEL")) {
            Log::error("FieldFoxDriver: Failed to select trace " + traceNum);
            return {};
        }

        // Query complex S-parameter data (Real + Imaginary pairs)
        std::string rawData;
        if (!m_session.query("CALC1:DATA:SDAT?", rawData, 65536)) {
            Log::error("FieldFoxDriver: Failed to read complex data for trace " + traceNum);
            return {};
        }

        SParamData traceData;
        traceData.type = m_currentConfig.sparams[i];
        traceData.frequencies_Hz = freq_vector;

        if (!parseSDataString(rawData, traceData.magnitudes_dB, traceData.phases_deg)) {
            Log::error("FieldFoxDriver: Failed to parse trace " + traceNum + " data.");
            return {};
        }

        // Verify that parsed points count matches configuration
        if (static_cast<int>(traceData.magnitudes_dB.size()) != m_currentConfig.numPoints) {
            Log::error("FieldFoxDriver: Parsed points (" + std::to_string(traceData.magnitudes_dB.size()) + 
                       ") does not match configured points (" + std::to_string(m_currentConfig.numPoints) + ").");
            return {};
        }

        results.push_back(traceData);
    }

    return results;
}

bool FieldFoxDriver::isConnected() const {
    return m_isConnected && m_session.isConnected();
}

std::string FieldFoxDriver::getIdnString() {
    return m_idnString;
}

bool FieldFoxDriver::parseSDataString(const std::string& rawData, 
                                     std::vector<double>& outMagDb, 
                                     std::vector<double>& outPhaseDeg) const {
    std::stringstream ss(rawData);
    std::string valStr;
    std::vector<double> rawValues;
    
    // Split by comma
    while (std::getline(ss, valStr, ',')) {
        if (valStr.empty()) continue;
        try {
            rawValues.push_back(std::stod(valStr));
        } catch (...) {
            Log::error("FieldFoxDriver: Double parsing failed for value: '" + valStr + "'");
            return false;
        }
    }

    if (rawValues.size() % 2 != 0) {
        Log::error("FieldFoxDriver: Complex data must contain even number of values. Found: " + 
                   std::to_string(rawValues.size()));
        return false;
    }

    size_t numPoints = rawValues.size() / 2;
    outMagDb.clear();
    outPhaseDeg.clear();
    outMagDb.reserve(numPoints);
    outPhaseDeg.reserve(numPoints);

    const double PI = 3.14159265358979323846;

    for (size_t i = 0; i < numPoints; ++i) {
        double real = rawValues[2 * i];
        double imag = rawValues[2 * i + 1];

        double mag = std::sqrt(real * real + imag * imag);
        double magDb = -150.0;
        if (mag > 1e-15) {
            magDb = 20.0 * std::log10(mag);
        }
        
        double phaseDeg = std::atan2(imag, real) * 180.0 / PI;

        outMagDb.push_back(magDb);
        outPhaseDeg.push_back(phaseDeg);
    }

    return true;
}

} // namespace AMAS
