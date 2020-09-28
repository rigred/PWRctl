#!/usr/bin/python3
import os
import sys
import serial
import argparse

serialPort = '/dev/ttyUSB0' # change as needed depending on system
numDev = 0 # load this value with numDevices queried from Arduino

ser = serial.Serial(serialPort, 9600, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=1)

def power(devId, state):
	if (state):


		print('powering on')
	else :
		print('powering off')

def reset(devId):
	print('system reset')

def kill(devId):
	print('system forced off')

def status(devId):
	print('system status: ')

def query():
	print('all systems status')



my_parser = argparse.ArgumentParser(description='Control Computers via Arduino')


my_parser.add_argument('-d', '--devnum', dest='devnum', action='append',
					   type=int,
					   required=False,
					   metavar='dev id',
					   help='The device to issue a action to')
my_parser.add_argument('action',
					   type = str.lower,
					   metavar='action',
					   default='status',
					   choices=['on', 'off', 'reset','kill', 'status'],
					   help='The command to issue to the device.\n If a device number is specified it will be issued to that particular device.\n If no device is specified only the status can be printed for all devices.')

# Execute the parse_args() method
args = my_parser.parse_args()

if args.devnum and args.action:	

	if (args.action == 'on'):
		power(args.devnum, True)

	if (args.action == 'off'):
		power(args.devnum, False)

	if (args.action == 'reset'):
		reset(args.devnum)

	if (args.action == 'kill'):
		kill(args.devnum)
		
	if (args.action == 'status'):
		status(args.devnum)

elif args.action and not args.devnum and args.action == 'status':
	i = 1
	while i < numDev:
		status(i)

else: 
	print('cannot perform an action without a valid device number specified')








#print(ser.name)
#ser.write(node2On.to_bytes(1, byteorder='big'))
#buf = ser.readline();
#print(buf)
#ser.close()


