# serpy-ardweather
# Connects to an Ardweather Arduino and collects data
# Also sends the data to an Emoncms-server for storage

#Needs Python serial, http://pyserial.sourceforge.net
# "apt-get install python-serial"

import time
import serial
import sys, string
import httplib

# For Emoncms, inspired by https://github.com/openenergymonitor/EmoncmsPythonLink/blob/master/pylink.py
# Local Emoncms
domain = "localhost"
emoncmspath = "emoncms"
apikey = "b0660d27cc88ee1a916b7bbb4b9f8baf"
nodeid = 1
conn = httplib.HTTPConnection(domain)

# Remote Emoncms
remotedomain="emoncms.org"
remoteemoncmspath = "emoncms"
remoteapikey = "bdd96271de3ff345c087afa276d1a619"
remotenodeid = 1
remoteconn = httplib.HTTPConnection(remotedomain)

# This depends on the serial adapter used
#ardPort="/dev/ttyUSB0"
#ardPort="/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0"
#ardPort="/dev/serial/by-id/usb-FTDI_TTL232R_FTCW2AXL-if00-port0"
ardPort="/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0"

# Check no of arguments
if len(sys.argv) < 2:
	print 'Usage error. Use <command> <Esic House> <Esic Channel> or <command> 7 for air pressure'
	exit()
	
# Get house code
house = sys.argv[1]
# If house = 7 then we're reading the humidity sensor and no channel is given at the command line 
if house != '7':
	cnl = sys.argv[2]
else:
	cnl = 0

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
	port=ardPort,
	baudrate=19200,
	timeout=5
)

ser.open()
ser.isOpen()
time.sleep(1)

#print "Port used: "+ser.portstr       # check which port was really used
#print "Ok lets go"
#print "House & channel: " + (str(house) +":"+ str(cnl))

ser.write (str(house) +" "+ str(cnl))

# Read serial port
#status = ser.readline()
status = ser.read(40)
#print "Read:"
print(status)

# Split read values 
parts = status.split(';')
# Debug: 
#print parts[0]
#print parts[1]
#print parts[2]

#Send to Emoncms
# Set nodeid to the current channel
nodeid = cnl
# Create payload from read values
csv = ",".join(parts)

epoch_time = int(time.time())

# Send values to local Emoncms
#conn.set_debuglevel(3)
conn.request("GET", "/"+emoncmspath+"/input/post.json?time="+str(epoch_time)+"&apikey="+apikey+"&node="+str(nodeid)+"&csv="+csv)
response = conn.getresponse()
print response.read()

# Send values to Emoncms.org
#remoteconn.set_debuglevel(3)
remoteconn.request("GET", "/"+remoteemoncmspath+"/input/post.json?time="+str(epoch_time)+"&apikey="+remoteapikey+"&node="+str(cnl)+"&csv="+csv)
response = remoteconn.getresponse()
print response.read()
#

# Clean up
del status
exit()


