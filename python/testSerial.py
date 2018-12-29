import serial
import time

targetFPS = 20;

def send_serial( ard_id, val ):
        if True or val%targetFPS==0:
            print "Serial . id: "+str(ard_id)+" value: "+str(val)
        #msg = bytearray([ ard_id, val, 255])
        msg = str(chr(ard_id))+str(chr(val))+str(chr(255))
        ser.write(msg)         
                        
if __name__ == "__main__":
    ser = serial.Serial('/dev/ttyACM0',baudrate=115200, writeTimeout=0.0, exclusive=True )
    ser.close()
    ser.open()
    v= 0
    while (True):
      time.sleep(1.0/targetFPS)
      send_serial(20,v);
      v+=1
      v%=255
      
      
      
      
