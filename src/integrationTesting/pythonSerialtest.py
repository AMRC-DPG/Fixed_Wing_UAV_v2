import sys
import serial
import serial
import serial.threaded
import time

def main():
    with serial.Serial('COM3', 115200) as ser:
        while True:
            x = ser.read(4) #read 4 bytes
            print(x)


if __name__ == 'main':
    main()
