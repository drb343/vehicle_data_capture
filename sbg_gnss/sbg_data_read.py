import serial
import time
import os

PORT = '/dev/ttyUSB0'
BAUD = 115200

# Create timestamped filename
timestamp = time.strftime("%Y%m%d_%H%M%S")
filename = f"ellipse_{timestamp}.sbf"

ser = serial.Serial(PORT, BAUD, timeout=1)

print(f"Logging to {filename}...")

with open(filename, 'wb') as f:
    try:
        while True:
            data = ser.read(4096)
            if data:
                f.write(data)
    except KeyboardInterrupt:
        print("Logging stopped.")
        ser.close()