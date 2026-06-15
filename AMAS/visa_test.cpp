#include <iostream>
#include <iomanip>
#include <cstring>
#include <visa.h>

int main() {
    ViSession defaultRM = VI_NULL;
    ViSession instr = VI_NULL;
    ViStatus status = VI_SUCCESS;
    ViUInt32 retCnt = 0;
    unsigned char buffer[256];

    std::cout << "AMAS - Keysight VISA Connection Test" << std::endl;
    std::cout << "====================================" << std::endl;

    // 1. Open the Default Resource Manager
    status = viOpenDefaultRM(&defaultRM);
    if (status < VI_SUCCESS) {
        std::cerr << "[ERROR] Failed to open Default Resource Manager. Status: 0x" 
                  << std::hex << status << std::dec << std::endl;
        return 1;
    }
    std::cout << "[INFO] Successfully opened Default Resource Manager." << std::endl;

    // 2. Open Session to the VNA Instrument
    // Using the default GPIB address specified in the design doc: GPIB0::18::INSTR
    const char* resourceString = "GPIB0::18::INSTR";
    std::cout << "[INFO] Attempting to connect to VNA at: " << resourceString << " ..." << std::endl;
    
    status = viOpen(defaultRM, (ViRsrc)resourceString, VI_NULL, VI_NULL, &instr);
    if (status < VI_SUCCESS) {
        std::cerr << "[ERROR] Failed to open session to " << resourceString << ". Status: 0x" 
                  << std::hex << status << std::dec << std::endl;
        std::cerr << "[TIP] Check if the Keysight VNA is powered on, connected via GPIB/USB, and has address 18." << std::endl;
        viClose(defaultRM);
        return 1;
    }
    std::cout << "[INFO] Successfully connected to VNA." << std::endl;

    // 3. Set a timeout value (5000 ms = 5 seconds)
    status = viSetAttribute(instr, VI_ATTR_TMO_VALUE, 5000);
    if (status < VI_SUCCESS) {
        std::cerr << "[WARNING] Failed to set timeout attribute. Status: 0x" 
                  << std::hex << status << std::dec << std::endl;
    }

    // 4. Send Identification Query (*IDN?)
    const char* idnCmd = "*IDN?\n";
    std::cout << "[INFO] Sending query: *IDN?" << std::endl;
    status = viWrite(instr, (ViBuf)idnCmd, (ViUInt32)strlen(idnCmd), &retCnt);
    if (status < VI_SUCCESS) {
        std::cerr << "[ERROR] Failed to write command to VNA. Status: 0x" 
                  << std::hex << status << std::dec << std::endl;
        viClose(instr);
        viClose(defaultRM);
        return 1;
    }

    // 5. Read response
    std::memset(buffer, 0, sizeof(buffer));
    status = viRead(instr, (ViBuf)buffer, sizeof(buffer) - 1, &retCnt);
    if (status < VI_SUCCESS) {
        std::cerr << "[ERROR] Failed to read response from VNA. Status: 0x" 
                  << std::hex << status << std::dec << std::endl;
    } else {
        std::cout << "[SUCCESS] VNA Identification String received:" << std::endl;
        std::cout << "          " << buffer << std::endl;
    }

    // 6. Clean up resources
    std::cout << "[INFO] Closing sessions..." << std::endl;
    viClose(instr);
    viClose(defaultRM);
    std::cout << "[INFO] Finished." << std::endl;

    return 0;
}
