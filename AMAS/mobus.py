from pymodbus.client import ModbusSerialClient
import time

client = ModbusSerialClient(
    port="COM3",
    baudrate=19200,
    parity="N",
    stopbits=1,
    bytesize=8,
    timeout=3
)

if not client.connect():
    print("Could not connect")
    exit()

print("Connected — waiting 2 seconds for device to settle...")
time.sleep(2)

for i in range(5):
    result = client.read_holding_registers(address=3, count=1, device_id=1)
    if not result.isError():
        print(f"Read {i+1}: AZ = {result.registers[0] / 10.0} deg")
    else:
        print(f"Read {i+1}: FAILED")
    time.sleep(2)   # 2 second gap between reads