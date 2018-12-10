/* 
  Anima
  Video machine, using raspberry pi
  http://github.com/undessens/anima
  http://assoundessens.fr
  
*/

#include <Servo.h>
byte buffer[32];
int bufferindex = 0;

void setup() {

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {

  //RECEIVE
  while (Serial.available() > 0) {
    byte b = Serial.read();
    if ( b == 255) {
      processBuffer();
      bufferindex = 0;
    }
    else {
      if (bufferindex < 32) {
        buffer[bufferindex] = b;
        bufferindex ++;
      }

    }
  }


}

void processBuffer() {

  byte cmd = buffer[0];
  switch (cmd) {
    case 0:
      //ledR jardin 
      break;
    case 1:
      //ledG jardin 
      break;
    case 2:
      //ledB jardin 
      break;
   case 3:
      //ledPower jardin 
      break;
   case 4:
      //ledR haut 
      break;
    case 5:
      //ledG haut 
      break;
    case 6:
      //ledB haut 
      break;
   case 7:
      //ledPower haut 
      break;
   case 8:
      //ledR cour 
      break;
    case 9:
      //ledG cour 
      break;
    case 10:
      //ledB cour 
      break;
   case 11:
      //ledPower cour
      break;
   case 20:
      //relay1 
      digitalWrite(LED_BUILTIN, buffer[1]);
      break;
   case 21:
      //realy2
      break;
      


  }
}


