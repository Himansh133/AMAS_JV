# AMAS Development Log

**Project:** Antenna Measurement Automation Software (AMAS)

**Intern:** Mahima Ancy Verghese

---

# Project Goal

Develop an industrial antenna measurement software capable of controlling:

- TAP3001 Antenna Positioner
- Keysight N9951B FieldFox VNA

using C++, VISA, SCPI and Qt.

---

# Current Project Status

## Positioner

Status: ✅ Completed

Successfully established Modbus RTU communication with TAP3001.

Implemented:

- Connect
- Read Coordinates
- Move Azimuth
- Move Elevation
- Move Polarization
- Home Positioner
- Hardware Verification

---

## VNA

Status: ✅ Communication Established

Successfully established:

- Ethernet Communication
- VISA Communication
- SCPI Communication

Verified using Keysight Connection Expert.

---

# Development Timeline

---

# Phase 1 : Positioner Communication

Initial Goal

Establish RS485 Modbus communication with TAP3001.

---

## Problems Encountered

### Problem 1

No communication.

Output:

```
Read timeout.
Expected 7 bytes.
Read 0 bytes.
```

### Investigation

Checked

- COM Port
- Slave ID
- Baud Rate
- Serial Settings

---

### Problem 2

COM Port mismatch.

Originally

```
COM8
```

Hardware actually connected on

```
COM3
```

Updated source files accordingly.

---

### Problem 3

Incorrect Serial Configuration

Windows showed

```
1200
Even
7 Data Bits
```

Correct configuration

```
19200
No Parity
8 Data Bits
1 Stop Bit
```

Applied using

```
mode COM3: BAUD=19200 PARITY=N DATA=8 STOP=1
```

---

### Problem 4

Parity mismatch

Driver was modified to

```cpp
EVENPARITY
```

Original working version used

```cpp
NOPARITY
```

Restored original implementation.

Communication resumed.

---

### Problem 5

Git mistakes

Accidentally pushed experimental code.

Deleted commits.

Lost working state.

---

### Recovery

Created emergency branch

```
emergency-backup
```

Used

```
git diff

git stash

git checkout

git restore
```

Recovered working implementation.

---

### Final Result

Successfully implemented

- Read Position
- Move AZ
- Move EL
- Move PL
- Home Positioner

Verified physically on hardware.

---

# Lessons Learned

Always

- create feature branches
- commit working code first
- avoid testing directly on main

---

# Phase 2 : VNA Communication

Goal

Establish Ethernet communication with Keysight N9951B.

---

## Initial Problems

Laptop had

No Ethernet Port.

---

### Solution

Used

TP-Link TL-MR3020

as Ethernet bridge.

---

### Problem

Router configuration confusion.

Initially

- Phone Hotspot
- Router
- VNA

were on different networks.

---

### Investigation

Performed

```
ipconfig

ping
```

Verified network.

---

### Problem

Initially browser opened TP-Link webpage instead of VNA.

Reason

Wrong address being accessed.

---

### Solution

Verified actual VNA IP

```
192.168.113.206
```

Opened

```
http://192.168.113.206
```

Successfully reached

Keysight FieldFox Web Interface.

---

### Network Verification

Ping

```
Packets Sent : 4

Packets Received : 4

Lost : 0
```

Network confirmed working.

---

# VISA Communication

Installed

Keysight IO Libraries Suite.

Opened

Keysight Connection Expert.

Initially

Instrument appeared as

```
Unknown
```

---

### Root Cause

Incorrect address entered.

Originally entered

```
TCPIP0::192.168.113.206::inst0::INSTR
```

inside

Hostname/IP field.

Incorrect.

---

### Correct Configuration

Hostname

```
192.168.113.206
```

Protocol

```
Instrument (VXI-11)
```

Remote Name

```
inst0
```

---

### Result

Connection Expert

Status

```
Connected
```

---

# SCPI Verification

Successfully executed

---

## Identity

```
*IDN?
```

Returned

```
Keysight Technologies
N9951B
...
```

---

## Firmware Version

```
:SYST:VERS?
```

Returned

```
1999.0
```

---

## Error Queue

```
:SYST:ERR?
```

Returned

```
0,"No error"
```

---

SCPI communication fully verified.

---

# Major Milestones Achieved

## Positioner

✅ Modbus RTU Communication

✅ Read Coordinates

✅ Move Position

✅ Home

---

## VNA

✅ Ethernet Communication

✅ Browser Access

✅ VISA Session

✅ SCPI Commands

---

# Current Project Architecture

```
                 Qt GUI
                    │
        Measurement Controller
         /                    \
 Positioner Driver      FieldFox Driver
        │                     │
    Modbus RTU          VISA + SCPI
         \                   /
          Data Acquisition
                 │
          CSV / Reports
```

---

# Next Development Tasks

## FieldFoxDriver

Implement

```cpp
connect()

disconnect()

identify()

query()

writeCommand()
```

---

## Frequency Control

Implement

```
setStartFrequency()

setStopFrequency()

setCenterFrequency()

setSpan()
```

---

## Sweep Control

Implement

```
triggerSweep()

setPoints()

setIFBW()

setPower()
```

---

## Trace Acquisition

Implement

```
readTrace()
```

Store results inside

```
std::vector<double>
```

---

## Automation

Combine

Positioner

+

VNA

Example

```
Move Positioner

↓

Trigger Sweep

↓

Read Trace

↓

Store Data

↓

Repeat
```

---

## GUI

Develop Qt interface for

- VNA Connection
- Positioner Connection
- Sweep Settings
- Live Logs
- Measurement Controls
- Real Time Plot
- Data Export

---

# Technologies Used

Language

- C++17

Libraries

- VISA
- SCPI
- Qt (Upcoming)

Protocols

- Modbus RTU
- VISA
- SCPI
- Ethernet

Hardware

- TAP3001 Positioner
- Keysight N9951B FieldFox

Tools

- Git
- CMake
- Visual Studio 2022
- Keysight IO Libraries Suite
- Keysight Connection Expert

---

# Current Status

| Module | Status |
|---------|--------|
| Git | ✅ |
| CMake | ✅ |
| Positioner Driver | ✅ |
| Positioner Hardware Verification | ✅ |
| Modbus RTU | ✅ |
| Ethernet | ✅ |
| TP-Link Bridge | ✅ |
| VNA Browser Access | ✅ |
| VISA | ✅ |
| SCPI | ✅ |
| FieldFox Driver | 🔄 Next |
| Qt GUI | ⏳ |
| Measurement Automation | ⏳ |

---

# Biggest Lessons Learned

- Always verify hardware before debugging software.
- Test communication layer independently.
- Never push unverified code to main.
- Commit working versions frequently.
- Build software in layers:
    1. Hardware
    2. Driver
    3. Controller
    4. GUI

This approach significantly simplifies debugging and improves maintainability.