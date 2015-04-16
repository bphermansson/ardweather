/*
Program for receiving and decoding signals from 433.92 MHz AM ASK radio receiver module.
Send <House> <Channel> and you get values from the corresponding 
Esic-transmitter. Send 7 and you get the air pressure.

Uses input capture interrupt for reception.
Decodes signals from TFA brand wireless thermometer and Esic brand wireless thermometer/hygrometer.

Based on http://elektronikforumet.com/forum/viewtopic.php?f=2&t=41394
and http://ala-paavola.fi/jaakko/doku.php?id=wireless

RF-receiver (https://www.sparkfun.com/products/10532 or similar)
connected to D8.
Air pressure sensor MPXA4115A connected to A1.
*/

#include <avr/interrupt.h>
#include "rfrx.h"

#define output Serial
#define outBps 19200
#define SENSOR_COUNT 4
#define NETWORK_COUNT 2
//#define debugEsic 

// Global RF-sensor data
char houseChar[5];
char channelChar[5];
char lobatchar[5];

struct sensor {
  int           humidity;
  float         temp;
  float         temp_min;
  float         temp_max;
  boolean       lobatt;
};
struct sensor measurement[NETWORK_COUNT][SENSOR_COUNT];

// Flag for new rf-data
int intNewData;
    
// Serial comms
int inByte = 0;    // incoming serial byte
char buffer[2];    // Read two chars on serial bus
int received;
    
boolean pulseInt(uint16_t dur, uint16_t intLow, uint16_t intHigh) {
  return(intLow <= dur && dur <= intHigh);
}

int presAPin = A1;    // Set input pin for the MPXA4115
float pres = 0;       // variable to store the value coming from the sensor

void hexPrint(uint32_t number, uint8_t pos) {
  uint8_t i;

  for(i=0; i<(pos - 1); i++) {
    if(number < (1 << ((pos - 1 - i) << 2))) {
      output.print("0");
    }
  }
  output.print(number, HEX);
}

void setup(void) {
  output.begin(outBps);
  icp_init();
  output.println("Started...");
  received = 0;
  buffer[received] = '\0';
}

void loop(void) {
  rxDataStruct currentPulse;

  if (Serial.available() > 0) {
      int house = Serial.parseInt();
      int channel = Serial.parseInt();    
    
      /* An incoming digit on serial port selects which
      sensor to display, 'house' and 'channel' for Esic sensors
        
      Digit 1-4 is wireless sensors, Esic WT450(H)
      House '7' is a pressure sensor 
      */
                  
        if (house == 7 ) {
            
          // Show air pressure
          float P = (analogRead(0)/1024.0 + 0.095 ) / 0.0009;
          int Pi = round(P);
          output.print(Pi);          
            
        }
        
        else {
          /*
            Debug
            output.print("House:");
            output.print(house);
            output.print(";");
            output.print("Channel:");
            output.print(channel);
            output.print(";");
          */  
            output.print(measurement[house][channel].temp/10);
            output.print(";");
            output.print(measurement[house][channel].humidity/10);
            output.print(";");
            output.println(measurement[house][channel].lobatt);
        }
    Serial.flush();
  }

  if(icpAvail()) {
    currentPulse = icpRead();
    decodeEsic(currentPulse);
        
   #if defined(debugEsic)
     
       if (intNewData) {
           output.print("Temp: ");
           output.print(measurement[atoi(houseChar)][atoi(channelChar)].temp);
           output.print(":");          
           output.print("Humidity: ");
           output.println(measurement[atoi(houseChar)][atoi(channelChar)].humidity);
           output.print("House:"); 
           output.println(houseChar);  
           output.print("Channel:"); 
           output.println(channelChar);        
           output.print("Low Battery: "); 
           output.println(measurement[atoi(houseChar)][atoi(channelChar)].lobatt);   
           output.print(";");
           intNewData=0;
        }
    #endif
  }
}
