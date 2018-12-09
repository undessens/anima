import mido
import serial
import os
import sys
import glob
import time
from channel import channel
from video_effect import video_effect
from transport_effect import transport_effect
from OSC import OSCClient, OSCMessage

########################################
# 3  types of command/class :
# .1 Channel :
#	Control arduino element from midi input, supposed to be analog ( led, motor speed, motor pos)
#	and able to record play sequence  
#.2 video effect :
#	Control video parameter, sending osc message from midi input. Can't be recorder
#.3 transport effect :
#	send arduino message, can't be recorder. Supposed to be switch ( On or Off)
########################################

########################################
# Korg nano control MEMO
# potar : 16, to 23
# fader : 0 to 7
########################################

#########################################
# Behavior about recording, playing ...
#	On record button pressed : is_recording is True
#							 every "channel" is begin to record
#							 once you move it. Stop record with stop
#							 or play. Each recording remove previous recording
#   On play button pressed : Each channel that has a recorded sequence is going 
#							 to play. If you move it manually while it is playing,
#							 then stop .
#   On Stop button pressed : Stop all recording, and all playing. Release motor
#
##########################################""






def receive_midi_msg(msg):
	global is_recording
	#global is_playing
	global time_without_midi

	time_without_midi = 0

	print msg
	for c in list_of_all['motor']:
		if (c.midiChannel == msg.control):
			c.setValue(msg.value)
			c.printResult()
			if( is_recording and not(c.isRecording)):
				c.playStop(False)
				c.startRecording()

	for c in list_of_all['led']:
		if (c.midiChannel == msg.control):
			c.setValue(msg.value)
			c.printResult()
			if( is_recording and not(c.isRecording)):
				c.playStop(False)
				c.startRecording()

	for c in list_of_all['videoFx']:
		if (c.midiChannel == msg.control):
			c.setValue(msg.value)
			c.printResult()

	for c in list_of_all['transport']:
		if (c.midiChannel == msg.control):
			c.setValue(msg.value)
			c.printResult()

	#record button
	if (msg.control == 45 and msg.value>0 ):
		
		if not(is_recording):
			start_recording_all()
			# do not record, until fader or potar is moved
			if is_playing:
				print "record overdub"
				#overdud
				send_osc("transport/overdub", 1)
			else :
				print "record only"
				#only recording
				send_osc("transport/recording", 1)



	#play button
	if msg.control == 41 and msg.value>0:
		print "play button"
		print is_recording
		if not is_playing :
			send_osc("transport/play", 1)

			if is_recording :
				print "play but is recording"
				stop_recording_all()
				play_all()
			else :
				print "play already recorded"
				play_all()
		if is_playing and is_recording:
			print "play after recording on playing"
			stop_recording_all()
			play_all()


	#stop button
	if msg.control == 42 and msg.value>0:
		print "stop button"
		send_osc("transport/stop", 1)
		if is_playing and not(is_recording):
			stop_all()
		if is_recording :
			stop_recording_all()
			stop_all()

	#clear button
	if msg.control == 46 and msg.value>0:
                print "clear button"
                clear_all()


def update_channel( channel):
	result = channel.update()
	if result != None  :
		#print result
		send_serial(result, channel.arduinoID)

def update_transport( transp):
	result = transp.update()
	if result :
		#print result
		send_serial(100, transp.arduinoID)

def update_videoFx( vidFx):
	result = vidFx.update()
	if result != None :
		#print result
		send_osc ( vidFx.oscAddress, result)


def send_serial(val, id):
	msg = ""
	msg = str(chr(id))+str(chr(val))
	if(ser):
		try:
			ser.write(msg)
		except Exception, e:
			print e

def send_osc(address, value):
	oscMsg = OSCMessage()
	oscMsg.setAddress(address)
	oscMsg.append(int(value))
	try:
		oscClient.send(oscMsg)
	except Exception, e:
		print e

def start_recording_all():
	#Nothing is going to be 
	#recorded, until its fader, potar 
	#is moved.
	global is_recording 

	print "record all"
	is_recording = True

	
def stop_recording_all():
	global is_recording
	
	print "stop recording all"
	is_recording = False
	for c in list_of_all['motor']:
			c.stopRecording()
	for c in list_of_all['led']:
			c.stopRecording()

def play_all():
	global is_playing
	print "play all"
	is_playing = True
	for c in list_of_all['motor']:
			c.playStop(True)
	for c in list_of_all['led']:
			c.playStop(True)

def stop_all():
	global is_playing
	print "stop all"
	is_playing = False
	for c in list_of_all['motor']:
			c.playStop(False)
	for c in list_of_all['led']:
			c.playStop(False)

def clear_all():
        global is_playing
	print "clear all"
	is_playing = False
	is_recording = False
	for c in list_of_all['motor']:
			c.clear()
	for c in list_of_all['led']:
			c.clear()
        



def main():
	
	#channel listing
	global list_of_motor
	list_of_motor= []
	list_of_motor.append( channel( "servomoteur3", 18, 6 , 33) )
	list_of_motor.append( channel( "servomoteur4", 19, 7 , 32) )
	list_of_motor.append( channel( "stepper motor1",22, 13, 34) )
	list_of_motor.append( channel( "stepper motor2",23, 14, 35) )
	#list_of_motor.append( channel( "channel4", 36, 4, 0) )
	
	global list_of_leds
	list_of_leds = []
	list_of_leds.append( channel( "ledRouge", 0, 2, 64))
	list_of_leds.append( channel( "ledRGBr", 1, 10, 65))
	list_of_leds.append( channel( "ledRGBg", 2, 11, 66))
	list_of_leds.append( channel( "ledRGBb", 3, 12, 67))
	#list_of_leds.append( channel( "ledSide2", 12, 34, 0))
	
	global list_of_videoFx 
	list_of_videoFx = []
	list_of_videoFx.append ( video_effect("sharpness", 5 , "enhancement/sharpness"))
	list_of_videoFx.append ( video_effect("Constrate", 4 , "enhancement/contrast"))
	list_of_videoFx.append ( video_effect("Saturation", 6, "enhancement/saturation"))
	list_of_videoFx.append ( video_effect("Brightness", 7, "enhancement/brightness"))
	list_of_videoFx.append ( video_effect("Filter +", 62, "filters/nextFilter"))
	list_of_videoFx.append ( video_effect("Filter -", 61, "filters/previousFilter"))
	list_of_videoFx.append ( video_effect("init Filter", 60, "filters/initFilter"))
	list_of_videoFx.append ( video_effect("zoom", 20, "zoomCrop/topMargin"))
	list_of_videoFx.append ( video_effect("zoom", 21, "zoomCrop/leftMargin"))
	list_of_videoFx.append ( video_effect("zoom", 0, "zoomCrop/zoomLevel"))
	list_of_videoFx.append ( video_effect("zoom", 59, "whiteBalance/wbNext"))
	list_of_videoFx.append ( video_effect("zoom", 58, "whiteBalance/wbPrev"))
	
	global list_of_transport
	list_of_transport = []
	list_of_transport.append( transport_effect("release Stepper", 42, 21))
        list_of_transport.append( transport_effect("relay1", 32, 4))
        list_of_transport.append( transport_effect("release Stepper", 33, 5))
	#list_of_transport.append( transport_effect("hold Stepper", 41, 20))
	#list_of_transport.append( transport_effect("init stepper pos", 43, 22))
	
	global list_of_all 
	list_of_all = dict()
	list_of_all['motor'] = list_of_motor
	list_of_all['led'] = list_of_leds
	list_of_all['videoFx'] = list_of_videoFx
	list_of_all['transport'] = list_of_transport

	#intern transport : record, play , stop , reset
	global is_recording
	is_recording = False
	global is_playing
	is_playing = False

	#Heating security : realase motor after inactivity
	global time_without_midi
	time_without_midi = 0




	# OSC connect
	global oscClient
	oscClient = OSCClient()
	oscClient.connect( ("localhost",12345 ))


	
	# Midi connect						
	try:
		if sys.platform.startswith('darwin'):
			inport = mido.open_input('nanoKONTROL2 SLIDER/KNOB')
		elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
			inport = mido.open_input('nanoKONTROL2 MIDI 1')
		else:
			inport = None
	except :
		print "Impossible to connect to Midi device"
		inport = None	

	if(inport):
		inport.callback = receive_midi_msg

	#Serial connect
	try:
		global ser
		if sys.platform.startswith('darwin'):
			ser = serial.Serial('/dev/tty.usbmodem1a151',115200)
			print "Serial connected"
		elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
			try:
				ser = serial.Serial('/dev/ttyACM0',115200)
				print "ACM0"
			except :
				ser = serial.Serial('/dev/ttyACM1', 115200)
				print "ACM1"
			
		else:
			ser = None
	except :
		print "Impossible to connect to Serial"
		ser = None	


	while True:
		#Main update
		#Frequency update 20Hz
		time.sleep(0.05)

		for c in list_of_all['motor']:
			update_channel(c)
		for c in list_of_all['led']:
			update_channel(c)
		for c in list_of_all['videoFx']:
			update_videoFx(c)
		for c in list_of_all['transport']:
			update_transport(c)

		#Heating security, realese stepper after 5sec of inactivity
		time_without_midi = time_without_midi+1
		if(time_without_midi> 100):
			#release stepper motor
			send_serial(100, 21)
			print "Surchauffe"
			time_without_midi = 0





if __name__ == "__main__":
    main()
