// It is designed to work with the other example Feather9x_TX

#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h> 
#include <Wire.h>
#include "RTClib.h"

/* for feather32u4 
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
*/

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3


/* for shield 
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 7
*/


/* for ESP w/featherwing 
#define RFM95_CS  2    // "E"
#define RFM95_RST 16   // "D"
#define RFM95_INT 15   // "B"
*/

/* Feather 32u4 w/wing
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     2    // "SDA" (only SDA/SCL/RX/TX have IRQ!)
*/

/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/

/* Teensy 3.x w/wing 
#define RFM95_RST     9   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     4    // "C"
*/

// Changing frequency to 915.0Mhz or other frequency, must match TX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//Creating RTC objects
RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Blinky on receipt
#define LED 13

void setup() 
{
  pinMode(LED, OUTPUT);     
  pinMode(RFM95_RST, OUTPUT); //Configuraing Reset pin as output which is connected to LoRa RST pin.
  pinMode(A2, OUTPUT); //SD card Chip Select pin... I am used A2 pin.. you can use any available pin.. that need to be connected to SDcard CS pin

  digitalWrite(LED, LOW);
  Serial.begin(9600);// USB Serial Baudrate setting
  delay(100);


  // manual reset
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

void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      String dataString = "";      
      dataString += String((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      delay(10);
      
      // Send a reply
      delay(100); // may or may not be needed
      uint8_t data[] = "GPS data received";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(LED, LOW);

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


    dataString=timeStamp+dataString;// this is the string write's to SD card txt file


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
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}
