#!/usr/bin/python3
import os
import sys
import serial
import termios
import argparse
from time import sleep

DEBUG = False

# Serial Protocol Defintions
## Action codes
ACT_OFF  = 0 # 0x000 Turn Off
ACT_ON   = 1 # 0x001 Turn On
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
numDev = 0 # load this value with numDevices queried from Arduino

## Disgusting workaround to avoid resetting the RTS line
f = open(serialPort)
attrs = termios.tcgetattr(f)
attrs[2] = attrs[2] & ~termios.HUPCL
termios.tcsetattr(f, termios.TCSAFLUSH, attrs)
f.close()

ser = serial.Serial()
ser.baudrate = 115200
ser.port = serialPort
if DEBUG:
	print ('dtr =', ser.dtr)
ser.open()

def recv(ser, x):
  data = ser.read(1)
  return data+ser.read(min(x-1, ser.in_waiting))

def bitsauce(devId, act): #function that actual smushes our bitfields together as requested.
	#print('devid: '+ str(type(devId)))
	#print('act: '+ str(type(act)))

	sBit = 1 << 7
	devBits = devId << 4
	actBits = act << 1
	eBit = 1
	rstatBits = 0

	message = (sBit | devBits | actBits | eBit).to_bytes(1, 'little')

	if DEBUG:
		print('Device ID: ' + str(devId))
		print('Action ID: ' + str(act))
		print('Sending: ' + bin(ord(message)) + ' to ' + ser.name)
	
	ser.write(message)
	buf = ser.read(1)
	ser.flush()

	if DEBUG:
		print(buf.hex())
	
	intByte = int.from_bytes(buf, 'little')

	rsBit = bool((intByte & 0b10000000) >> 7)
	rdevBits = int((intByte & 0b01110000) >> 4)
	rstatBits = int((intByte & 0b00001110) >> 1)
	reBit = bool((intByte & 0b00000001))

	if DEBUG:
		print('Response: ' + bin(intByte) + '\n\tStart Bit: ' + str(rsBit) + '\n\tDevice ID: ' + str(rdevBits) + '\n\tStatus Code: ' + str(rstatBits) + '\n\tEnd Bit: ' + str(reBit))

	return rstatBits # return the status code from the arduino


def power(devId, state): #set the requested power state
	if (state): # Power on the system
		if (status(devId) == STAT_OFF):
			result = bitsauce(devId, ACT_ON)
			if (result == STAT_ON):
				print('system ' + str(devId) + ': on')
			elif (result == STAT_ON_FAIL):
				print('system ' + str(devId) + ': failed to power on')
			else:
				print('unknown error occured trying to power on the system')
		else:
			print('system ' + str(devId) + ': already on')
		
	else: #Power off the system
		if (status(devId) == STAT_ON):
			result = bitsauce(devId, ACT_OFF)
			if (result == STAT_OFF):
				print('system ' + str(devId) + ': off')
			elif (result == STAT_OFF_FAIL):
				print('system ' + str(devId) + ': failed to power off')
			else:
				print('unknown error occured trying to power off the system - try the \'kill\' action if neccessary')
		else:
			print('system ' + str(devId) + ': already off')
	

def reset(devId):
	result = bitsauce(devId, ACT_RST)
	if (result == STAT_OK):
		print('system ' + str(devId) + ': reset')

def kill(devId):
	result = bitsauce(devId, ACT_KILL)
	if (result == STAT_OFF):
		print('system ' + str(devId) + ': off')
	elif (result == STAT_OFF_FAIL):
		print('system ' + str(devId) + ': kill failed. system still on - something went horribly wrong!')
	else:
		print('unknown error occurred while trying to force off the system - check the arduino')

def status(devId):
	result = bitsauce(devId, ACT_STAT)
	if (result == STAT_OFF):
		print('system ' + str(devId) + ': off')
	elif(result == STAT_ON):
		print('system ' + str(devId) + ': on')
	else:
		print('invalid status received from arduino - check the arduino') 

	return result
	

def query(): # the function to get number of devices on the system
	count = bitsauce(0, ACT_QRY)
	if (count > 0):
		print('numder of devices: ' + str(count))
		return count
	else:
		print('no devices to control')
		exit()


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
	numDev = query()

	if (args.action == 'on'):
		for devid in args.devnum:
			power(devid, True)

	if (args.action == 'off'):
		for devid in args.devnum:
			power(devid, False)

	if (args.action == 'reset'):
		for devid in args.devnum:
			reset(devid)

	if (args.action == 'kill'):
		for devid in args.devnum:
			kill(devid)
		
	if (args.action == 'status'):
		for devid in args.devnum:
			status(devid)

elif args.action and not args.devnum and args.action == 'status':
	numDev = query()
	i = 0
	while i < numDev:
		status(i)
		#sleep(0.05)
		i += 1

else: 
	print('cannot perform an action without a valid device number specified')

ser.close()