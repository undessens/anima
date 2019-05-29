/* 
  Anima
  Video machine, using raspberry pi
  http://github.com/undessens/anima
  http://assoundessens.fr
  
*/

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PINSTRIP 7
#define PINRELAY1 8
#define PINRELAY2 9

//BUFFER SERIAL
byte buffer[32];
int bufferindex = 0;

//Relay
boolean relay1 = false;
boolean relay2 = false;


//STRIP LED
int r_left = 0;
int g_left = 0;
int b_left = 0;
int power_left = 70; // 0 to 127 going from 0 to maxintensity
int r_top = 0;
int g_top = 0;
int b_top = 0;
int power_top = 127;
int r_right = 0;
int g_right = 0;
int b_right = 0;
int power_right = 30;
int strip_zone1 = 18;
int strip_zone2 =41;
int strip_maxled = 60;
int strip_maxintensity = 15;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(strip_maxled, PINSTRIP, NEO_GRB + NEO_KHZ800);


void setup() {

  Serial.begin(115200);

  //Pinmode
  pinMode(PINRELAY1, OUTPUT);
  pinMode(PINRELAY2, OUTPUT);
  digitalWrite(PINRELAY1, relay1);
  digitalWrite(PINRELAY2, relay2);

  
  strip.begin();

  for(int i=0;i<strip_maxled;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    strip.setPixelColor(i, strip.Color(0,0,0)); // Moderately bright green color.

  }
  strip.show();
  delay(500);

 
  
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


  //Serial.println("process buffer");


  byte cmd = buffer[0];
  
  switch (cmd) {
    case 0:
      //ledR jardin 
      r_left = buffer[1]/127;
      strip_update_zone(1);
      break;
    case 1:
       g_left = buffer[1]/127;
       strip_update_zone(1);
      break;
    case 2:
       b_left = buffer[1]/127;
       strip_update_zone(1);
      break;
   case 3:
      power_left = buffer[1];
      strip_update_zone(1);
      break;
   case 4:
       r_top = buffer[1]/127; 
       strip_update_zone(2);
      break;
    case 5:
      g_top = buffer[1]/127;
      strip_update_zone(2); 
      break;
    case 6:
      b_top = buffer[1]/127;
      strip_update_zone(2);
      break;
   case 7:
      power_top = buffer[1];
      strip_update_zone(2); 
      break;
   case 8:
      r_right = buffer[1]/127;
      strip_update_zone(3); 
      break;
    case 9:
      g_right = buffer[1]/127;
      strip_update_zone(3);  
      break;
    case 10:
      b_right = buffer[1]/127;
      strip_update_zone(3);  
      break;
   case 11:
      power_right = buffer[1];
      strip_update_zone(3); 
      break;
   case 20:
      //relay
      boolean state1;
      state1= (buffer[1]==0);
      digitalWrite( PINRELAY1, state1 );
      break;
   case 21:
      //realy2
      boolean state2 ;
      state2 = (buffer[1]==0);
      digitalWrite( PINRELAY2, state2 );
      break;
    default:
      break;


  }
}

void strip_update_zone(int z){
int finalpower;

switch(z){
  case 1://ZONE 1 :
    finalpower = map(power_left, 0, 127, 0, strip_maxintensity);
    for( int i = strip_zone2; i<strip_maxled; i++){ 
    
      strip.setPixelColor(i, strip.Color( r_left*finalpower,g_left*finalpower,b_left*finalpower));
    }
  break;
  
  case 2://ZONE 2:
    finalpower = map(power_top, 0, 127, 0, strip_maxintensity);
    for( int i = strip_zone1; i<strip_zone2; i++){
    
      strip.setPixelColor(i, strip.Color( r_top*finalpower,g_top*finalpower,b_top*finalpower));
    }
  break;
  
  
  case 3://ZONE 3:
    finalpower = map(power_right, 0, 127, 0, strip_maxintensity);
    for( int i = 0; i<strip_zone1; i++){  
    
      strip.setPixelColor(i, strip.Color( r_right*finalpower,g_right*finalpower,b_right*finalpower));
    }    
   break;

}//end of switch

strip.show();

}



