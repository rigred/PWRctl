import os
import sys
import serial
import argparse


my_parser = argparse.ArgumentParser(description='Control Computers via Arduino')


my_parser.add_argument('Device',
					   metavar='devnum',
					   type=int,
					   help='The device to issue a command to')
my_parser.add_argument('Action',
					   metavar='action',
					   type=str,
					   help='The command to issue to the device')
my_parser.add_argument()

# Execute the parse_args() method
args = my_parser.parse_args()

devname = args.Device



#ser = serial.Serial('/dev/ttyUSB0', 9600, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=0)
#print(ser.name)
#ser.write(node2On.to_bytes(1, byteorder='big'))
#buf = ser.readline();
#print(buf)
#ser.close()


