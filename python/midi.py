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
import nanoKontrolMap as midiMap

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


        # print (msg)

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
        print ("Serial . id: "+str(ard_id)+" value: "+str(val))
        #msg = bytearray([ ard_id, val, 255])
        msg = str(chr(ard_id))+str(chr(val))+str(chr(255))
        if(ser):
                try:
                        ser.write(msg)

                        
                except Exception as e:
                        print (e)

def send_osc(address, value):
        print ("OSC addresse: "+str(address)+" value: "+str(value))
        oscMsg = OSCMessage()
        oscMsg.setAddress(address)
        oscMsg.append(value)
        try:
                oscClient.send(oscMsg)
        except Exception as e:
                print (e)


def main():
        

        global list_of_videoFx 
        list_of_videoFx = []
        list_of_videoFx.append ( video_effect("sharpness",  midiMap.faders[5]   , "/enhancement/sharpness"))
        list_of_videoFx.append ( video_effect("Constrate",  midiMap.faders[4]   , "/enhancement/contrast"))
        list_of_videoFx.append ( video_effect("Saturation", midiMap.faders[6]   , "/enhancement/saturation"))
        list_of_videoFx.append ( video_effect("Brightness", midiMap.faders[7]   , "/enhancement/brightness"))
        list_of_videoFx.append ( video_effect("Filter +",   62                  , "/filters/nextFilter"))
        list_of_videoFx.append ( video_effect("Filter -",   61                  , "/filters/previousFilter"))
        list_of_videoFx.append ( video_effect("init Filter",60                  , "/filters/initFilter"))
        #list_of_videoFx.append ( video_effect("zoom", 20, "/zoomCrop/topMargin"))
        #list_of_videoFx.append ( video_effect("zoom", 21, "/zoomCrop/leftMargin"))
        #list_of_videoFx.append ( video_effect("zoom", 0, "/zoomCrop/zoomLevel"))
        list_of_videoFx.append ( video_effect("zoom", 59                        , "/whiteBalance/wbNext"))
        list_of_videoFx.append ( video_effect("zoom", 58                        , "/whiteBalance/wbPrev"))

        list_of_videoFx.append ( video_effect("kal_enable", midiMap.prevb       , "/shaders/kaleidoscope/enabled"))
        list_of_videoFx.append ( video_effect("mirror_enable", midiMap.nextb    , "/shaders/mirror/enabled"))
        list_of_videoFx.append ( video_effect("bord_enable", midiMap.stopb       , "/shaders/borders/enabled"))
        list_of_videoFx.append ( video_effect("toon_enable", midiMap.playb       , "/shaders/toon/enabled"))
        list_of_videoFx.append ( video_effect("curve_enable", midiMap.recb      , "/shaders/ShadowHighlights/enabled"))

        list_of_videoFx.append ( video_effect("whiteB", midiMap.solos[7]       , "/omx/disableWhiteB"))
        list_of_videoFx.append ( video_effect("lowR", midiMap.encoders[5]       , "/omx/colors/x",lambda x:(x+0.5)*4))
        list_of_videoFx.append ( video_effect("lowG", midiMap.encoders[6]       , "/omx/colors/y",lambda x:(x+0.5)*4))
        list_of_videoFx.append ( video_effect("lowB", midiMap.encoders[7]       , "/omx/colors/z",lambda x:(x+0.5)*4))
        
        global list_of_serial
        list_of_serial = []
        list_of_serial.append( serial_effect("ledR jardin",     midiMap.solos  [0], 0 , False))
        list_of_serial.append( serial_effect("ledG jardin",     midiMap.mutes  [0], 1 , False))
        list_of_serial.append( serial_effect("ledB jardin",     midiMap.records[0], 2 , False))
        list_of_serial.append( serial_effect("ledPower jardin", midiMap.faders [0], 3 , False))
        list_of_serial.append( serial_effect("ledR haut",       midiMap.solos  [1], 4 , False))
        list_of_serial.append( serial_effect("ledG haut",       midiMap.mutes  [1], 5 , False))
        list_of_serial.append( serial_effect("ledB haut",       midiMap.records[1], 6 , False))
        list_of_serial.append( serial_effect("ledPower haut",   midiMap.faders [1], 7 , False))
        list_of_serial.append( serial_effect("ledR cour",       midiMap.solos  [2], 8 , False))
        list_of_serial.append( serial_effect("ledG cour",       midiMap.mutes  [2], 9 , False))
        list_of_serial.append( serial_effect("ledB cour",       midiMap.records[2], 10, False))
        list_of_serial.append( serial_effect("ledPower cour",   midiMap.faders [2], 11, False))
        list_of_serial.append( serial_effect("relay1",          35,                 20, False))
        list_of_serial.append( serial_effect("relay2",          36,                 21, False))
        list_of_serial.append( serial_effect("rien",            3,                  30, False))
        
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
                print ("Impossible to connect to Midi device")
                inport = None   

        if(inport):
                inport.callback = receive_midi_msg

        #Serial connect
        try:
                global ser
                baudrate = 115200
                if sys.platform.startswith('darwin'):
                        ser = serial.Serial('/dev/cu.usbmodem1421',baudrate)
                        print ("Serial connected")
                elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
                        try:
                                ser = serial.Serial('/dev/ttyACM0',baudrate)
                                print ("ACM0")
                        except :
                                ser = serial.Serial('/dev/ttyACM1', baudrate)
                                print ("ACM1")
                        
                else:
                        ser = None
        except :
                print ("Impossible to connect to Serial")
                ser = None      


        while True:
                #Main update
                #Frequency update 20Hz
                time.sleep(0.05)


                for c in list_of_all['videoFx']:
                        update_videoFx(c)
                for c in list_of_all['serial']:
                        update_serial(c)

                #print ("Serial")
                #print (str(ser.readline()))
                          


if __name__ == "__main__":
    main()
