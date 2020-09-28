#!/usr/bin/python3
import os
import sys
import serial
import argparse
from time import sleep

# Serial Protocol Defintions
## Action codes
ACT_ON   = 0 # 0x000 Turn On
ACT_OFF  = 1 # 0x001 Turn Off
ACT_KILL = 2 # 0x010 Force Device Off (Hold Power btn)
ACT_RST  = 3 # 0x011 Reset Dev
ACT_STAT = 4 # 0x100 Query State of Specific Device
ACT_QRY  = 5 # 0x101 Query Count of Devices

## Codes returned by arduino in response to actions
STAT_OFF      = 0 #0x000
STAT_ON       = 1 #0x001
STAT_ON_FAIL  = 2 #0x010
STAT_OFF_FAIL = 3 #0x011
STAT_OK       = 4 #0x100
STAT_FAIL     = 5 #0x101
STAT_PARITY   = 6 #0x110
STAT_DATABAD  = 7 #0x111

serialPort = '/dev/ttyUSB0' # change as needed depending on system
numDev = 4 # load this value with numDevices queried from Arduino

ser = serial.Serial(serialPort, 9600, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=1)

def bitsauce(devId, act): #function that actual smushes our bitfields together as requested.
	sBit = 1 << 7
	actBits = act << 1
	devBits = devId << 4
	eBit = 1

	message = sBit | actBits | devBits | eBit

	print('Device ID: ' + str(devId))
	print('Action ID: ' + str(act))

	print('Sending: ' +bin(message) + ' to ' + ser.name)
	ser.write(message)
	buf = int.from_bytes(ser.read(1), 'big')
	ser.flush()
	print('Received: ' + bin(buf))

	rsBit = bool((buf & 0b10000000) >> 7)
	rdevBits = int((buf & 0b01110000) >> 4)
	rstatBits = int((buf & 0b00001110) >> 1)
	reBit = bool((buf & 0b00000001))
	


	return rstatBits # return the status code from the arduino


def power(devId, state): #set the requested power state
	if (state): # Power on the system
		if (status(devId) == STAT_OFF):
			result = bitsauce(devId, ACT_ON)
			if (result == STAT_ON):
				print('system on')
			elif (result == STAT_ON_FAIL):
				print('system failed to power on')
			else:
				print('unknown error occured trying to power on the system')
		else:
			print('system already on')
		
	else: #Power off the system
		if (status(devId) == STAT_ON):
			result = bitsauce(devId, ACT_OFF)
			if (result == STAT_ON):
				print('system off')
			elif (result == STAT_ON_FAIL):
				print('system failed to power off')
			else:
				print('unknown error occured trying to power off the system - try the \'kill\' action if neccessary')
		else:
			print('system already off')
	

def reset(devId):
	result = bitsauce(devId, ACT_RST)
	if (result == STAT_OK):
		print('system reset')

def kill(devId):
	result = bitsauce(devId, ACT_KILL)
	if (result == STAT_OFF):
		print('system off')
	elif (result == STAT_OFF_FAIL):
		print('system kill failed. system still on - something went horribly wrong!')
	else:
		print('unknown error occurred while trying to force off the system - check the arduino')

def status(devId):
	result = bitsauce(devId, ACT_STAT)
	if (result == STAT_OFF):
		print('system off')
	elif(result ==STAT_ON):
		print('system on')
	else:
		print('invalid status received from arduino - check the arduino') 

	return result
	

def query(): # the function to get number of devices on the system
	#bitsauce(0, ACT_QRY)
	print('device stat')


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
		for devid in args.devnum:
			power(devid, True)

	if (args.action == 'off'):
		for devnum in args.devnum:
			power(devnum, False)

	if (args.action == 'reset'):
		for devnum in args.devnum:
			reset(devnum)

	if (args.action == 'kill'):
		for devnum in args.devnum:
			kill(devnum)
		
	if (args.action == 'status'):
		for devnum in args.devnum:
			status(devnum)

elif args.action and not args.devnum and args.action == 'status':
	i = 0
	while i < numDev:
		status(i)
		sleep(0.1)
		i += 1

else: 
	print('cannot perform an action without a valid device number specified')

ser.close()