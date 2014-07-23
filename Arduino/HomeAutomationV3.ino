#include <homeGW.h>
#include <weather.h>
#include <door.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
#include <GreenHouseProtocol.h>

#include <Wire.h>
#include <Adafruit_BMP085.h> //https://github.com/adafruit/Adafruit-BMP085-Library

#include <RemoteTransmitter.h> //https://bitbucket.org/fuzzillogic/433mhzforarduino

#define DEBUG
#define RF_RECEIVER_PIN 2 // pin 3 on the Uno
#define RF_TRANSMITTER_PIN 9  //pin 5 on the Uno 
#define LM35sensorPin 0  //A0
#define RF_PERIOD 480 //usecs (as detected for Chacon Ref:54656 using a 434mhz receiver and fuzzillogic example code)
#define WEATHER_PERIOD 30000

homeGW gw;
weather station(0x3F);
door front(0x7F7B04);
Adafruit_BMP085 bmp;
boolean bmp_present;
unsigned long time;

#ifdef GREENHOUSE
struct GHDataPacket p;
volatile boolean interrupt_flag = false;
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9,10);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
#endif

String cmd = "";         // a string to hold incoming data
volatile boolean command_flag = false;

#ifdef GREENHOUSE
void check_radio(void)
{
  // What happened?
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);

  if (rx) {
    interrupt_flag = true;
    // We got something on the radio!
    uint8_t len = radio.getDynamicPayloadSize();
    radio.read(&p, len );
  } else if (fail) {
    interrupt_flag = true;
    Serial.print("{\"id\": \"weather\", \"code\": 400}");
  }
}
#endif

void setup()
{
  Serial.begin( 115200 );   //using the serial port at 115200bps for debugging and logging
  printf_begin();

  if (!bmp.begin()) {
    Serial.print("{\"id\": \"bmp\", \"code\": 400}");
    bmp_present = false;
  } else {
    bmp_present = true;
  }

  pinMode(RF_RECEIVER_PIN, OUTPUT);
  digitalWrite(RF_RECEIVER_PIN, LOW);
  pinMode(RF_RECEIVER_PIN, INPUT);
  digitalWrite(RF_RECEIVER_PIN, LOW);
  gw.setup(RF_RECEIVER_PIN, 0x3F);
  
  gw.registerPlugin(72, weather::detectPacket);
  gw.registerPlugin(48, door::detectPacket);
 
#ifdef GREENHOUSE
  //
  // Setup and configure rf radio
  //
  radio.begin();
  // enable dynamic payloads
  radio.enableDynamicPayloads();
  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  //radio.setChannel(1);
  //radio.setPALevel(RF24_PA_MIN);

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  radio.startListening();
  Serial.print("{\"id\": \"greenhouse\", \"code\": 300, \"details\": \"");
  radio.printDetails();
  Serial.println("\"}");
  attachInterrupt(0, check_radio, FALLING);
#endif

}

void loop()
{
  delayMicroseconds(1000);

  if(millis() > time + WEATHER_PERIOD) {
    Serial.print("{\"id\": \"weather\", \"code\": 200");
    Serial.print(", \"IndoorTemperature\": ");
    if(bmp_present) {
      Serial.print(bmp.readTemperature());
      Serial.print(", \"Pressure\": ");
      Serial.print(bmp.readPressure());
    } else {
      //http://playground.arduino.cc/Main/LM35HigherResolution 
      Serial.print(analogRead(LM35sensorPin)*0.45);
    }

    Serial.println("}");
    time = millis();
  }

  // Weather
  if(station.available()) {
    uint64_t p = 0;
    if((p = station.getPacket())) {
      int r = weather::isValidWeather(p);

      switch(r) {
      case OK:
	Serial.print("{\"id\": \"weather\", \"code\": 200, \"Humidity\": ");
        Serial.print(weather::getHumidity(p));
        Serial.print(", \"Temperature\": ");
        Serial.print(weather::getTemperature(p));
#ifdef DEBUG
        Serial.print(", \"Channel\": ");
        Serial.print(weather::getChannel(p));
#endif
        break;
      case INVALID_HUMIDITY:
        Serial.print("{\"id\": \"weather\", \"code\": 500, \"Humidity\": " + station.getError());
        break;
      case INVALID_TEMPERATURE:
        Serial.print("{\"id\": \"weather\", \"code\": 500, \"Temperature\": " + station.getError());
        break;
      case INVALID_SYNC:
        Serial.print("{\"id\": \"weather\", \"code\": 500, \"Sync\": false");
        break;
      }
#ifdef DEBUG
      Serial.print(", \"Packet\": \"0x");
      Serial.print((long unsigned int) p, HEX);
      Serial.print("\"");
#endif
      Serial.println("}");
    }
  }


  // Door
  if(front.available()) {
    uint64_t p = front.getPacket(); //getPacket clears the packet, we keep it to print it further ahead
    if(front.change(p)) {
      Serial.print("{\"id\": \"door\", \"code\": 200, \"change\": 1");
      Serial.println("}");
    }
    else {
      Serial.print("{\"id\": \"door\", \"code\": 500");
#ifdef DEBUG
      Serial.print(", \"Packet\": \"0x");
      Serial.print((long unsigned int) p, HEX);
      Serial.println("\"}");
#endif
    }
  }

#ifdef GREENHOUSE
  // Greenhouse
  if(interrupt_flag) {
    // print the current readings, in JSON format:
    Serial.print("{\"id\": \"greenhouse\", \"code\": 200");
    Serial.print(", \"VCC\": ");
    Serial.print(p.vcc);
    Serial.print(", \"Humidity\": ");
    Serial.print(p.humidity);
    Serial.print(", \"Temperature\": ");
    Serial.print(p.temperature);
    Serial.print(", \"InternalTemp\": ");
    Serial.print(p.internaltemperature);
    Serial.print(", \"Luminosity\": ");
    Serial.print(p.luminosity);
    Serial.print(", \"Water\": ");
    Serial.print(p.water);
    int probes = sizeof(p.humidityProbe)/2; /* int = 2 bytes */
    for(int i=0; i<probes-1; i++) {
      Serial.print(", \"HumidityProbe");
      Serial.print(i);
      Serial.print("\": ");
      Serial.print(p.humidityProbe[i]);
    }
    Serial.print(", \"HumidityProbe"+String(probes-1)+"\": ");
    Serial.print(p.humidityProbe[probes-1]);
    Serial.println(" }");

    interrupt_flag = false;

  }
#endif

  if(command_flag) {
    command_flag = false;

    //Serial.println(cmd[0]);
    char cmd_c[cmd.length()];
    long int code;
    char C = cmd[0];

    switch(C) {
      case 'R':
        cmd.substring(1).toCharArray(cmd_c, cmd.length());
        code = strtol(cmd_c, (char **)0, 16);
        Serial.print("{\"id\": \"transmitter\", \"code\": 200, \"message\": \"0x");
        Serial.print(code, HEX);
        Serial.println("\" }");
        // Retransmit the signal 8 times ( == 2^3) on pin RF_TRANSMITTER. Note: no object was created!
        for(int i=0; i<3; i++) {
          RemoteTransmitter::sendCode(RF_TRANSMITTER_PIN, code, RF_PERIOD, 3);
          delay(100);
         }
         break;
    }
    cmd = "";
  }
}


void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    cmd += inChar;

    if (inChar == ';' || inChar == '\n') {
      command_flag = true;
    }
    Serial.flush();
  }
}
