import sys
import serial
import serial
import serial.threaded
import time

def main():
    with ser = serial.Serial('COM3', 115200):
        while True:
            x = ser.read(4) #read 4 bytes
            print(x)


if __name__ == 'main':
    main()
