from pymodbus.client import ModbusSerialClient
import time

print("Opening COM3...")
client = ModbusSerialClient(
    port="COM3",
    baudrate=19200,
    parity="N",
    stopbits=1,
    bytesize=8,
    timeout=3
)

if not client.connect():
    print("FAILED to open COM3")
    exit()

print("Port opened. Waiting 2 seconds for TAP-3001 to settle...")
time.sleep(2)

print("Reading AZ position...")
result = client.read_holding_registers(address=3, count=1, device_id=1)

if not result.isError():
    print(f"SUCCESS! AZ = {result.registers[0] / 10.0} deg")
else:
    print(f"FAILED: {result}")

client.close()
print("Done.")