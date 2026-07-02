# Understanding VNA S11 Measurements & Integration

This document explains the physical meaning, SCPI command sequence, and mathematical calculations involved in how `visa_test.cpp` measures the **S11 (Reflection Coefficient)** parameter during turntable sweeps.

---

## 1. What is the S11 Parameter?

In RF/antenna engineering, **S-parameters** (Scattering parameters) describe how power behaves in an electrical network:

*   **S11** represents the **Reflection Coefficient** at Port 1 of the VNA.
*   It measures how much power is reflected back from the antenna under test (AUT) compared to the power sent to it.
*   **Magnitude (in dB):**
    *   An S11 of **0 dB** means 100% of the power is reflected (antenna is not radiating at all).
    *   An S11 of **-10 dB** means 10% of the power is reflected (90% is delivered to the antenna).
    *   An S11 of **-20 dB** or lower represents an excellent match (99%+ of the power is delivered).
*   **Phase (in degrees):**
    *   Represents the phase shift of the reflected wave relative to the incident wave, which is critical for determining the input impedance ($Z_{in} = R + jX$) of the antenna.

---

## 2. Step-by-Step Measurement Walkthrough in Code

Our integration program automates this process sequentially:

### Step 2.1: Loading the Chamber Calibration
The chamber has preset calibration parameters for specific frequency bands to subtract cable losses and environment reflections:
```cpp
sendSCPI(instr, "MMEM:LOAD:STAT \"" + calFile + "\"");
```
*   `MMEM:LOAD:STAT` (Mass Memory Load State) tells the VNA to load the state configuration and OSL calibration coefficients.
*   `*OPC?` (Operation Complete query) blocks execution on our PC until the instrument finishes loading the binary state file into its active hardware RAM.

### Step 2.2: Mode & Trace Configuration
```cpp
sendSCPI(instr, "INST:SEL \"NA\"");       // Select Network Analyzer mode
sendSCPI(instr, "CALC:PAR:COUN 1");       // Trace count = 1
sendSCPI(instr, "CALC:PAR1:DEF S11");     // Assign Trace 1 to measure S11
sendSCPI(instr, "CALC:PAR1:SEL");         // Select Trace 1 for queries
sendSCPI(instr, "CALC:FORM MLOG");        // Display log magnitude on the VNA screen
```
*   **Trace Definition:** The command `CALC:PAR1:DEF S11` maps trace 1 specifically to capture Port 1 reflections.

### Step 2.3: Coordinated Sweep Synchronization
For every target turntable angle:
1. The positioner moves to the angle and settles.
2. The VNA is armed for a single sweep:
   ```cpp
   sendSCPI(instr, "INIT:CONT OFF"); // Single sweep mode (stops free-running)
   sendSCPI(instr, "INIT:IMM");      // Trigger a single sweep immediately
   ```
3. The program blocks and waits using:
   ```cpp
   querySCPI(instr, "*OPC?", opcResponse);
   ```
   The VNA will *not* respond to `*OPC?` until the physical sweep finishes scanning across all frequency points. This guarantees we only read fresh data after the sweep completes.

---

## 3. Data Query and Mathematical Conversion

Once the sweep completes, the program queries the raw S11 data:

### Step 3.1: Querying Complex S-Data (SDATA)
```cpp
querySCPI(instr, "CALC:DATA? SDATA", sdata);
```
*   `CALC:DATA? SDATA` requests raw complex data (Real and Imaginary number pairs) for each frequency point.
*   The VNA returns a long comma-separated string:
    $$\text{Real}_1, \text{Imag}_1, \text{Real}_2, \text{Imag}_2, \dots, \text{Real}_N, \text{Imag}_N$$
    where $N$ is the number of sweep points.

### Step 3.2: Splitting and Parsing
The program splits the string by commas and parses each text segment into a floating-point double. If you have 401 sweep points, the VNA returns 802 values.

### Step 3.3: Complex Math to dB Magnitude and Phase
Each data point represents a complex reflection coefficient:
$$\Gamma = R + j \cdot Im$$

For each point $i$, the program calculates:

1.  **Linear Magnitude ($\rho$):**
    $$\rho = |\Gamma| = \sqrt{R^2 + Im^2}$$

2.  **Logarithmic Magnitude (dB):**
    $$\text{Magnitude (dB)} = 20 \cdot \log_{10}(\rho)$$
    *If $\rho$ is close to 0 (no reflection), we cap it at a noise floor of -150 dB to avoid math errors (log(0) is undefined).*

3.  **Phase (Degrees):**
    $$\text{Phase (degrees)} = \text{atan2}(Im, R) \cdot \frac{180}{\pi}$$
    *The `atan2` function computes the correct four-quadrant arctangent (between $-180^\circ$ and $+180^\circ$).*

---

## 4. Saving the Data
The computed values are saved to a CSV file (`measurement_results_<band>.csv`) with the following format:

```csv
Angle_deg,Frequency_Hz,Magnitude_dB,Phase_deg
0.0,18000000000,-15.4215,85.1234
0.0,18010000000,-15.6534,81.9821
...
10.0,18000000000,-12.3325,92.5562
```

This format is easily plotted by `plot_s11.py` or imported into Excel or MATLAB for analysis!
