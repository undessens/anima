import mido
import serial
import os
import sys
import glob
import time
import struct
from video_effect import video_effect
from serial_effect import serial_effect
from OSC import OSCClient, OSCMessage

########################################
# 2 types of command/class :
#.1 video effect :
#       Control video parameter, sending osc message from midi input. Can't be recorder
#.2 serial effect :
#       send arduino message, can't be recorder. Supposed to be switch ( On or Off) or analog
########################################

########################################
# Korg nano control MEMO
# potar : 16, to 23
# fader : 0 to 7
########################################




def receive_midi_msg(msg):


        #print (msg)

        for c in list_of_all['videoFx']:
                if (c.midiChannel == msg.control):
                        c.setValue(msg.value)
                        #c.printResult()

        for c in list_of_all['serial']:
                if (c.midiChannel == msg.control):
                        c.setValue(msg.value)
                        #c.printResult()




def update_serial( ser):
        result = ser.update()
        if result !=None:
                #ser.printResult()
                send_serial( ser.arduinoID, result)

def update_videoFx( vidFx):
        result = vidFx.update()
        if result != None :
                #vidFx.printResult()
                send_osc ( vidFx.oscAddress, result)


def send_serial( ard_id, val ):
        print "Serial . id: "+str(ard_id)+" value: "+str(val)
        #msg = bytearray([ ard_id, val, 255])
        msg = str(chr(ard_id))+str(chr(val))+str(chr(255))
        if(ser):
                try:
                        ser.write(msg)

                        
                except Exception, e:
                        print e

def send_osc(address, value):
        print "OSC addresse: "+str(address)+" value: "+str(value)
        oscMsg = OSCMessage()
        oscMsg.setAddress(address)
        oscMsg.append(int(value))
        try:
                oscClient.send(oscMsg)
        except Exception, e:
                print e


def main():
        

        global list_of_videoFx 
        list_of_videoFx = []
        list_of_videoFx.append ( video_effect("sharpness", 4 , "/enhancement/sharpness"))
        list_of_videoFx.append ( video_effect("Constrate", 6 , "/enhancement/contrast"))
        list_of_videoFx.append ( video_effect("Saturation", 5, "/enhancement/saturation"))
        list_of_videoFx.append ( video_effect("Brightness",7, "/enhancement/brightness"))
        list_of_videoFx.append ( video_effect("Filter +", 62, "/filters/nextFilter"))
        list_of_videoFx.append ( video_effect("Filter -", 61, "/filters/previousFilter"))
        list_of_videoFx.append ( video_effect("init Filter", 60, "/filters/initFilter"))
        #list_of_videoFx.append ( video_effect("zoom", 20, "/zoomCrop/topMargin"))
        #list_of_videoFx.append ( video_effect("zoom", 21, "/zoomCrop/leftMargin"))
        #list_of_videoFx.append ( video_effect("zoom", 0, "/zoomCrop/zoomLevel"))
        list_of_videoFx.append ( video_effect("zoom", 59, "/whiteBalance/wbNext"))
        list_of_videoFx.append ( video_effect("zoom", 58, "/whiteBalance/wbPrev"))
        
        global list_of_serial
        list_of_serial = []
        list_of_serial.append( serial_effect("ledR jardin", 32, 0, False))
        list_of_serial.append( serial_effect("ledG jardin", 48, 1, False))
        list_of_serial.append( serial_effect("ledB jardin", 64, 2 , False))
        list_of_serial.append( serial_effect("ledPower jardin", 0, 3, False))
        list_of_serial.append( serial_effect("ledR haut", 33, 4, False))
        list_of_serial.append( serial_effect("ledG haut", 49, 5, False))
        list_of_serial.append( serial_effect("ledB haut", 65, 6, False))
        list_of_serial.append( serial_effect("ledPower haut", 1, 7, False))
        list_of_serial.append( serial_effect("ledR cour", 34, 8,False))
        list_of_serial.append( serial_effect("ledG cour", 50, 9, False))
        list_of_serial.append( serial_effect("ledB cour", 66, 10 , False))
        list_of_serial.append( serial_effect("ledPower cour", 2, 11, False))
        list_of_serial.append( serial_effect("relay1", 35, 20, True))
        list_of_serial.append( serial_effect("relay2", 36, 21, True))
	list_of_serial.append( serial_effect("rien", 3, 30, False))
        
        global list_of_all 
        list_of_all = dict()

        list_of_all['videoFx'] = list_of_videoFx
        list_of_all['serial'] = list_of_serial

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
                        ser = serial.Serial('/dev/cu.usbmodem1421',115200)
                        print "Serial connected"
                elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
                        try:
                                ser = serial.Serial('/dev/ttyACM0',baudrate=115200 )
                                print "ACM0"
                        except :
                                ser.close()
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


                for c in list_of_all['videoFx']:
                        update_videoFx(c)
                for c in list_of_all['serial']:
                        update_serial(c)

                #print "Serial"
                #print str(ser.readline())
                          


if __name__ == "__main__":
    main()
