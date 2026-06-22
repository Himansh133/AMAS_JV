#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cmath>

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

// Modbus RTU CRC16 Calculator
uint16_t calculateCRC(const uint8_t* buf, int len) {
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

// Win32 Serial Helper: Open & Configure Port
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
        DWORD err = GetLastError();
        std::cerr << "[ERROR] Failed to open port " << portName 
                  << ". Windows Error code: " << err << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    // Configure Port Settings (DCB)
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "[ERROR] GetCommState failed." << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    // Disable all hardware/software flow control
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

    // Enable DTR and RTS lines automatically (matching pymodbus/pyserial settings)
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "[ERROR] SetCommState failed." << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    // Configure Timeouts (non-blocking reads with 3000ms total timeout to match Python)
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 3000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        std::cerr << "[ERROR] SetCommTimeouts failed." << std::endl;
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    return hSerial;
}

// Modbus Function Code 03: Read Holding Registers
bool readRegisters(HANDLE hSerial, uint8_t slaveId, uint16_t address, uint16_t numRegisters, int16_t* outValues) {
    uint8_t frame[8];
    frame[0] = slaveId;
    frame[1] = 0x03;
    frame[2] = (address >> 8) & 0xFF;
    frame[3] = address & 0xFF;
    frame[4] = (numRegisters >> 8) & 0xFF;
    frame[5] = numRegisters & 0xFF;

    uint16_t crc = calculateCRC(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, frame, 8, &bytesWritten, NULL) || bytesWritten != 8) {
        std::cerr << "[ERROR] Modbus Read Request: Failed to write to serial port." << std::endl;
        return false;
    }

    // Expected response size: 1 (slaveId) + 1 (fn) + 1 (byteCount) + 2*numRegisters + 2 (crc)
    DWORD expectedBytes = 5 + 2 * numRegisters;
    uint8_t response[256] = {0};
    DWORD bytesRead = 0;

    if (!ReadFile(hSerial, response, expectedBytes, &bytesRead, NULL)) {
        std::cerr << "[ERROR] Modbus Read Response: ReadFile failed." << std::endl;
        return false;
    }

    if (bytesRead != expectedBytes) {
        std::cerr << "[ERROR] Modbus Read Response: Read timeout. Expected " 
                  << expectedBytes << " bytes, read " << bytesRead << "." << std::endl;
        return false;
    }

    // Verify CRC
    uint16_t respCrc = calculateCRC(response, expectedBytes - 2);
    uint16_t respCrcReceived = response[expectedBytes - 2] | (response[expectedBytes - 1] << 8);
    if (respCrc != respCrcReceived) {
        std::cerr << "[ERROR] Modbus Read Response: CRC mismatch." << std::endl;
        return false;
    }

    // Check for Modbus Exceptions
    if (response[1] & 0x80) {
        std::cerr << "[ERROR] Modbus Exception received. Exception code: " 
                  << (int)response[2] << std::endl;
        return false;
    }

    // Extract raw registers
    for (int i = 0; i < numRegisters; i++) {
        outValues[i] = (int16_t)((response[3 + 2 * i] << 8) | response[4 + 2 * i]);
    }

    return true;
}

// Modbus Function Code 16 (0x10): Write Multiple Registers
bool writeRegister(HANDLE hSerial, uint8_t slaveId, uint16_t address, int16_t value) {
    uint8_t frame[11];
    frame[0] = slaveId;
    frame[1] = 0x10; // Function Code 16 (0x10)
    frame[2] = (address >> 8) & 0xFF;
    frame[3] = address & 0xFF;
    frame[4] = 0x00; // Number of registers high
    frame[5] = 0x01; // Number of registers low (1 register)
    frame[6] = 0x02; // Byte count (2 bytes)
    frame[7] = (value >> 8) & 0xFF; // Data high
    frame[8] = value & 0xFF;        // Data low

    uint16_t crc = calculateCRC(frame, 9);
    frame[9] = crc & 0xFF;
    frame[10] = (crc >> 8) & 0xFF;

    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, frame, 11, &bytesWritten, NULL) || bytesWritten != 11) {
        std::cerr << "[ERROR] Modbus Write Request: Failed to write to serial port." << std::endl;
        return false;
    }

    uint8_t response[8] = {0};
    DWORD bytesRead = 0;

    if (!ReadFile(hSerial, response, 8, &bytesRead, NULL)) {
        std::cerr << "[ERROR] Modbus Write Response: ReadFile failed." << std::endl;
        return false;
    }

    if (bytesRead != 8) {
        std::cerr << "[ERROR] Modbus Write Response: Read timeout. Expected 8 bytes, read " 
                  << bytesRead << "." << std::endl;
        return false;
    }

    // Verify CRC
    uint16_t respCrc = calculateCRC(response, 6);
    uint16_t respCrcReceived = response[6] | (response[7] << 8);
    if (respCrc != respCrcReceived) {
        std::cerr << "[ERROR] Modbus Write Response: CRC mismatch." << std::endl;
        return false;
    }

    // Check for exceptions
    if (response[1] & 0x80) {
        std::cerr << "[ERROR] Modbus Exception received. Exception code: " 
                  << (int)response[2] << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "      TAP-3001 Positioner Control Tool     " << std::endl;
    std::cout << "===========================================" << std::endl;

    // Detect and list active COM ports
    std::vector<std::string> ports = getAvailableCOMPorts();
    std::string portName = "COM3";
    if (ports.empty()) {
        std::cout << "[WARNING] No serial COM ports detected in registry." << std::endl;
        std::cout << "Enter COM Port Name (e.g. COM3) [default: COM3]: ";
        std::string inputPort;
        std::getline(std::cin, inputPort);
        if (!inputPort.empty()) {
            portName = inputPort;
        }
    } else {
        std::cout << "[INFO] Detected COM ports on this machine:" << std::endl;
        for (size_t i = 0; i < ports.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << ports[i] << std::endl;
        }
        std::cout << "Select port number [1-" << ports.size() << "] or type name directly [default: 1]: ";
        std::string selection;
        std::getline(std::cin, selection);
        if (selection.empty()) {
            portName = ports[0];
        } else {
            try {
                int idx = std::stoi(selection);
                if (idx >= 1 && idx <= (int)ports.size()) {
                    portName = ports[idx - 1];
                } else {
                    portName = selection;
                }
            } catch (...) {
                portName = selection;
            }
        }
    }

    uint8_t slaveId = 1;
    std::cout << "Enter Modbus Slave ID [default: 1]: ";
    std::string inputSlave;
    std::getline(std::cin, inputSlave);
    if (!inputSlave.empty()) {
        try {
            slaveId = (uint8_t)std::stoi(inputSlave);
        } catch(...) {
            std::cout << "Invalid Slave ID, using default 1." << std::endl;
        }
    }

    std::cout << "\n[INFO] Opening serial port " << portName << " at 19200 baud..." << std::endl;
    HANDLE hSerial = openPort(portName, 19200);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "[FATAL] Unable to open COM port " << portName << std::endl;
        return 1;
    }

    // Ping Verification: Try to read Azimuth Current Register (3)
    int16_t pingVal = 0;
    std::cout << "[INFO] Verifying connection by pinging positioner..." << std::endl;
    if (readRegisters(hSerial, slaveId, 3, 1, &pingVal)) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  [SUCCESS] TAP-3001 POSITIONER CONNECTED!" << std::endl;
        std::cout << "  Communication verified on " << portName << std::endl;
        std::cout << "  Initial Azimuth position: " << (pingVal / 10.0f) << " deg (Raw: " << pingVal << ")" << std::endl;
        std::cout << "=======================================================\n" << std::endl;
    } else {
        std::cerr << "\n=======================================================" << std::endl;
        std::cerr << "  [ERROR] PING FAILED! Connection opened, but device did" << std::endl;
        std::cerr << "  not respond. Check power, wiring, or Slave ID." << std::endl;
        std::cerr << "=======================================================\n" << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    while (true) {
        std::cout << "--- MENU ---" << std::endl;
        std::cout << "1. Read Current Coordinates (AZ, EL, PL)" << std::endl;
        std::cout << "2. Move Azimuth (AZREF) - [0.0 to 360.0]" << std::endl;
        std::cout << "3. Move Elevation (ELREF) - [-90.0 to 90.0]" << std::endl;
        std::cout << "4. Move Polarization (PLREF) - [0.0 to 360.0]" << std::endl;
        std::cout << "5. Reset all angles to 0" << std::endl;
        std::cout << "6. Exit" << std::endl;
        std::cout << "Select option [1-6]: ";

        std::string optionStr;
        std::getline(std::cin, optionStr);
        if (optionStr.empty()) continue;
        int option = 0;
        try { option = std::stoi(optionStr); } catch(...) { continue; }

        if (option == 1) {
            // Read holding registers:
            // AZCUR: 3, ELCUR: 11, PLCUR: 19
            int16_t azVal = 0, elVal = 0, plVal = 0;
            bool success = true;

            if (!readRegisters(hSerial, slaveId, 3, 1, &azVal)) success = false;
            if (!readRegisters(hSerial, slaveId, 11, 1, &elVal)) success = false;
            if (!readRegisters(hSerial, slaveId, 19, 1, &plVal)) success = false;

            if (success) {
                float azDeg = azVal / 10.0f;
                float elDeg = elVal / 10.0f;
                float plDeg = plVal / 10.0f;

                std::cout << "\n[SUCCESS] Current Positioner Coordinates:" << std::endl;
                std::cout << "  Azimuth      (AZCUR / Address 3) : " 
                          << std::fixed << std::setprecision(1) << std::setw(5) << azDeg << " deg"
                          << " (Raw register value: " << std::setw(5) << azVal << ")" << std::endl;
                std::cout << "  Elevation    (ELCUR / Address 11): " 
                          << std::fixed << std::setprecision(1) << std::setw(5) << elDeg << " deg"
                          << " (Raw register value: " << std::setw(5) << elVal << ")" << std::endl;
                std::cout << "  Polarization (PLCUR / Address 19): " 
                          << std::fixed << std::setprecision(1) << std::setw(5) << plDeg << " deg"
                          << " (Raw register value: " << std::setw(5) << plVal << ")" << std::endl;
                std::cout << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to query coordinates from positioner." << std::endl;
            }
        } 
        else if (option == 2) {
            std::cout << "Enter target Azimuth angle [0.0 to 360.0]: ";
            std::string targetStr;
            std::getline(std::cin, targetStr);
            if (!targetStr.empty()) {
                try {
                    float targetVal = std::stof(targetStr);
                    if (targetVal < 0.0f || targetVal > 360.0f) {
                        std::cout << "[ERROR] Target angle out of bounds [0.0 - 360.0]!" << std::endl;
                    } else {
                        int16_t writeVal = (int16_t)(targetVal * 10.0f);
                        std::cout << "[INFO] Writing " << writeVal << " to AZREF (Address 2)..." << std::endl;
                        if (writeRegister(hSerial, slaveId, 2, writeVal)) {
                            std::cout << "[SUCCESS] Target Azimuth angle set to " << targetVal << " deg." << std::endl;
                        }
                    }
                } catch(...) {
                    std::cout << "[ERROR] Invalid numeric input." << std::endl;
                }
            }
        } 
        else if (option == 3) {
            std::cout << "Enter target Elevation angle [-90.0 to 90.0]: ";
            std::string targetStr;
            std::getline(std::cin, targetStr);
            if (!targetStr.empty()) {
                try {
                    float targetVal = std::stof(targetStr);
                    if (targetVal < -90.0f || targetVal > 90.0f) {
                        std::cout << "[ERROR] Target angle out of bounds [-90.0 - 90.0]!" << std::endl;
                    } else {
                        int16_t writeVal = (int16_t)(targetVal * 10.0f);
                        std::cout << "[INFO] Writing " << writeVal << " to ELREF (Address 10)..." << std::endl;
                        if (writeRegister(hSerial, slaveId, 10, writeVal)) {
                            std::cout << "[SUCCESS] Target Elevation angle set to " << targetVal << " deg." << std::endl;
                        }
                    }
                } catch(...) {
                    std::cout << "[ERROR] Invalid numeric input." << std::endl;
                }
            }
        } 
        else if (option == 4) {
            std::cout << "Enter target Polarization angle [0.0 to 360.0]: ";
            std::string targetStr;
            std::getline(std::cin, targetStr);
            if (!targetStr.empty()) {
                try {
                    float targetVal = std::stof(targetStr);
                    if (targetVal < 0.0f || targetVal > 360.0f) {
                        std::cout << "[ERROR] Target angle out of bounds [0.0 - 360.0]!" << std::endl;
                    } else {
                        int16_t writeVal = (int16_t)(targetVal * 10.0f);
                        std::cout << "[INFO] Writing " << writeVal << " to PLREF (Address 18)..." << std::endl;
                        if (writeRegister(hSerial, slaveId, 18, writeVal)) {
                            std::cout << "[SUCCESS] Target Polarization angle set to " << targetVal << " deg." << std::endl;
                        }
                    }
                } catch(...) {
                    std::cout << "[ERROR] Invalid numeric input." << std::endl;
                }
            }
        } 
        else if (option == 5) {
            std::cout << "[INFO] Homing positioner... resetting AZ, EL, and PL to 0.0 deg..." << std::endl;
            bool success = true;
            if (!writeRegister(hSerial, slaveId, 2, 0)) success = false;
            if (!writeRegister(hSerial, slaveId, 10, 0)) success = false;
            if (!writeRegister(hSerial, slaveId, 18, 0)) success = false;
            if (success) {
                std::cout << "[SUCCESS] Reset commands sent successfully to all axes (0.0 deg / raw value 0)." << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to send reset commands to all axes." << std::endl;
            }
        }
        else if (option == 6) {
            break;
        }
    }

    std::cout << "[INFO] Closing serial port..." << std::endl;
    CloseHandle(hSerial);
    std::cout << "[INFO] Finished." << std::endl;
    return 0;
}
