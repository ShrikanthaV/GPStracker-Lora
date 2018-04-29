# GPStracker-Lora
collar tracker prototype using feather M0 LoRa module

Transmitter Module Consist of...
  1. Feather M0 with LoRa Radio Module
  2. Ultimate GPS Module (TX & RX connected to Feather Board tx, rx pin. i.e. Serial1).... 
  3. Adalogger Featherwing (SDCrad CS connected to Feather Board A2 pin & SD card formatted to FAT16)
  
 Reciver Module Consist of...
  1. Feather M0 with LoRa Radio Module
  2. Adalogger Featherwing (SDCrad CS connected to Feather Board A2 pin & SD card formatted to FAT16)
  
  
By Above Module setup And Transmitter Side & receiver Side Code.

The Transmitter module can able to get the current GPS location and transmits to Another LoRa module as
well as stores that information to SD card with Timestamp. GPS data sent every 15sec intervel

The Receiver Module can able to get the transmitted info from transmitter and send's Acknowledgment message and strores the received info to SD card with current timestamp
 
