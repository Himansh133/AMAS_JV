import logging
logging.basicConfig(level=logging.DEBUG)

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

print("Connected")
time.sleep(2)

result = client.read_holding_registers(
    address=3,
    count=1,
    device_id=1
)

print(result)