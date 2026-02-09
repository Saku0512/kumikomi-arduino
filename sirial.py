import serial
import subprocess

SERIAL_PORT = "/dev/ttyACM0"
BAUDRATE = 9600
LOCK_WORD = "!!LOCK!!"

ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)

print("Listening serial...")

while True:
    line = ser.readline().decode(errors="ignore").strip()
    if not line:
        continue

    print("Recv:", line)

    if LOCK_WORD in line:
        print("Lock command received. Launching lock screen...")
        subprocess.Popen(["python3", "lock_screen.py"])
