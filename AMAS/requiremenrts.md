# TAP-3001 Positioner Test Tool - Requirements & Execution Guide

This document lists the requirements and commands needed to compile and run the standalone TAP-3001 positioner testing utility on any Windows machine.

---

## 1. System & Hardware Requirements

To run the pre-compiled program (`positioner_test.exe`) and establish connection with the physical positioner, the following are required:

1. **Operating System:** Windows (Windows 10/11) is required, as the software leverages native Win32 serial handles and registry polling.
2. **Connectivity Interface:** A **USB-to-RS485 adapter** terminal connected to the laptop/PC.
3. **Signal Polarity Check:**
   - Verify that **Data+ (A)** and **Data- (B)** wires are connected correctly.
   - *Troubleshooting:* If you receive connection timeouts (0 bytes read), try swapping the A and B wires on the adapter's terminal block.
4. **Device Power:** The TAP-3001 positioner control box must be powered ON and connected via the COM line.
5. **Port Ownership:** Ensure no other application (like Python measurement scripts or serial monitors) is holding the active COM port open.

---

## 2. Compilation Requirements (Building from Source)

If you need to compile `positioner_test.cpp` on another machine:

1. **Compiler:** **Microsoft Visual C++ Compiler (MSVC)** via Visual Studio 2019/2022 or VC++ Build Tools.
2. **Environment:** Run the command in the **Developer Command Prompt for VS** (e.g., *x64 Native Tools Developer Command Prompt*).
3. **Registry Linkage:** The code uses `#pragma comment(lib, "advapi32.lib")` to automatically link registry utilities, so standard compilation commands work out of the box.

### Compilation Command:
```cmd
cl.exe /EHsc /O2 /std:c++17 positioner_test.cpp /Fe:positioner_test.exe
```

---

## 3. How to Run the Program

1. Copy the executable `positioner_test.exe` to the target computer.
2. Open **PowerShell** or **Command Prompt** in the folder containing the binary.
3. Run the executable:
   ```powershell
   .\positioner_test.exe
   ```
4. **Port Selection:**
   The tool automatically scans the Windows registry and lists all active COM ports. Select the appropriate index:
   ```text
   [INFO] Detected COM ports on this machine:
     1. COM3
     2. COM7
   Select port number [1-2] or type name directly [default: 1]: 2
   ```
5. **Slave ID:**
   Enter the Modbus Slave ID configured on the positioner control box (default is `1`):
   ```text
   Enter Modbus Slave ID [default: 1]: 1
   ```
6. **Connection Verification:**
   The tool pings the controller by performing a Modbus read on Register `3` (`AZCUR`).
   * **Success:** Shows `[SUCCESS] TAP-3001 POSITIONER CONNECTED!` along with the current Azimuth position.
   * **Failure:** Tells you the ping failed and prompts you to check power, wiring, or Slave ID.

---

## 4. Usage Options

Once connected, use the menu to send commands:
* **Option 1 (Read Coordinates):** Reads `AZCUR` (3), `ELCUR` (11), and `PLCUR` (19). Displays both decimal angles and raw register values.
* **Option 2/3/4 (Move Axis):** Checks angle limits before translating and writing targets using **Modbus Function Code 16 (0x10)**:
  - Azimuth Target: `[0.0, 360.0]` deg $\rightarrow$ writes to `2` (`AZREF`)
  - Elevation Target: `[-90.0, 90.0]` deg $\rightarrow$ writes to `10` (`ELREF`)
  - Polarization Target: `[0.0, 360.0]` deg $\rightarrow$ writes to `18` (`PLREF`)
* **Option 5 (Reset All Axes):** Sends target value `0` to AZREF (2), ELREF (10), and PLREF (18) simultaneously to return the positioner to home.
