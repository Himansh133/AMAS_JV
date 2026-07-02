#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> getAvailableCOMPorts() {
  std::vector<std::string> ports;
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0,
                    KEY_READ, &hKey) == ERROR_SUCCESS) {
    char valueName[256];
    BYTE data[256];
    DWORD valueNameSize, dataSize, type, index = 0;
    while (true) {
      valueNameSize = sizeof(valueName);
      dataSize = sizeof(data);
      if (RegEnumValueA(hKey, index, valueName, &valueNameSize, NULL, &type,
                        data, &dataSize) != ERROR_SUCCESS)
        break;
      if (type == REG_SZ)
        ports.push_back((char *)data);
      index++;
    }
    RegCloseKey(hKey);
  }
  return ports;
}

uint16_t calculateCRC(const uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];
    for (int i = 8; i != 0; i--) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else
        crc >>= 1;
    }
  }
  return crc;
}

// ── Print bytes as hex for debugging ─────────────────────────────────────────
void printHex(const char *label, const uint8_t *buf, DWORD len) {
  std::cout << label;
  for (DWORD i = 0; i < len; i++)
    std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << (int)buf[i] << " ";
  std::cout << std::dec << "\n";
}

HANDLE openPort(const std::string &portName, DWORD baudRate = 19200) {
  std::string fp = "\\\\.\\" + portName;
  HANDLE h = CreateFileA(fp.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                         OPEN_EXISTING, 0, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    std::cerr << "[ERROR] Cannot open " << portName << " — error "
              << GetLastError() << "\n";
    return INVALID_HANDLE_VALUE;
  }

  DCB dcb = {0};
  dcb.DCBlength = sizeof(dcb);
  if (!GetCommState(h, &dcb)) {
    std::cerr << "[ERROR] GetCommState failed\n";
    CloseHandle(h);
    return INVALID_HANDLE_VALUE;
  }

  dcb.BaudRate = baudRate;
  dcb.ByteSize = 8;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity = NOPARITY;
  dcb.fBinary = TRUE;
  dcb.fAbortOnError = FALSE;
  dcb.fOutxCtsFlow = FALSE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDsrSensitivity = FALSE;
  dcb.fTXContinueOnXoff = FALSE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  dcb.fErrorChar = FALSE;
  dcb.fNull = FALSE;
  // RTS_CONTROL_TOGGLE: Windows auto-asserts RTS during TX, deasserts for RX
  // This is correct for RS-485 half-duplex direction control
  dcb.fRtsControl = RTS_CONTROL_TOGGLE;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;

  if (!SetCommState(h, &dcb)) {
    std::cerr << "[ERROR] SetCommState failed\n";
    CloseHandle(h);
    return INVALID_HANDLE_VALUE;
  }

  // Short interval timeout — return as soon as inter-byte gap > 20ms
  // Total timeout is 3000ms as safety net
  COMMTIMEOUTS to = {0};
  to.ReadIntervalTimeout = 20;
  to.ReadTotalTimeoutConstant = 3000;
  to.ReadTotalTimeoutMultiplier = 5;
  to.WriteTotalTimeoutConstant = 500;
  to.WriteTotalTimeoutMultiplier = 10;

  if (!SetCommTimeouts(h, &to)) {
    std::cerr << "[ERROR] SetCommTimeouts failed\n";
    CloseHandle(h);
    return INVALID_HANDLE_VALUE;
  }

  return h;
}

// ── Accumulating read — collects bytes until we have enough or timeout
// ────────
bool readExact(HANDLE h, uint8_t *buf, DWORD needed, DWORD timeoutMs = 3000) {
  DWORD total = 0;
  DWORD start = GetTickCount();
  while (total < needed) {
    if (GetTickCount() - start > timeoutMs) {
      std::cerr << "[ERROR] Read timeout — got " << total << " of " << needed
                << " bytes\n";
      return false;
    }
    DWORD got = 0;
    if (ReadFile(h, buf + total, needed - total, &got, NULL) && got > 0)
      total += got;
    else
      Sleep(2);
  }
  return true;
}

bool readRegisters(HANDLE h, uint8_t slaveId, uint16_t address,
                   uint16_t numRegs, int16_t *out) {
  uint8_t frame[8];
  frame[0] = slaveId;
  frame[1] = 0x03;
  frame[2] = (address >> 8) & 0xFF;
  frame[3] = address & 0xFF;
  frame[4] = (numRegs >> 8) & 0xFF;
  frame[5] = numRegs & 0xFF;
  uint16_t crc = calculateCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;

  PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
  Sleep(5); // settle after purge

  printHex("[TX] ", frame, 8);

  DWORD written = 0;
  if (!WriteFile(h, frame, 8, &written, NULL) || written != 8) {
    std::cerr << "[ERROR] WriteFile failed\n";
    return false;
  }

  // Wait for TX to complete before switching to RX
  // At 19200 baud, 8 bytes take ~4.2ms — 15ms gives comfortable margin
  Sleep(15);

  DWORD expected = 5 + 2 * numRegs;
  uint8_t resp[256] = {0};

  if (!readExact(h, resp, expected))
    return false;

  printHex("[RX] ", resp, expected);

  // CRC check
  uint16_t calcCrc = calculateCRC(resp, expected - 2);
  uint16_t recvCrc = resp[expected - 2] | (resp[expected - 1] << 8);
  if (calcCrc != recvCrc) {
    std::cerr << "[ERROR] CRC mismatch — calc=0x" << std::hex << calcCrc
              << " recv=0x" << recvCrc << std::dec << "\n";
    return false;
  }

  if (resp[1] & 0x80) {
    std::cerr << "[ERROR] Modbus exception code " << (int)resp[2] << "\n";
    return false;
  }

  for (int i = 0; i < numRegs; i++)
    out[i] = (int16_t)((resp[3 + 2 * i] << 8) | resp[4 + 2 * i]);

  return true;
}

bool writeRegister(HANDLE h, uint8_t slaveId, uint16_t address, int16_t value) {
  uint8_t frame[11];
  frame[0] = slaveId;
  frame[1] = 0x10;
  frame[2] = (address >> 8) & 0xFF;
  frame[3] = address & 0xFF;
  frame[4] = 0x00;
  frame[5] = 0x01;
  frame[6] = 0x02;
  frame[7] = (value >> 8) & 0xFF;
  frame[8] = value & 0xFF;
  uint16_t crc = calculateCRC(frame, 9);
  frame[9] = crc & 0xFF;
  frame[10] = (crc >> 8) & 0xFF;

  PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
  Sleep(10);

  printHex("[TX] ", frame, 11);

  DWORD written = 0;
  if (!WriteFile(h, frame, 11, &written, NULL) || written != 11) {
    std::cerr << "[ERROR] WriteFile failed\n";
    return false;
  }

  Sleep(50); // increased delay for RTS toggle and device response // TX complete wait

  uint8_t resp[8] = {0};
  if (!readExact(h, resp, 8))
    return false;

  printHex("[RX] ", resp, 8);

  uint16_t calcCrc = calculateCRC(resp, 6);
  uint16_t recvCrc = resp[6] | (resp[7] << 8);
  if (calcCrc != recvCrc) {
    std::cerr << "[ERROR] CRC mismatch\n";
    return false;
  }
  if (resp[1] & 0x80) {
    std::cerr << "[ERROR] Modbus exception " << (int)resp[2] << "\n";
    return false;
  }
  return true;
}

int main() {
  std::cout << "===========================================\n";
  std::cout << "      TAP-3001 Positioner Control Tool     \n";
  std::cout << "===========================================\n";

  auto ports = getAvailableCOMPorts();
  std::string portName = "COM3";

  if (ports.empty()) {
    std::cout << "[WARNING] No COM ports found.\nEnter port [default COM3]: ";
    std::string s;
    std::getline(std::cin, s);
    if (!s.empty())
      portName = s;
  } else {
    std::cout << "[INFO] Detected COM ports:\n";
    for (size_t i = 0; i < ports.size(); i++)
      std::cout << "  " << (i + 1) << ". " << ports[i] << "\n";
    std::cout << "Select [1-" << ports.size() << "] [default 1]: ";
    std::string s;
    std::getline(std::cin, s);
    if (s.empty()) {
      portName = ports[0];
    } else {
      try {
        int idx = std::stoi(s);
        portName = (idx >= 1 && idx <= (int)ports.size()) ? ports[idx - 1] : s;
      } catch (...) {
        portName = s;
      }
    }
  }

  uint8_t slaveId = 1;
  std::cout << "Enter Modbus Slave ID [default 1]: ";
  std::string sl;
  std::getline(std::cin, sl);
  if (!sl.empty()) {
    try {
      slaveId = (uint8_t)std::stoi(sl);
    } catch (...) {
      std::cout << "Invalid, using 1\n";
    }
  }

  std::cout << "\n[INFO] Opening " << portName << " at 19200 baud...\n";
  HANDLE h = openPort(portName, 19200);
  if (h == INVALID_HANDLE_VALUE) {
    std::cerr << "[FATAL] Cannot open port\n";
    return 1;
  }
  std::cout << "[INFO] Waiting for device to settle...\n";
  Sleep(4000);

  std::cout << "[INFO] Pinging positioner (reading AZCUR register)...\n";
  int16_t pingVal = 0;
  if (readRegisters(h, slaveId, 3, 1, &pingVal)) {
    std::cout << "\n=======================================================\n";
    std::cout << "  [SUCCESS] TAP-3001 CONNECTED on " << portName << "\n";
    std::cout << "  Azimuth: " << (pingVal / 10.0f) << " deg (raw: " << pingVal
              << ")\n";
    std::cout << "=======================================================\n\n";
  } else {
    std::cerr << "\n=======================================================\n";
    std::cerr << "  [FAIL] No response. Check power, wiring, Slave ID.\n";
    std::cerr << "=======================================================\n\n";
    CloseHandle(h);
    return 1;
  }

  while (true) {
    std::cout << "--- MENU ---\n";
    std::cout << "1. Read coordinates (AZ, EL, PL)\n";
    std::cout << "2. Move Azimuth    [0 - 360]\n";
    std::cout << "3. Move Elevation  [-90 - 90]\n";
    std::cout << "4. Move Polarization [0 - 360]\n";
    std::cout << "5. Home all axes to 0\n";
    std::cout << "6. Exit\n";
    std::cout << "Select: ";

    std::string opt;
    std::getline(std::cin, opt);
    if (opt.empty())
      continue;
    int option = 0;
    try {
      option = std::stoi(opt);
    } catch (...) {
      continue;
    }

    if (option == 1) {
      int16_t az = 0, el = 0, pl = 0;
      bool ok = readRegisters(h, slaveId, 3, 1, &az) &&
                readRegisters(h, slaveId, 11, 1, &el) &&
                readRegisters(h, slaveId, 19, 1, &pl);
      if (ok) {
        std::cout << "\n[SUCCESS] Coordinates:\n";
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  AZ : " << az / 10.0f << " deg\n";
        std::cout << "  EL : " << el / 10.0f << " deg\n";
        std::cout << "  PL : " << pl / 10.0f << " deg\n\n";
      } else {
        std::cerr << "[ERROR] Failed to read coordinates\n";
      }
    } else if (option == 2) {
      std::cout << "Target AZ [0-360]: ";
      std::string s;
      std::getline(std::cin, s);
      try {
        float v = std::stof(s);
        if (v < 0 || v > 360) {
          std::cout << "[ERROR] Out of range\n";
          continue;
        }
        if (writeRegister(h, slaveId, 2, (int16_t)(v * 10)))
          std::cout << "[SUCCESS] AZ set to " << v << " deg\n";
      } catch (...) {
        std::cout << "[ERROR] Invalid input\n";
      }
    } else if (option == 3) {
      std::cout << "Target EL [-90 to 90]: ";
      std::string s;
      std::getline(std::cin, s);
      try {
        float v = std::stof(s);
        if (v < -90 || v > 90) {
          std::cout << "[ERROR] Out of range\n";
          continue;
        }
        if (writeRegister(h, slaveId, 10, (int16_t)(v * 10)))
          std::cout << "[SUCCESS] EL set to " << v << " deg\n";
      } catch (...) {
        std::cout << "[ERROR] Invalid input\n";
      }
    } else if (option == 4) {
      std::cout << "Target PL [0-360]: ";
      std::string s;
      std::getline(std::cin, s);
      try {
        float v = std::stof(s);
        if (v < 0 || v > 360) {
          std::cout << "[ERROR] Out of range\n";
          continue;
        }
        if (writeRegister(h, slaveId, 18, (int16_t)(v * 10)))
          std::cout << "[SUCCESS] PL set to " << v << " deg\n";
      } catch (...) {
        std::cout << "[ERROR] Invalid input\n";
      }
    } else if (option == 5) {
      bool ok = writeRegister(h, slaveId, 2, 0) &&
                writeRegister(h, slaveId, 10, 0) &&
                writeRegister(h, slaveId, 18, 0);
      std::cout << (ok ? "[SUCCESS] All axes homed to 0\n"
                       : "[ERROR] Homing failed\n");
    } else if (option == 6) {
      break;
    }
  }

  CloseHandle(h);
  std::cout << "[INFO] Done.\n";
  return 0;
}