#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>

uint16_t calculateCRC(const uint8_t* buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; }
            else { crc >>= 1; }
        }
    }
    return crc;
}

int main() {
    std::string portName = "COM3";
    std::string formattedPort = "\\\\.\\" + portName;

    std::cout << "========================================" << std::endl;
    std::cout << "  COM3 Deep Diagnostic Tool" << std::endl;
    std::cout << "========================================" << std::endl;

    // === TEST 1: Check for leftover handles ===
    std::cout << "\n[TEST 1] Opening " << portName << "..." << std::endl;
    HANDLE hSerial = CreateFileA(
        formattedPort.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        std::cerr << "[FAIL] Cannot open " << portName << ". Error: " << err << std::endl;
        if (err == 5) std::cerr << "  -> ERROR_ACCESS_DENIED: Another process is holding the port!" << std::endl;
        if (err == 2) std::cerr << "  -> ERROR_FILE_NOT_FOUND: Port does not exist." << std::endl;
        return 1;
    }
    std::cout << "[OK] Port opened successfully." << std::endl;

    // === TEST 2: Check and clear any pending errors ===
    std::cout << "\n[TEST 2] Checking for pending COM errors..." << std::endl;
    DWORD comErrors = 0;
    COMSTAT comStat;
    if (ClearCommError(hSerial, &comErrors, &comStat)) {
        std::cout << "  Error flags: 0x" << std::hex << comErrors << std::dec << std::endl;
        if (comErrors & CE_BREAK)    std::cout << "    -> CE_BREAK (break condition detected)" << std::endl;
        if (comErrors & CE_FRAME)    std::cout << "    -> CE_FRAME (framing error)" << std::endl;
        if (comErrors & CE_OVERRUN)  std::cout << "    -> CE_OVERRUN (buffer overrun)" << std::endl;
        if (comErrors & CE_RXPARITY) std::cout << "    -> CE_RXPARITY (parity error)" << std::endl;
        if (comErrors & CE_RXOVER)   std::cout << "    -> CE_RXOVER (receive buffer overflow)" << std::endl;
        if (comErrors & CE_TXFULL)   std::cout << "    -> CE_TXFULL (transmit buffer full)" << std::endl;
        if (comErrors == 0) std::cout << "  No pending errors." << std::endl;
        std::cout << "  RX queue: " << comStat.cbInQue << " bytes, TX queue: " << comStat.cbOutQue << " bytes" << std::endl;
    } else {
        std::cerr << "  ClearCommError failed!" << std::endl;
    }

    // === TEST 3: Check modem status lines ===
    std::cout << "\n[TEST 3] Checking modem control lines..." << std::endl;
    DWORD modemStatus = 0;
    if (GetCommModemStatus(hSerial, &modemStatus)) {
        std::cout << "  CTS:  " << ((modemStatus & MS_CTS_ON) ? "ON" : "OFF") << std::endl;
        std::cout << "  DSR:  " << ((modemStatus & MS_DSR_ON) ? "ON" : "OFF") << std::endl;
        std::cout << "  RING: " << ((modemStatus & MS_RING_ON) ? "ON" : "OFF") << std::endl;
        std::cout << "  RLSD: " << ((modemStatus & MS_RLSD_ON) ? "ON" : "OFF") << std::endl;
    } else {
        std::cerr << "  GetCommModemStatus failed. Error: " << GetLastError() << std::endl;
    }

    // === TEST 4: Read current DCB state ===
    std::cout << "\n[TEST 4] Reading current DCB port state..." << std::endl;
    DCB currentDcb = {0};
    currentDcb.DCBlength = sizeof(currentDcb);
    if (GetCommState(hSerial, &currentDcb)) {
        std::cout << "  BaudRate:       " << currentDcb.BaudRate << std::endl;
        std::cout << "  ByteSize:       " << (int)currentDcb.ByteSize << std::endl;
        std::cout << "  Parity:         " << (int)currentDcb.Parity << std::endl;
        std::cout << "  StopBits:       " << (int)currentDcb.StopBits << std::endl;
        std::cout << "  fBinary:        " << currentDcb.fBinary << std::endl;
        std::cout << "  fAbortOnError:  " << currentDcb.fAbortOnError << std::endl;
        std::cout << "  fOutxCtsFlow:   " << currentDcb.fOutxCtsFlow << std::endl;
        std::cout << "  fOutxDsrFlow:   " << currentDcb.fOutxDsrFlow << std::endl;
        std::cout << "  fDtrControl:    " << currentDcb.fDtrControl << std::endl;
        std::cout << "  fRtsControl:    " << currentDcb.fRtsControl << std::endl;
        std::cout << "  fOutX:          " << currentDcb.fOutX << std::endl;
        std::cout << "  fInX:           " << currentDcb.fInX << std::endl;
    }

    // === TEST 5: Force full port reset ===
    std::cout << "\n[TEST 5] Forcing full port reset..." << std::endl;

    // Escape any pending comm function
    EscapeCommFunction(hSerial, CLRDTR);
    EscapeCommFunction(hSerial, CLRRTS);
    Sleep(100);

    // Purge everything
    PurgeComm(hSerial, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

    // Clear errors again
    ClearCommError(hSerial, &comErrors, &comStat);

    // Now configure from scratch using BuildCommDCB (bypasses GetCommState entirely)
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    if (!BuildCommDCBA("baud=19200 parity=N data=8 stop=1", &dcb)) {
        std::cerr << "  BuildCommDCB failed!" << std::endl;
    }

    // Explicitly set every flow control field
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(hSerial, &dcb)) {
        std::cerr << "  SetCommState failed! Error: " << GetLastError() << std::endl;
    } else {
        std::cout << "  Port configured: 19200 8N1 (BuildCommDCB method)." << std::endl;
    }

    // Raise DTR and RTS
    EscapeCommFunction(hSerial, SETDTR);
    EscapeCommFunction(hSerial, SETRTS);
    Sleep(200); // Let the adapter settle

    // Set timeouts
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 3000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    // Clear errors one final time
    ClearCommError(hSerial, &comErrors, &comStat);
    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    std::cout << "  Port reset complete." << std::endl;

    // === TEST 6: Send Modbus ping ===
    std::cout << "\n[TEST 6] Sending Modbus Read (Slave=1, Reg=3, FC=03)..." << std::endl;

    uint8_t frame[8];
    frame[0] = 0x01; // Slave ID
    frame[1] = 0x03; // Function Code 03
    frame[2] = 0x00; // Address high
    frame[3] = 0x03; // Address low (register 3 = AZCUR)
    frame[4] = 0x00; // Quantity high
    frame[5] = 0x01; // Quantity low (1 register)
    uint16_t crc = calculateCRC(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    std::cout << "  TX: ";
    for (int i = 0; i < 8; i++) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)frame[i] << " ";
    }
    std::cout << std::dec << std::endl;

    DWORD written = 0;
    if (!WriteFile(hSerial, frame, 8, &written, NULL) || written != 8) {
        std::cerr << "  [FAIL] WriteFile failed! Written: " << written << ", Error: " << GetLastError() << std::endl;
    } else {
        std::cout << "  [OK] Wrote " << written << " bytes." << std::endl;
    }

    // Check for errors after write
    ClearCommError(hSerial, &comErrors, &comStat);
    if (comErrors) {
        std::cerr << "  [WARNING] Post-write errors: 0x" << std::hex << comErrors << std::dec << std::endl;
    }

    std::cout << "  Waiting for response (3 second timeout)..." << std::endl;

    uint8_t rxBuf[256] = {0};
    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(hSerial, rxBuf, sizeof(rxBuf), &bytesRead, NULL);

    // Check for errors after read
    ClearCommError(hSerial, &comErrors, &comStat);

    if (!readOk) {
        std::cerr << "  [FAIL] ReadFile returned FALSE. Error: " << GetLastError() << std::endl;
    } else if (bytesRead == 0) {
        std::cout << "  [FAIL] Read 0 bytes (timeout). No response from device." << std::endl;
        if (comErrors) {
            std::cerr << "  Post-read errors: 0x" << std::hex << comErrors << std::dec << std::endl;
        }
    } else {
        std::cout << "  [OK] Received " << bytesRead << " bytes:" << std::endl;
        std::cout << "  RX: ";
        for (DWORD i = 0; i < bytesRead; i++) {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)rxBuf[i] << " ";
        }
        std::cout << std::dec << std::endl;

        if (bytesRead >= 7 && rxBuf[1] == 0x03) {
            uint16_t respCrc = calculateCRC(rxBuf, 5);
            uint16_t respCrcRec = rxBuf[5] | (rxBuf[6] << 8);
            if (respCrc == respCrcRec) {
                int16_t val = (rxBuf[3] << 8) | rxBuf[4];
                std::cout << "  [SUCCESS] CRC OK. Azimuth = " << (val / 10.0f) << " deg" << std::endl;
            } else {
                std::cerr << "  [WARNING] CRC mismatch." << std::endl;
            }
        }
    }

    // === TEST 7: Try again with FC 16 (0x10) write-style read ===
    std::cout << "\n[TEST 7] Retrying with longer inter-frame delay..." << std::endl;
    Sleep(500);
    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    ClearCommError(hSerial, &comErrors, &comStat);
    Sleep(100);

    written = 0;
    WriteFile(hSerial, frame, 8, &written, NULL);
    std::cout << "  Wrote " << written << " bytes. Waiting..." << std::endl;

    bytesRead = 0;
    ReadFile(hSerial, rxBuf, sizeof(rxBuf), &bytesRead, NULL);
    ClearCommError(hSerial, &comErrors, &comStat);

    if (bytesRead > 0) {
        std::cout << "  [OK] Received " << bytesRead << " bytes on retry!" << std::endl;
        std::cout << "  RX: ";
        for (DWORD i = 0; i < bytesRead; i++) {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)rxBuf[i] << " ";
        }
        std::cout << std::dec << std::endl;
    } else {
        std::cout << "  [FAIL] Still 0 bytes on retry." << std::endl;
    }

    // === SUMMARY ===
    std::cout << "\n========================================" << std::endl;
    std::cout << "  DIAGNOSIS SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "If all tests show 0 bytes received, the issue is likely:" << std::endl;
    std::cout << "  1. FTDI driver stuck: Unplug USB adapter, wait 5 sec, re-plug" << std::endl;
    std::cout << "  2. Disable+Re-enable COM3 in Device Manager" << std::endl;
    std::cout << "  3. RS-485 adapter polarity flipped (swap A/B wires)" << std::endl;
    std::cout << "  4. Positioner controller box power-cycled needed" << std::endl;

    CloseHandle(hSerial);
    return 0;
}
