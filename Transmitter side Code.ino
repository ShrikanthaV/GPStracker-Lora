#include <Adafruit_SleepyDog.h> 
#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h> //for this to work you need to comment tc3_handler function in RH_ASK.cpp file.. this file usually in arduino librabry/RH_RF95 folder
#include <Wire.h>
#include "RTClib.h"
#include <TinyGPS.h>

//LoRa Module pin Mapping
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//LoRa Operating Frequency
#define RF95_FREQ 915.0

//Initializing Latitude & longitude variable
float lat = 0.00001,lon =0.00001; 

//Creating GPS, LoRa & RTC objects
TinyGPS gps;
RH_RF95 rf95(RFM95_CS, RFM95_INT);
RTC_PCF8523 rtc;


char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Blinky on receipt
#define LED 13


void setup() {
  Serial.begin(9600); // USB Serial Baudrate setting
  Serial1.begin(9600); // GPS serial line Baudrate setting

  
  
  pinMode(RFM95_RST, OUTPUT); //Configuraing Reset pin as output which is connected to LoRa RST pin.
  pinMode(A2, OUTPUT);//SD card Chip Select pin... I am used A2 pin.. you can use any available pin.. that need to be connected to SDcard CS pin
  
  
  // manual reset lora
  digitalWrite(RFM95_RST, HIGH);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  //Lora Module Initializing
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  while (!rf95.init()) {
  Serial.println("LoRa radio init failed");
  return; //if Lora not initialized the program stops...
  }
  Serial.println("LoRa radio init OK!");

  
  // Setting LoRa frequency
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    return; //if setFrequency failed then the programs stops 
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

//Starting RTC Module
 if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return;//if RTC initilized failed we can stop the program by using "return"... or ignore this failure and use gps time(UTC)
 }


 //initilizing RTC Module
 if (! rtc.initialized()) {
 Serial.println("RTC is NOT running!");
 // following line sets the RTC to the date & time this sketch was compiled
 rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 // This line sets the RTC with an explicit date & time, for example to set
 // January 21, 2014 at 3am you would call:
 // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
 }

  
 //Starting SDCard Module 
 if (!SD.begin(A2)) {
 Serial.println("Card failed, or not present");
 return;//it can be ignored because LoRa receiver log's GPS data...
  }
  Serial.println("card initialized."); 
  
  
} //end of void Setup

void loop() {

   while(Serial1.available()){
    if(gps.encode(Serial1.read()))// encode gps data
    {
      gps.f_get_position(&lat,&lon); // get latitude and longitude
      String latitude = String(lat,6); //float to string canversion with upto 6 decimal point
      String longitude = String(lon,6); //float to string canversion with upto 6 decimal point
      String packet = latitude+";"+longitude; //concate of latitude and longitude 
      
      char radiopacket[20];
      packet.toCharArray(radiopacket, 20);//String to Char convertion. result stored in radiopacket variable
      
      Serial.println("Sending gps data to rf95_server");
      Serial.println(radiopacket);      
      
      //Transmitting GPS lat & lon data
      uint8_t lora_packet[20];
      strcpy((char *)lora_packet, radiopacket);
      lora_packet[19]='\0';
      lora:rf95.send((uint8_t *)lora_packet, 20);
      Serial.println("Waiting for packet to complete..."); delay(10);
      rf95.waitPacketSent();
      
      
      // Now wait for a reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);

      Serial.println("Waiting for reply..."); delay(10);
      if (rf95.waitAvailableTimeout(1000))
      { 
        // Should be a reply message for us now   
        if (rf95.recv(buf, &len))
        {
          Serial.print("Got reply: ");
          Serial.println((char*)buf);
          // Serial.print("RSSI: ");
          // Serial.println(rf95.lastRssi(), DEC);    
         }
        else
        {
          Serial.println("Receive failed");  //optional implimentation(if resend_count !=0 jump to lora lable to resend GPS data and dec resend_count by 1)
        }
      }
      else
      {
        Serial.println("No reply, is there a listener around?"); // jump(using  go to )  to lora to resend packet & increment counter.. max retransimission is 3
      }

    //fie name
    String fileName = "DATE";

    //getting timestamp from rtc
     DateTime now = rtc.now();
     String timeStamp = "";      
    
    timeStamp += String(now.year());
    timeStamp += ".";
    timeStamp += String(now.month());
    timeStamp += ".";
    timeStamp += String(now.day());
    fileName += String(now.day()); // concat Present Date
    fileName += ".txt"; //formating file as text by adding .txt 
    timeStamp += String(" (");
    timeStamp += String(daysOfTheWeek[now.dayOfTheWeek()]);
    timeStamp += String(") ");
    timeStamp += String(now.hour());
    timeStamp += String(':');
    timeStamp += String(now.minute());
    timeStamp += String(':');
    timeStamp += String(now.second());
    timeStamp += String("   GPS info: ");

    String dataString="";
    dataString=timeStamp+packet;


    //Serial.println(fileName);
    //data logging
    File dataFile = SD.open(fileName, FILE_WRITE);
        
   // if the file is available, write to it:
   if (dataFile) {
   //dataFile.print(timeStamp);
   dataFile.println(dataString);
   dataFile.close();
  // print to the serial port too:
  Serial.print("Data updated to Memory card= ");
  Serial.println(dataString);
                }
  // if the file isn't open, pop up an error:
  else {
        Serial.println("error opening file");
        }

      Serial.println("Going 15sec sleep");
      Watchdog.sleep(15000);
     }

    }

}
