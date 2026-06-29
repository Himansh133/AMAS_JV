#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>
#include <visa.h>

#pragma comment(lib, "advapi32.lib")

// Registry-based COM port detection for different machines
std::vector<std::string> getAvailableCOMPorts() {
  std::vector<std::string> ports;
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    char valueName[256];
    BYTE data[256];
    DWORD valueNameSize, dataSize, type;
    DWORD index = 0;
    while (true) {
      valueNameSize = sizeof(valueName);
      dataSize = sizeof(data);
      LSTATUS status = RegEnumValueA(hKey, index, valueName, &valueNameSize, NULL, &type, data, &dataSize);
      if (status == ERROR_SUCCESS) {
        if (type == REG_SZ) {
          ports.push_back((char*)data);
        }
        index++;
      } else {
        break;
      }
    }
    RegCloseKey(hKey);
  }
  return ports;
}

// Win32 Serial Helper: Open & Configure Port (disables hardware/software flow control for TAP-3001 compatibility)
HANDLE openPort(const std::string& portName, DWORD baudRate = 19200) {
  std::string formattedPort = "\\\\.\\" + portName;
  
  HANDLE hSerial = CreateFileA(
      formattedPort.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      OPEN_EXISTING,
      0,
      NULL
  );

  if (hSerial == INVALID_HANDLE_VALUE) {
      return INVALID_HANDLE_VALUE;
  }

  DCB dcbSerialParams = {0};
  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

  if (!GetCommState(hSerial, &dcbSerialParams)) {
      CloseHandle(hSerial);
      return INVALID_HANDLE_VALUE;
  }

  dcbSerialParams.BaudRate = baudRate;
  dcbSerialParams.ByteSize = 8;
  dcbSerialParams.StopBits = ONESTOPBIT;
  dcbSerialParams.Parity = NOPARITY;

  // Disable hardware/software flow control (critical for TAP-3001)
  dcbSerialParams.fOutxCtsFlow = FALSE;
  dcbSerialParams.fOutxDsrFlow = FALSE;
  dcbSerialParams.fDsrSensitivity = FALSE;
  dcbSerialParams.fTXContinueOnXoff = FALSE;
  dcbSerialParams.fOutX = FALSE;
  dcbSerialParams.fInX = FALSE;
  dcbSerialParams.fErrorChar = FALSE;
  dcbSerialParams.fNull = FALSE;
  dcbSerialParams.fAbortOnError = FALSE;

  // Enable binary mode for raw Modbus RTU byte stream
  dcbSerialParams.fBinary = TRUE;

  // Enable DTR and RTS lines automatically
  dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
  dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

  if (!SetCommState(hSerial, &dcbSerialParams)) {
      CloseHandle(hSerial);
      return INVALID_HANDLE_VALUE;
  }

  // Configure timeouts (3000ms read constant timeout)
  COMMTIMEOUTS timeouts = {0};
  timeouts.ReadIntervalTimeout = 50;
  timeouts.ReadTotalTimeoutConstant = 3000;
  timeouts.ReadTotalTimeoutMultiplier = 10;
  timeouts.WriteTotalTimeoutConstant = 50;
  timeouts.WriteTotalTimeoutMultiplier = 10;

  if (!SetCommTimeouts(hSerial, &timeouts)) {
      CloseHandle(hSerial);
      return INVALID_HANDLE_VALUE;
  }

  return hSerial;
}

const double PI = 3.14159265358979323846;

// Modbus RTU CRC16 Calculator
uint16_t calculateCRC(const uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// Helper to parse frequency string (e.g., 22GHz, 5GHz) into Hz
double parseFrequency(const std::string &input) {
  if (input.empty())
    return 0.0;

  std::string clean = input;
  clean.erase(std::remove_if(clean.begin(), clean.end(),
                             [](unsigned char c) { return std::isspace(c); }),
              clean.end());

  double val = 0.0;
  size_t idx = 0;
  try {
    val = std::stod(clean, &idx);
  } catch (...) {
    return 0.0;
  }

  std::string unit = clean.substr(idx);
  for (char &c : unit)
    c = std::toupper(c);

  if (unit == "GHZ" || unit == "G" || unit == "g") {
    val *= 1e9;
  } else if (unit == "MHZ" || unit == "M" || unit == "m") {
    val *= 1e6;
  } else if (unit == "KHZ" || unit == "K" || unit == "k") {
    val *= 1e3;
  }
  return val;
}

// Modbus RTU serial read registers
bool modbusReadRegs(HANDLE hSerial, uint8_t slaveId, uint16_t address, uint16_t numRegs, int16_t *outVals) {
  uint8_t frame[8];
  frame[0] = slaveId;
  frame[1] = 0x03; // Read holding registers
  frame[2] = (address >> 8) & 0xFF;
  frame[3] = address & 0xFF;
  frame[4] = (numRegs >> 8) & 0xFF;
  frame[5] = numRegs & 0xFF;

  uint16_t crc = calculateCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;

  PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

  DWORD bytesWritten = 0;
  if (!WriteFile(hSerial, frame, 8, &bytesWritten, NULL) || bytesWritten != 8) {
    return false;
  }

  DWORD expectedBytes = 5 + 2 * numRegs;
  uint8_t response[256] = {0};
  DWORD bytesRead = 0;

  if (!ReadFile(hSerial, response, expectedBytes, &bytesRead, NULL)) {
    return false;
  }

  if (bytesRead != expectedBytes) {
    return false;
  }

  uint16_t respCrc = calculateCRC(response, expectedBytes - 2);
  uint16_t respCrcReceived = response[expectedBytes - 2] | (response[expectedBytes - 1] << 8);
  if (respCrc != respCrcReceived) {
    return false;
  }

  if (response[1] & 0x80) {
    return false;
  }

  for (int i = 0; i < numRegs; i++) {
    outVals[i] = (int16_t)((response[3 + 2 * i] << 8) | response[4 + 2 * i]);
  }

  return true;
}

// Modbus RTU serial write single register (uses Function Code 16 / 0x10 for TAP-3001 compatibility)
bool modbusWriteReg(HANDLE hSerial, uint8_t slaveId, uint16_t address, int16_t val) {
  uint8_t frame[11];
  frame[0] = slaveId;
  frame[1] = 0x10; // Function Code 16 (0x10) - Write Multiple Registers
  frame[2] = (address >> 8) & 0xFF;
  frame[3] = address & 0xFF;
  frame[4] = 0x00; // Number of registers high
  frame[5] = 0x01; // Number of registers low (1 register)
  frame[6] = 0x02; // Byte count (2 bytes)
  frame[7] = (val >> 8) & 0xFF; // Data high
  frame[8] = val & 0xFF;        // Data low

  uint16_t crc = calculateCRC(frame, 9);
  frame[9] = crc & 0xFF;
  frame[10] = (crc >> 8) & 0xFF;

  PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

  DWORD bytesWritten = 0;
  if (!WriteFile(hSerial, frame, 11, &bytesWritten, NULL) || bytesWritten != 11) {
    return false;
  }

  uint8_t response[8] = {0};
  DWORD bytesRead = 0;

  if (!ReadFile(hSerial, response, 8, &bytesRead, NULL)) {
    return false;
  }

  if (bytesRead != 8) {
    return false;
  }

  uint16_t respCrc = calculateCRC(response, 6);
  uint16_t respCrcReceived = response[6] | (response[7] << 8);
  if (respCrc != respCrcReceived) {
    return false;
  }

  if (response[1] & 0x80) {
    return false;
  }

  return true;
}

// Helper to send SCPI command
bool sendSCPI(ViSession instr, const std::string &cmd) {
  std::string formatted = cmd;
  if (formatted.empty() || formatted.back() != '\n') {
    formatted += "\n";
  }
  ViUInt32 retCnt = 0;
  ViStatus status = viWrite(instr, (ViBuf)formatted.c_str(), (ViUInt32)formatted.length(), &retCnt);
  return (status >= VI_SUCCESS);
}

// Helper to query SCPI string response
bool querySCPI(ViSession instr, const std::string &query, std::string &outRes) {
  if (!sendSCPI(instr, query)) {
    return false;
  }
  ViUInt32 bufSize = 131072;
  std::vector<unsigned char> buffer(bufSize, 0);
  ViUInt32 retCnt = 0;
  ViStatus status = viRead(instr, (ViBuf)buffer.data(), bufSize - 1, &retCnt);
  if (status >= VI_SUCCESS) {
    outRes = reinterpret_cast<char *>(buffer.data());
    while (!outRes.empty() && (outRes.back() == '\n' || outRes.back() == '\r' || outRes.back() == ' ')) {
      outRes.pop_back();
    }
    return true;
  }
  return false;
}

// Helper to query SCPI VNA error status
void checkVnaErrors(ViSession instr, const std::string &context) {
  std::string errStr;
  if (querySCPI(instr, ":SYST:ERR?", errStr)) {
    if (errStr.find("+0") == std::string::npos && errStr.find("No error") == std::string::npos) {
      std::cerr << "[VNA WARNING] Error during " << context << ": " << errStr << std::endl;
    }
  }
}

// Sub-output routines
void runStaticS11(ViSession instr, ViSession defaultRM);
void runPositionerTest();
void runCoordinatedMeasurement(ViSession instr, ViSession defaultRM);

int main() {
  std::cout << "AMAS - Integrated RF Measurement & Control Diagnostic Utility" << std::endl;
  std::cout << "==============================================================" << std::endl;
  std::cout << "1. Static VNA S11 Measurement (No Positioner Rotation)" << std::endl;
  std::cout << "2. Standalone Positioner Control Test (Modbus Serial)" << std::endl;
  std::cout << "3. Coordinated Radiation Pattern Measurement (VNA + Positioner)" << std::endl;
  std::cout << "Select function [1-3, default: 1]: ";

  std::string mainChoice;
  std::getline(std::cin, mainChoice);
  if (mainChoice.empty()) mainChoice = "1";

  if (mainChoice == "2") {
    runPositionerTest();
    return 0;
  }

  // Connect VNA for Options 1 and 3
  ViSession defaultRM = VI_NULL;
  ViSession instr = VI_NULL;
  ViStatus status = viOpenDefaultRM(&defaultRM);
  if (status < VI_SUCCESS) {
    std::cerr << "[ERROR] Failed to open Default Resource Manager. Status: 0x" << std::hex << status << std::dec << std::endl;
    return 1;
  }

  const char *resourceString = "TCPIP0::192.168.113.206::inst0::INSTR";
  std::cout << "[INFO] Connecting to FieldFox VNA at: " << resourceString << " ..." << std::endl;
  status = viOpen(defaultRM, (ViRsrc)resourceString, VI_NULL, VI_NULL, &instr);
  if (status < VI_SUCCESS) {
    std::cerr << "[ERROR] Failed to open VNA session. Status: 0x" << std::hex << status << std::dec << std::endl;
    viClose(defaultRM);
    return 1;
  }
  std::cout << "[SUCCESS] VNA Connected successfully!" << std::endl;

  // Set timeout to 10 seconds and clear stale errors
  viSetAttribute(instr, VI_ATTR_TMO_VALUE, 10000);
  sendSCPI(instr, "*CLS");

  // Print VNA Identification info
  std::string idnResponse, scpiVersion;
  if (querySCPI(instr, "*IDN?", idnResponse)) {
    std::cout << "  *IDN?        : " << idnResponse << std::endl;
  }
  if (querySCPI(instr, ":SYST:VERS?", scpiVersion)) {
    std::cout << "  SCPI Version : " << scpiVersion << std::endl;
  }

  // Print VNA Directory content
  std::string currentDir, fileList;
  if (querySCPI(instr, "MMEM:CDIR?", currentDir)) {
    std::cout << "  Current VNA Directory: " << currentDir << std::endl;
  }
  if (querySCPI(instr, "MMEM:CAT?", fileList)) {
    std::cout << "  VNA Directory Files: " << fileList << std::endl;
  }
  checkVnaErrors(instr, "VNA Identification");

  if (mainChoice == "1") {
    runStaticS11(instr, defaultRM);
  } else if (mainChoice == "3") {
    runCoordinatedMeasurement(instr, defaultRM);
  }

  viClose(instr);
  viClose(defaultRM);
  return 0;
}

// ------------------------------------------------------------------------
// SUB-OUTPUT 1: Static VNA S11 Measurement
// ------------------------------------------------------------------------
void runStaticS11(ViSession instr, ViSession defaultRM) {
  std::cout << "\n--- SUB-OUTPUT 1: Static VNA S11 Measurement ---" << std::endl;
  std::cout << "Select S11 Calibration Band:" << std::endl;
  std::cout << "1. 8.0 GHz - 12.0 GHz  | Points: 401 | Cal: calchamber_s11_1_18ghz.sta" << std::endl;
  std::cout << "2. 12.0 GHz - 18.0 GHz | Points: 601 | Cal: calchamber_s11_1_18ghz.sta" << std::endl;
  std::cout << "3. 18.0 GHz - 26.5 GHz | Points: 851 | Cal: calchamber_s11_18_26p5ghz.sta" << std::endl;
  std::cout << "4. 26.5 GHz - 40.0 GHz | Points: 136 | Cal: calchamber_s11_26p5_40ghz.sta" << std::endl;
  std::cout << "5. Custom Range (Manual input)" << std::endl;
  std::cout << "Select option [1-5, default: 3]: ";

  std::string bandChoice;
  std::getline(std::cin, bandChoice);
  if (bandChoice.empty()) bandChoice = "3";

  double startHz = 18.0e9;
  double stopHz = 26.5e9;
  int numPoints = 851;
  std::string calFile = "calchamber_s11_18_26p5ghz.sta";
  std::string bandLabel = "s11_18_26.5ghz";

  if (bandChoice == "1") {
    startHz = 8.0e9;
    stopHz = 12.0e9;
    numPoints = 401;
    calFile = "calchamber_s11_1_18ghz.sta";
    bandLabel = "s11_8_12ghz";
  } else if (bandChoice == "2") {
    startHz = 12.0e9;
    stopHz = 18.0e9;
    numPoints = 601;
    calFile = "calchamber_s11_1_18ghz.sta";
    bandLabel = "s11_12_18ghz";
  } else if (bandChoice == "3") {
    startHz = 18.0e9;
    stopHz = 26.5e9;
    numPoints = 851;
    calFile = "calchamber_s11_18_26p5ghz.sta";
    bandLabel = "s11_18_26.5ghz";
  } else if (bandChoice == "4") {
    startHz = 26.5e9;
    stopHz = 40.0e9;
    numPoints = 136;
    calFile = "calchamber_s11_26p5_40ghz.sta";
    bandLabel = "s11_26.5_40ghz";
  } else {
    bandLabel = "s11_custom";
    calFile = "";
    std::string inputStart, inputStop, inputPoints;
    std::cout << "Enter Start Frequency (e.g. 5GHz, 21.9GHz): ";
    std::getline(std::cin, inputStart);
    startHz = parseFrequency(inputStart);
    
    std::cout << "Enter Stop Frequency (e.g. 16GHz, 22.1GHz): ";
    std::getline(std::cin, inputStop);
    stopHz = parseFrequency(inputStop);
    
    std::cout << "Enter Sweep Points: ";
    std::getline(std::cin, inputPoints);
    numPoints = std::stoi(inputPoints);
    
    std::cout << "Enter custom Calibration State File (leave blank if none): ";
    std::getline(std::cin, calFile);
  }

  std::string inputIFBW, inputPower;
  std::cout << "Enter IF Bandwidth (Hz) [default: 1000]: ";
  std::getline(std::cin, inputIFBW);
  double ifbwHz = inputIFBW.empty() ? 1000.0 : std::stod(inputIFBW);

  std::cout << "Enter Output Power (dBm) [default: -15]: ";
  std::getline(std::cin, inputPower);
  double powerDbm = inputPower.empty() ? -15.0 : std::stod(inputPower);

  // Configure VNA parameters
  std::cout << "\n[INFO] Configuring VNA parameters..." << std::endl;
  sendSCPI(instr, "*CLS");

  if (!calFile.empty()) {
    std::cout << "[VNA] Loading Calibration State File: " << calFile << " ..." << std::endl;
    sendSCPI(instr, "MMEM:LOAD:STAT \"" + calFile + "\"");
    std::string opcRes;
    querySCPI(instr, "*OPC?", opcRes);
    Sleep(1000);
  }

  sendSCPI(instr, "INST:SEL \"NA\"");
  sendSCPI(instr, "CALC1:PAR1:DEF S11");
  sendSCPI(instr, "CALC1:FORM MLOG");
  sendSCPI(instr, "SENS1:FREQ:STAR " + std::to_string(startHz));
  sendSCPI(instr, "SENS1:FREQ:STOP " + std::to_string(stopHz));
  sendSCPI(instr, "SENS1:SWE:POIN " + std::to_string(numPoints));
  sendSCPI(instr, "SENS1:BWID " + std::to_string(ifbwHz));
  sendSCPI(instr, "SOUR1:POW " + std::to_string(powerDbm));
  sendSCPI(instr, "INIT:CONT OFF");
  sendSCPI(instr, "TRIG:SOUR INT"); // Force trigger source to internal/auto
  checkVnaErrors(instr, "VNA Setup");

  std::vector<double> frequencies(numPoints);
  double freqStep = (numPoints > 1) ? (stopHz - startHz) / (numPoints - 1) : 0.0;
  for (int i = 0; i < numPoints; ++i) {
    frequencies[i] = startHz + i * freqStep;
  }

  std::string csvName = "measurement_results_" + bandLabel + ".csv";
  std::ofstream csvFile(csvName);
  csvFile << "Frequency_Hz,Magnitude_dB,Phase_deg\n";

  std::cout << "[VNA] Triggering S11 sweep..." << std::endl;
  sendSCPI(instr, "INIT:IMM");

  std::string opcResponse;
  if (querySCPI(instr, "*OPC?", opcResponse) && opcResponse == "1") {
    std::string sdata;
    if (querySCPI(instr, "CALC1:DATA:SDAT?", sdata)) {
      std::stringstream ss(sdata);
      std::string valStr;
      std::vector<double> complexVals;
      while (std::getline(ss, valStr, ',')) {
        if (!valStr.empty()) complexVals.push_back(std::stod(valStr));
      }

      size_t pointsRead = complexVals.size() / 2;
      if (pointsRead != numPoints) {
        std::cerr << "[ERROR] Points read (" << pointsRead << ") mismatch with configuration (" << numPoints << ")!" << std::endl;
      } else {
        std::cout << "[SUCCESS] Sweep complete. S11 data acquired." << std::endl;
        for (size_t i = 0; i < pointsRead; ++i) {
          double r = complexVals[2 * i];
          double im = complexVals[2 * i + 1];
          double mag = std::sqrt(r * r + im * im);
          double magDb = (mag > 1e-15) ? 20.0 * std::log10(mag) : -150.0;
          double phaseDeg = std::atan2(im, r) * 180.0 / PI;

          csvFile << std::fixed << std::setprecision(0) << frequencies[i] << ","
                  << std::fixed << std::setprecision(4) << magDb << ","
                  << std::fixed << std::setprecision(4) << phaseDeg << "\n";
        }
        
        size_t mid = pointsRead / 2;
        double midR = complexVals[2 * mid];
        double midIm = complexVals[2 * mid + 1];
        double midMag = std::sqrt(midR * midR + midIm * midIm);
        double midMagDb = (midMag > 1e-15) ? 20.0 * std::log10(midMag) : -150.0;
        std::cout << "  S11 at " << (frequencies[mid] / 1e9) << " GHz: " << std::fixed << std::setprecision(2) << midMagDb << " dB" << std::endl;
      }
    } else {
      std::cerr << "[ERROR] Failed to query S-parameter complex data." << std::endl;
      checkVnaErrors(instr, "Query SDATA");
    }
  } else {
    std::cerr << "[ERROR] VNA sweep timeout or OPC? failed." << std::endl;
    checkVnaErrors(instr, "OPC waiting");
  }

  csvFile.close();
  std::cout << "[INFO] Results saved to: " << csvName << std::endl;
}

// ------------------------------------------------------------------------
// SUB-OUTPUT 2: Standalone Positioner Control Test
// ------------------------------------------------------------------------
void runPositionerTest() {
  std::cout << "\n--- SUB-OUTPUT 2: Standalone Positioner Control Test ---" << std::endl;
  std::vector<std::string> detectedPorts = getAvailableCOMPorts();
  std::string portName = "COM3";
  if (detectedPorts.empty()) {
    std::cout << "[WARNING] No serial COM ports detected in registry." << std::endl;
    std::cout << "Enter COM Port name [default: COM3]: ";
    std::string portChoice;
    std::getline(std::cin, portChoice);
    if (!portChoice.empty()) portName = portChoice;
  } else {
    std::cout << "[INFO] Detected COM ports on this machine:" << std::endl;
    for (size_t i = 0; i < detectedPorts.size(); ++i) {
      std::cout << "  " << (i + 1) << ". " << detectedPorts[i] << std::endl;
    }
    std::cout << "Select port number [1-" << detectedPorts.size() << "] or type name directly [default: 1]: ";
    std::string selection;
    std::getline(std::cin, selection);
    if (selection.empty()) {
      portName = detectedPorts[0];
    } else {
      try {
        int idx = std::stoi(selection);
        if (idx >= 1 && idx <= (int)detectedPorts.size()) {
          portName = detectedPorts[idx - 1];
        } else {
          portName = selection;
        }
      } catch (...) {
        portName = selection;
      }
    }
  }

  std::cout << "[INFO] Opening serial port " << portName << " at 19200 Baud (8N1)..." << std::endl;
  HANDLE hSerial = openPort(portName, 19200);
  if (hSerial == INVALID_HANDLE_VALUE) {
    std::cerr << "[ERROR] Failed to open serial port " << portName << ". (Windows Error: " << GetLastError() << ")" << std::endl;
    return;
  }

  std::cout << "[SUCCESS] Positioner Port Opened." << std::endl;

  while (true) {
    std::cout << "\nPositioner Control Menu:" << std::endl;
    std::cout << "1. Read Current Coordinates (Azimuth, Elevation, Polarization)" << std::endl;
    std::cout << "2. Move Azimuth (0 to 360 deg) [Register 2]" << std::endl;
    std::cout << "3. Move Elevation (-90 to 90 deg) [Register 10]" << std::endl;
    std::cout << "4. Move Polarization (0 to 360 deg) [Register 18]" << std::endl;
    std::cout << "5. Home Positioner (Move AZ, EL, PL to 0.0 deg)" << std::endl;
    std::cout << "6. Exit Positioner Control" << std::endl;
    std::cout << "Enter option [1-6]: ";

    std::string menuOpt;
    std::getline(std::cin, menuOpt);
    if (menuOpt == "6" || menuOpt.empty()) break;

    if (menuOpt == "1") {
      int16_t azVal = 0, elVal = 0, plVal = 0;
      bool readOk = true;
      std::cout << "Reading coordinates..." << std::endl;
      if (!modbusReadRegs(hSerial, 1, 3, 1, &azVal)) readOk = false;
      if (!modbusReadRegs(hSerial, 1, 11, 1, &elVal)) readOk = false;
      if (!modbusReadRegs(hSerial, 1, 19, 1, &plVal)) readOk = false;

      if (readOk) {
        std::cout << "  Current Azimuth:      " << (azVal / 10.0f) << " deg" << std::endl;
        std::cout << "  Current Elevation:    " << (elVal / 10.0f) << " deg" << std::endl;
        std::cout << "  Current Polarization: " << (plVal / 10.0f) << " deg" << std::endl;
      } else {
        std::cerr << "[ERROR] Failed to query coordinates via Modbus." << std::endl;
      }
    } else if (menuOpt == "2") {
      std::cout << "Enter Target Azimuth Angle (deg) [0.0 - 360.0]: ";
      std::string angStr;
      std::getline(std::cin, angStr);
      try {
        float ang = std::stof(angStr);
        int16_t regVal = (int16_t)(ang * 10.0f);
        if (modbusWriteReg(hSerial, 1, 2, regVal)) {
          std::cout << "[SUCCESS] Move command sent. Waiting for settle..." << std::endl;
          while (true) {
            int16_t curVal = 0;
            if (modbusReadRegs(hSerial, 1, 3, 1, &curVal)) {
              float curAng = curVal / 10.0f;
              std::cout << "\r  Position: " << std::fixed << std::setprecision(1) << curAng << " deg" << std::flush;
              if (std::fabs(curAng - ang) <= 0.2f) {
                std::cout << " [SETTLED]" << std::endl;
                break;
              }
            }
            Sleep(150);
          }
        } else {
          std::cerr << "[ERROR] Failed to write register." << std::endl;
        }
      } catch (...) {
        std::cerr << "[ERROR] Invalid numeric input." << std::endl;
      }
    } else if (menuOpt == "3") {
      std::cout << "Enter Target Elevation Angle (deg) [-90.0 - 90.0]: ";
      std::string angStr;
      std::getline(std::cin, angStr);
      try {
        float ang = std::stof(angStr);
        int16_t regVal = (int16_t)(ang * 10.0f);
        if (modbusWriteReg(hSerial, 1, 10, regVal)) {
          std::cout << "[SUCCESS] Move command sent. Waiting for settle..." << std::endl;
          while (true) {
            int16_t curVal = 0;
            if (modbusReadRegs(hSerial, 1, 11, 1, &curVal)) {
              float curAng = curVal / 10.0f;
              std::cout << "\r  Position: " << std::fixed << std::setprecision(1) << curAng << " deg" << std::flush;
              if (std::fabs(curAng - ang) <= 0.2f) {
                std::cout << " [SETTLED]" << std::endl;
                break;
              }
            }
            Sleep(150);
          }
        }
      } catch (...) {
        std::cerr << "[ERROR] Invalid numeric input." << std::endl;
      }
    } else if (menuOpt == "4") {
      std::cout << "Enter Target Polarization Angle (deg) [0.0 - 360.0]: ";
      std::string angStr;
      std::getline(std::cin, angStr);
      try {
        float ang = std::stof(angStr);
        int16_t regVal = (int16_t)(ang * 10.0f);
        if (modbusWriteReg(hSerial, 1, 18, regVal)) {
          std::cout << "[SUCCESS] Move command sent. Waiting for settle..." << std::endl;
          while (true) {
            int16_t curVal = 0;
            if (modbusReadRegs(hSerial, 1, 19, 1, &curVal)) {
              float curAng = curVal / 10.0f;
              std::cout << "\r  Position: " << std::fixed << std::setprecision(1) << curAng << " deg" << std::flush;
              if (std::fabs(curAng - ang) <= 0.2f) {
                std::cout << " [SETTLED]" << std::endl;
                break;
              }
            }
            Sleep(150);
          }
        }
      } catch (...) {
        std::cerr << "[ERROR] Invalid numeric input." << std::endl;
      }
    } else if (menuOpt == "5") {
      std::cout << "Homing all axes..." << std::endl;
      modbusWriteReg(hSerial, 1, 2, 0);
      modbusWriteReg(hSerial, 1, 10, 0);
      modbusWriteReg(hSerial, 1, 18, 0);
      std::cout << "[SUCCESS] Homing commands written." << std::endl;
    }
  }

  CloseHandle(hSerial);
  std::cout << "[INFO] Positioner Port Closed." << std::endl;
}

// ------------------------------------------------------------------------
// SUB-OUTPUT 3: Coordinated Radiation Pattern Measurement
// ------------------------------------------------------------------------
void runCoordinatedMeasurement(ViSession instr, ViSession defaultRM) {
  std::cout << "\n--- SUB-OUTPUT 3: Coordinated Radiation Pattern Measurement ---" << std::endl;
  std::cout << "Select Chamber Calibration Band (Gain/S21):" << std::endl;
  std::cout << "1. 8.0 GHz - 12.0 GHz  | Points: 401  | Cal: calchamber8_12ghz.sta" << std::endl;
  std::cout << "2. 12.0 GHz - 18.0 GHz | Points: 601  | Cal: calchamber12_18ghz.sta" << std::endl;
  std::cout << "3. 18.0 GHz - 26.5 GHz | Points: 851  | Cal: calchamber18_26p5ghz.sta" << std::endl;
  std::cout << "4. 26.5 GHz - 40.0 GHz | Points: 136  | Cal: calchamber26p5g_40ghz.sta" << std::endl;
  std::cout << "5. 1.0 GHz - 18.0 GHz  | Points: 1701 | Cal: calchamber1_18ghz.sta" << std::endl;
  std::cout << "6. Custom Range (Manual input)" << std::endl;
  std::cout << "Select option [1-6, default: 3]: ";

  std::string bandChoice;
  std::getline(std::cin, bandChoice);
  if (bandChoice.empty()) bandChoice = "3";

  double startHz = 18.0e9;
  double stopHz = 26.5e9;
  int numPoints = 851;
  std::string calFile = "calchamber18_26p5ghz.sta";
  std::string bandLabel = "rad_18_26.5ghz";

  if (bandChoice == "1") {
    startHz = 8.0e9;
    stopHz = 12.0e9;
    numPoints = 401;
    calFile = "calchamber8_12ghz.sta";
    bandLabel = "rad_8_12ghz";
  } else if (bandChoice == "2") {
    startHz = 12.0e9;
    stopHz = 18.0e9;
    numPoints = 601;
    calFile = "calchamber12_18ghz.sta";
    bandLabel = "rad_12_18ghz";
  } else if (bandChoice == "3") {
    startHz = 18.0e9;
    stopHz = 26.5e9;
    numPoints = 851;
    calFile = "calchamber18_26p5ghz.sta";
    bandLabel = "rad_18_26.5ghz";
  } else if (bandChoice == "4") {
    startHz = 26.5e9;
    stopHz = 40.0e9;
    numPoints = 136;
    calFile = "calchamber26p5g_40ghz.sta";
    bandLabel = "rad_26.5_40ghz";
  } else if (bandChoice == "5") {
    startHz = 1.0e9;
    stopHz = 18.0e9;
    numPoints = 1701;
    calFile = "calchamber1_18ghz.sta";
    bandLabel = "rad_1_18ghz";
  } else {
    bandLabel = "rad_custom";
    calFile = "";
    std::string inputStart, inputStop, inputPoints;
    std::cout << "Enter Start Frequency: ";
    std::getline(std::cin, inputStart);
    startHz = parseFrequency(inputStart);
    std::cout << "Enter Stop Frequency: ";
    std::getline(std::cin, inputStop);
    stopHz = parseFrequency(inputStop);
    std::cout << "Enter Sweep Points: ";
    std::getline(std::cin, inputPoints);
    numPoints = std::stoi(inputPoints);
    std::cout << "Enter Calibration State File: ";
    std::getline(std::cin, calFile);
  }

  std::cout << "Select S-parameter(s) to measure:" << std::endl;
  std::cout << "1. S21 (Gain / Transmission) [default]" << std::endl;
  std::cout << "2. S11 (Reflection / Return Loss)" << std::endl;
  std::cout << "3. Both S11 and S21 concurrently" << std::endl;
  std::cout << "Select option [1-3, default: 1]: ";
  std::string paramOpt;
  std::getline(std::cin, paramOpt);
  bool measureS11 = false;
  bool measureS21 = false;
  std::string paramLabel = "S21";
  if (paramOpt == "2") {
    measureS11 = true;
    paramLabel = "S11";
  } else if (paramOpt == "3") {
    measureS11 = true;
    measureS21 = true;
    paramLabel = "S11_S21";
  } else {
    measureS21 = true;
  }

  std::string inputIFBW, inputPower;
  std::cout << "Enter IF Bandwidth (Hz) [default: 1000]: ";
  std::getline(std::cin, inputIFBW);
  double ifbwHz = inputIFBW.empty() ? 1000.0 : std::stod(inputIFBW);

  std::cout << "Enter Output Power (dBm) [default: -15]: ";
  std::getline(std::cin, inputPower);
  double powerDbm = inputPower.empty() ? -15.0 : std::stod(inputPower);

  std::cout << "\n--- Configure Positioner Sweep Parameters ---" << std::endl;
  std::string inpStartAng, inpStopAng, inpStepAng;
  std::cout << "Enter Start Angle (deg) [default: 0]: ";
  std::getline(std::cin, inpStartAng);
  float startAng = inpStartAng.empty() ? 0.0f : std::stof(inpStartAng);

  std::cout << "Enter Stop Angle (deg) [default: 90]: ";
  std::getline(std::cin, inpStopAng);
  float stopAng = inpStopAng.empty() ? 90.0f : std::stof(inpStopAng);

  std::cout << "Enter Step Angle (deg) [default: 10]: ";
  std::getline(std::cin, inpStepAng);
  float stepAng = inpStepAng.empty() ? 10.0f : std::stof(inpStepAng);

  // Connect Positioner
  HANDLE hSerial = INVALID_HANDLE_VALUE;
  bool usePositioner = false;

  std::vector<std::string> detectedPorts = getAvailableCOMPorts();
  std::string portName = "COM3";
  if (!detectedPorts.empty()) {
    portName = detectedPorts[0];
  }

  std::cout << "\nConnect to physical Positioner on " << portName << "? (y/n) [default: y]: ";
  std::string posChoice;
  std::getline(std::cin, posChoice);
  if (posChoice.empty() || posChoice[0] == 'y' || posChoice[0] == 'Y') {
    std::cout << "[INFO] Opening serial port " << portName << "..." << std::endl;
    hSerial = openPort(portName, 19200);
    if (hSerial != INVALID_HANDLE_VALUE) {
      int16_t curAz = 0;
      if (modbusReadRegs(hSerial, 1, 3, 1, &curAz)) {
        std::cout << "[SUCCESS] Positioner Connected! Current Angle: " << (curAz / 10.0f) << " deg" << std::endl;
        usePositioner = true;
      } else {
        std::cerr << "[ERROR] Modbus read failed." << std::endl;
        CloseHandle(hSerial);
      }
    } else {
      std::cerr << "[ERROR] Open serial failed." << std::endl;
    }
  }

  if (!usePositioner) {
    std::cout << "[INFO] Running sweep with SIMULATED Positioner." << std::endl;
  }

  // Configure VNA parameters
  std::cout << "\n[INFO] Configuring VNA parameters..." << std::endl;
  sendSCPI(instr, "*CLS");

  if (!calFile.empty()) {
    std::cout << "[VNA] Loading Calibration State File: " << calFile << " ..." << std::endl;
    sendSCPI(instr, "MMEM:LOAD:STAT \"" + calFile + "\"");
    std::string opcRes;
    querySCPI(instr, "*OPC?", opcRes);
    Sleep(1000);
  }

  sendSCPI(instr, "INST:SEL \"NA\"");
  int traceCount = (measureS11 && measureS21) ? 2 : 1;
  sendSCPI(instr, "CALC1:PAR:COUN " + std::to_string(traceCount));

  if (measureS11 && measureS21) {
    sendSCPI(instr, "CALC1:PAR1:DEF S11");
    sendSCPI(instr, "CALC1:PAR2:DEF S21");
  } else if (measureS11) {
    sendSCPI(instr, "CALC1:PAR1:DEF S11");
  } else {
    sendSCPI(instr, "CALC1:PAR1:DEF S21");
  }

  sendSCPI(instr, "CALC1:FORM MLOG");
  sendSCPI(instr, "SENS1:FREQ:STAR " + std::to_string(startHz));
  sendSCPI(instr, "SENS1:FREQ:STOP " + std::to_string(stopHz));
  sendSCPI(instr, "SENS1:SWE:POIN " + std::to_string(numPoints));
  sendSCPI(instr, "SENS1:BWID " + std::to_string(ifbwHz));
  sendSCPI(instr, "SOUR1:POW " + std::to_string(powerDbm));
  sendSCPI(instr, "INIT:CONT OFF");
  sendSCPI(instr, "TRIG:SOUR INT"); // Set trigger source to internal/auto
  checkVnaErrors(instr, "VNA Setup");

  std::vector<double> frequencies(numPoints);
  double freqStep = (numPoints > 1) ? (stopHz - startHz) / (numPoints - 1) : 0.0;
  for (int i = 0; i < numPoints; ++i) {
    frequencies[i] = startHz + i * freqStep;
  }

  std::string csvNameS11 = "measurement_results_S11_" + bandLabel + ".csv";
  std::string csvNameS21 = "measurement_results_S21_" + bandLabel + ".csv";
  std::ofstream csvFileS11;
  std::ofstream csvFileS21;

  if (measureS11) {
    csvFileS11.open(csvNameS11);
    csvFileS11 << "Angle_deg,Frequency_Hz,Magnitude_dB,Phase_deg\n";
  }
  if (measureS21) {
    csvFileS21.open(csvNameS21);
    csvFileS21 << "Angle_deg,Frequency_Hz,Magnitude_dB,Phase_deg\n";
  }

  std::cout << "\n--- Starting Coordinated Sweep ---" << std::endl;
  float currentAngle = startAng;
  while (currentAngle <= stopAng) {
    std::cout << "\n[POS] Targeting Angle: " << currentAngle << " deg" << std::endl;
    
    if (usePositioner) {
      int16_t writeVal = (int16_t)(currentAngle * 10.0f);
      if (modbusWriteReg(hSerial, 1, 2, writeVal)) {
        float tolerance = 0.2f;
        int maxAttempts = 100;
        int attempts = 0;
        bool settled = false;
        while (attempts < maxAttempts) {
          int16_t curVal = 0;
          if (modbusReadRegs(hSerial, 1, 3, 1, &curVal)) {
            float curAng = curVal / 10.0f;
            std::cout << "\r  Moving... Current Angle: " << std::fixed << std::setprecision(1) << curAng << " deg" << std::flush;
            if (std::fabs(curAng - currentAngle) <= tolerance) {
              settled = true;
              std::cout << " [SETTLED]" << std::endl;
              break;
            }
          }
          Sleep(150);
          attempts++;
        }
        if (!settled) {
          std::cerr << "\n[WARNING] Positioner did not settle." << std::endl;
        }
      }
    } else {
      std::cout << "  (Simulated move complete)" << std::endl;
      Sleep(500);
    }

    std::cout << "[VNA] Triggering sweep..." << std::endl;
    sendSCPI(instr, "INIT:IMM");
    
    std::string opcResponse;
    if (querySCPI(instr, "*OPC?", opcResponse) && opcResponse == "1") {
      if (measureS11) {
        sendSCPI(instr, "CALC1:PAR1:SEL");
        std::string sdata;
        if (querySCPI(instr, "CALC1:DATA:SDAT?", sdata)) {
          std::stringstream ss(sdata);
          std::string valStr;
          std::vector<double> complexVals;
          while (std::getline(ss, valStr, ',')) {
            if (!valStr.empty()) complexVals.push_back(std::stod(valStr));
          }
          size_t pointsRead = complexVals.size() / 2;
          if (pointsRead == numPoints) {
            for (size_t i = 0; i < pointsRead; ++i) {
              double r = complexVals[2 * i];
              double im = complexVals[2 * i + 1];
              double mag = std::sqrt(r * r + im * im);
              double magDb = (mag > 1e-15) ? 20.0 * std::log10(mag) : -150.0;
              double phaseDeg = std::atan2(im, r) * 180.0 / PI;

              csvFileS11 << std::fixed << std::setprecision(1) << currentAngle << ","
                         << std::fixed << std::setprecision(0) << frequencies[i] << ","
                         << std::fixed << std::setprecision(4) << magDb << ","
                         << std::fixed << std::setprecision(4) << phaseDeg << "\n";
            }
          } else {
            std::cerr << "[ERROR] S11 points read mismatch: " << pointsRead << " vs " << numPoints << std::endl;
          }
        } else {
          std::cerr << "[ERROR] S11 trace read failed." << std::endl;
          checkVnaErrors(instr, "S11 Read");
        }
      }

      if (measureS21) {
        std::string traceNum = (measureS11 && measureS21) ? "2" : "1";
        sendSCPI(instr, "CALC1:PAR" + traceNum + ":SEL");
        std::string sdata;
        if (querySCPI(instr, "CALC1:DATA:SDAT?", sdata)) {
          std::stringstream ss(sdata);
          std::string valStr;
          std::vector<double> complexVals;
          while (std::getline(ss, valStr, ',')) {
            if (!valStr.empty()) complexVals.push_back(std::stod(valStr));
          }
          size_t pointsRead = complexVals.size() / 2;
          if (pointsRead == numPoints) {
            for (size_t i = 0; i < pointsRead; ++i) {
              double r = complexVals[2 * i];
              double im = complexVals[2 * i + 1];
              double mag = std::sqrt(r * r + im * im);
              double magDb = (mag > 1e-15) ? 20.0 * std::log10(mag) : -150.0;
              double phaseDeg = std::atan2(im, r) * 180.0 / PI;

              csvFileS21 << std::fixed << std::setprecision(1) << currentAngle << ","
                         << std::fixed << std::setprecision(0) << frequencies[i] << ","
                         << std::fixed << std::setprecision(4) << magDb << ","
                         << std::fixed << std::setprecision(4) << phaseDeg << "\n";
            }
          } else {
            std::cerr << "[ERROR] S21 points read mismatch: " << pointsRead << " vs " << numPoints << std::endl;
          }
        } else {
          std::cerr << "[ERROR] S21 trace read failed." << std::endl;
          checkVnaErrors(instr, "S21 Read");
        }
      }
      std::cout << "[SUCCESS] Sweep complete." << std::endl;
    } else {
      std::cerr << "[ERROR] Sweep timeout." << std::endl;
      checkVnaErrors(instr, "OPC waiting");
    }

    currentAngle += stepAng;
  }

  std::cout << "\nHoming turntable..." << std::endl;
  if (usePositioner) {
    modbusWriteReg(hSerial, 1, 2, 0);
  }

  if (measureS11) {
    csvFileS11.close();
    std::cout << "[INFO] S11 Results saved to: " << csvNameS11 << std::endl;
  }
  if (measureS21) {
    csvFileS21.close();
    std::cout << "[INFO] S21 Results saved to: " << csvNameS21 << std::endl;
  }
  std::cout << "\n[SUCCESS] Coordinated sweep finished!" << std::endl;

  if (hSerial != INVALID_HANDLE_VALUE) {
    CloseHandle(hSerial);
  }
}

