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


####
## used as global to force lower contrast when low Brightness (have a true black)
brightnessFx = video_effect("Brightness", midiMap.faders[7]   , "/enhancement/brightness")
contrastFx = video_effect("Constrate",  midiMap.faders[4]   , "/enhancement/contrast")


class Strobe:
    def __init__(self):
        self.isOn = 0
        self.lastT = 0
        self.value = 0
        self.period = 1
        self.alternate = 1
    def update(self):
        if self.isOn:
            cTime = time.time();
            if(cTime-self.lastT>self.period):
                self.lastT = cTime
                self.value = not self.value
                v = 1
                if not self.value:
                    v = 0
                lvalue = v
                if self.alternate :
                    lvalue = 0 if v else 127
                send_serial(20,lvalue)
                send_serial(21,v)

strobe = Strobe();
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
        for c in list_of_all['functions']:
            if (c.midiChannel == msg.control):
                        c.setValue(msg.value)




def update_serial( ser):
        result = ser.update()
        if result !=None:
            ids = ser.arduinoID
            if  not isinstance(ids,list):
                ids = [ids]
            #ser.printResult()
            for i in ids:
                send_serial( i, result)

def lowerContrastIfBrightnessDown(vidFx,result):
    global brightnessFx, contrastFx
    maxB = 31
    if(vidFx == brightnessFx and result < maxB):
        tv = contrastFx.getMappedValue()
        send_osc ( contrastFx.oscAddress, tv*result/maxB)

def update_videoFx( vidFx):
        result = vidFx.update()
        if result != None :
                
                lowerContrastIfBrightnessDown(vidFx,result)
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

def send_midiCC(cc,v,channel=None):
    port = mido.open_output('nanoKONTROL2 MIDI 1')
    
    if port:
        msg = mido.Message('control_change',channel=0, control=cc, value=v,time=0)
        # print("sending cc",msg)
        port.send(msg)
    else:
        print("can't send to midi")

def setMaskWhiteBG(s):
    send_osc("/shaders/Mask/whiteBG",s);
    return None

def main():
        

        global list_of_videoFx ,brightnessFx,contrastFx
        list_of_videoFx = []
        list_of_videoFx.append ( video_effect("sharpness",  midiMap.faders[5]   , "/enhancement/sharpness"))
        list_of_videoFx.append ( contrastFx)
        list_of_videoFx.append ( video_effect("Saturation", midiMap.faders[6]   , "/enhancement/saturation"))
        list_of_videoFx.append ( brightnessFx)

        list_of_videoFx.append ( video_effect("",midiMap.prevb      , "/shaders/kaleidoscope/enabled"))
        list_of_videoFx.append ( video_effect("",midiMap.nextb      , "/shaders/mirror/enabled"))
        list_of_videoFx.append ( video_effect("",midiMap.stopb      , "/shaders/pixelate/enabled"))
        list_of_videoFx.append ( video_effect("",midiMap.playb      , "/shaders/toon/enabled"))
        list_of_videoFx.append ( video_effect("",midiMap.recb       , "/shaders/Mask/enabled"))
        list_of_videoFx.append ( video_effect("",midiMap.markerl    , "/shaders/Mask/reset",lambda x:setMaskWhiteBG(1) or x or None))
        list_of_videoFx.append ( video_effect("",midiMap.markerr    , "/shaders/Mask/next",lambda x:  x or None))
        # list_of_videoFx.append ( video_effect("",midiMap.faders[0]  , "/shaders/Mask/transparency",lambda x:  x/127.0))

        list_of_videoFx.append ( video_effect("",midiMap.solos[0]  , "/enhancement/brightness",lambda x: 0 if x>0 else brightnessFx.currentValue ))
        # list_of_videoFx.append ( video_effect("",midiMap.mutes[0]  , "/shaders/Mask/setMaskIndex",lambda x: setMaskWhiteBG(0) or 16 if x>0 else None ))
        # list_of_videoFx.append ( video_effect("",midiMap.records[0], "/shaders/Mask/setMaskIndex",lambda x: setMaskWhiteBG(0) or 17 if x>0 else None ))

        list_of_videoFx.append ( video_effect("", midiMap.solos[7]      , "/omx/disableWhiteB"))
        list_of_videoFx.append ( video_effect("", midiMap.mutes[7]      , "/shaders/ShadowHighlights/enabled"))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[5]   , "/omx/colors/x",lambda x:(x+0.5)*4))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[6]   , "/omx/colors/y",lambda x:(x+0.5)*4))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[7]   , "/omx/colors/z",lambda x:(x+0.5)*4))
        
        # shader params
        def s_functor(i):
            def func(x):
                for ii in range(8): # clear others but us
                    send_midiCC(midiMap.records[ii],int(ii==i)*127)
                return i 
            return func;

        for i in range(3,8):
            list_of_videoFx.append ( video_effect("", midiMap.records[i], "/shaders/Mask/setMaskIndex",s_functor(i-3))) 

        list_of_videoFx.append ( video_effect("", midiMap.encoders[0] , "/shaders/kaleidoscope/scale",lambda x:[1.0+x/64.0 for _ in range(2)]))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[1] , "/shaders/kaleidoscope/offset/x",lambda x:x/64.0 - 0.5))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[2] , "/shaders/kaleidoscope/offset/y",lambda x:x/64.0 - 0.5))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[3] , "/shaders/kaleidoscope/vAngle",lambda x:(x/64.0 - 0.5)*.02))
        list_of_videoFx.append ( video_effect("", midiMap.solos[3]    , "/shaders/kaleidoscope/rotation",lambda x:0))
        list_of_videoFx.append ( video_effect("", midiMap.encoders[4]    , "/shaders/pixelate/size", lambda x:1+x))
        list_of_videoFx.append ( video_effect("", midiMap.solos[1]    , "/togglePause", lambda x:x))
        # list_of_videoFx.append ( video_effect("kal_rec", midiMap.setb , "/omx/rec"))

        global list_of_serial
        list_of_serial = []
        list_of_serial.append( serial_effect("ledR haut",       midiMap.solos  [2], 4     , False))
        list_of_serial.append( serial_effect("ledG haut",       midiMap.mutes  [2], 5     , False))
        list_of_serial.append( serial_effect("ledB haut",       midiMap.records[2], 6     , False))
        list_of_serial.append( serial_effect("ledPower haut",   midiMap.faders [2], 7     , False))
        list_of_serial.append( serial_effect("ledR cour+j",     midiMap.solos  [3], [0,8] , False))
        list_of_serial.append( serial_effect("ledG cour+j",     midiMap.mutes  [3], [1,9] , False))
        list_of_serial.append( serial_effect("ledB cour+j",     midiMap.records[3], [2,10], False))
        list_of_serial.append( serial_effect("ledPower cour+j", midiMap.faders [3], [3,11], False))
        lightCour = serial_effect("lightcour",       midiMap.trackl    , 20    ,True)
        lightJar  = serial_effect("lightjar",        midiMap.trackr    , 21    ,True)
        list_of_serial.append( lightCour)
        list_of_serial.append( lightJar)
        
        global list_of_functions
        list_of_functions =[]
        class midiF:
            def __init__(self,cc,fun):
                self.midiChannel = cc
                self.value = 0
                self.fun = fun
                self.changed = False
            def setValue(self,v):
                self.value = v
                self.changed = True
            def update_fun(self):
                if(self.changed):
                    self.fun(self.value)

        # def toggleStrobe(v):
        #     global strobe
        #     if v :
        #         strobe.isOn = not strobe.isOn

        

        def toggleLights(v):
            if(lightJar.currentValue==0 and lightCour.currentValue==0) :
                lightJar.currentValue = 127
                lightCour.currentValue = 127
            else :
                lightJar.currentValue = 0
                lightCour.currentValue = 0
            lightJar.isModified = True
            lightCour.isModified = True

        list_of_functions.append(midiF(midiMap.cycleb,toggleLights))
        
        global list_of_all 
        list_of_all = dict()

        list_of_all['videoFx'] = list_of_videoFx
        list_of_all['serial'] = list_of_serial
        list_of_all['functions'] = list_of_functions

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
                for c in list_of_all['functions']:
                    c.update_fun();
                strobe.update()
                #print ("Serial")
                #print (str(ser.readline()))
                          


if __name__ == "__main__":
    main()
