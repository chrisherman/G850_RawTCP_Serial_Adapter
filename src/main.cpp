/*****************************************************************
 * Sharp PC-G850V(S) ESP32 Interface
 * By: ChrisHerman
 * Date: December 5th, 2021
 *
 * Receives files from Sharp G850 and sends to PC via USB-Serial (Nano)
 * Requires G850's RTS and CTS lines to be connected witch each other 
 * and pulled-up to +5V. otherwise SHarp will wait forever.
 * 
 *
 * Hardware Hookup:
 * * 
 * Sharp G850 11 pin port         ESP32
 *=========================================
 *  GND (pin 3) __________________ GND
 * 
 * Rx (pin 6)  __________________ GPIO4
 * 
 * Tx (pin 7)  __________________ GPIO5
 * 
 * RTS (pin 4) __________
 *                       |
 * CTS (pin 9) __________+
 *                       |
 * +5V         ---[10K]--+
 * 
 *
 * This code is released under the [MIT License](http://opensource.org/licenses/MIT).
 *****************************************************************/



#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Ticker.h>  

//below to ensure Strings from platformio.ini are handled as intended by pre-compiler
#define ST(A) #A
#define STR(A) ST(A)

#define RX_PIN 5
#define TX_PIN 4 
#define BAUD_FILE "/G850.CFG" //persistent storage for selected baudrate

//#define LED_PIN 2
#define LED_PIN 10
const int BlinkDelay= 10; // blink period in ms

// second button on the PCB should be PRG-pin
#define PRG_PIN 0


#ifndef SLEEPTIMEOUT_MSEC
  #define SLEEPTIMEOUT_MSEC 300000
#endif
#ifndef SLEEPTIMER_DIV
  #define SLEEPTIMER_DIV 12
#endif

//SOftSerial related objects
#define BUFFER_SIZE 1024    
byte buff[BUFFER_SIZE];
SoftwareSerial SoftSerial(RX_PIN, TX_PIN, true); // RX, TX, inverse_logic = true

// global objects
WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
WiFiServer server(RAW_TCP_PORT);
WiFiClient  client;

void checkFlash(){
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

#ifdef DEBUG
  Serial.printf("\n");
  Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u bytes\n", realSize);
  Serial.printf("Flash ide  size: %u bytes\n", ideSize);
  Serial.printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

  if (ideSize != realSize) 
    Serial.println("Flash Chip configuration wrong!\n");
  else 
    Serial.println("Flash Chip configuration ok.\n");
#endif

}


void OTASetup(){

  ArduinoOTA.setPassword((const char *)STR(OTAPW));
  ArduinoOTA.onStart([]() {
    #ifdef DEBUG
      Serial.println("Start");
    #endif
  });
  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
      Serial.println("\nEnd");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });

  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif
  });

  ArduinoOTA.begin();
}





int GetBaudrate(int i){

  #ifdef DEBUG
    Serial.println( String( "GetBaudrate=") + i);
  #endif

  switch (i){

    case 0: return SOFTBAUDRATE0;
    break;

    case 1: return SOFTBAUDRATE1;
    break;
    
    case 2: return SOFTBAUDRATE2;
    break;
    
    case 3: return SOFTBAUDRATE3;
    break;
    
    case 4: return SOFTBAUDRATE4;
    break;

    default: return SOFTBAUDRATE4;
    break;
  }
}


int ReadPersistent(){
  
  File file = SPIFFS.open( BAUD_FILE, "r");
  if(file){
    int i= file.readString().toInt();
    #ifdef DEBUG
      Serial.println( String("ReadPersistent=")+i);
    #endif
    return i;
  }
  else
    return -1;
}

void WritePersistent(int v){
  File file = SPIFFS.open( BAUD_FILE, "w");
  if(file){
    file.print( v);
  }
  #ifdef DEBUG
    Serial.println(String("WritePersistent=")+v);
  #endif

  ReadPersistent();
}

void Inspect(const uint8_t *buffer, size_t size){
  // looking for inband AT command coming from TCP client

  const char *codeword= "+++AT?";
  const size_t codelength= 6;
  static size_t matchcount=0;

  for(size_t n=0; n<size; n++){
    if((buffer[n]==codeword[matchcount])||(codeword[matchcount]=='?'))   //character matching
      matchcount++;                       // we have a match
    else{
      if(n>=matchcount){
        n-= matchcount;                   // start comparing entire sequence again, but start one character later, avoiding +++++ATx misses
      }
      matchcount=0;                       // nope, no match, start from scratch
    }

    if(matchcount==codelength){           // all charcters were matched
      int value= buffer[n]-48;
      int newbaud= GetBaudrate( value );
      SoftSerial.begin(newbaud);
      if(client)
        client.print(String("Baud")+newbaud+String("OK"));
      WritePersistent(value);
      matchcount=0;                       // reset for search for next command string
    }
  }
}




/////////////////////////////
//what to do with our LED: 
enum blinkstates {Off, Single, Double, Tripple, On} ;
blinkstates blinkstate= Off;

static void blinker(){

  #define LEDON 0x1
  #define LEDOFF 0x0
  
  #ifdef DEBUG
    //Serial.print("*");
  #endif

  //set pin as output
  static bool firstcall= true;
  if(firstcall){
      pinMode(LED_PIN, OUTPUT);
      firstcall=false;
  }

  static int cycle= 0;
  cycle++;
  switch (blinkstate){
    case Off: 
      digitalWrite(LED_PIN, LEDOFF); 
      cycle=0;
    break;

    case Single: 
        switch(cycle){
          case 1: digitalWrite(LED_PIN, LEDON); break; 
          case 150: cycle=0; break;
          default: digitalWrite(LED_PIN, LEDOFF); break;
        }
    break;
        
    case Double: 
        switch(cycle){
          case 1: digitalWrite(LED_PIN, LEDON); break; 
          case 20: digitalWrite(LED_PIN, LEDON); break;
          case 170: cycle=0; break;
          default: digitalWrite(LED_PIN, LEDOFF); break;
        }
    break;
    
    case Tripple: 
        switch(cycle){
          case 1: digitalWrite(LED_PIN, LEDON); break; 
          case 21: digitalWrite(LED_PIN, LEDON); break;
          case 41: digitalWrite(LED_PIN, LEDON); break;
          case 190: cycle=0; break;
          default: digitalWrite(LED_PIN, LEDOFF); break;
        }
    break;
    case On: 
        digitalWrite(LED_PIN, LEDON); 
        cycle=0;
    break;

    default: 
      digitalWrite(LED_PIN, LEDOFF);
    break;
  }
}

void SetBlinker(blinkstates bs){
  blinkstate= bs;
}

Ticker BlinkTimer(blinker, BlinkDelay, 0, MILLIS);


//////////////////////////////////
// sleep-timout related stuff:
int sleepCountdown=0;
static void SleepCheck(){
  #ifdef DEBUG
    Serial.println("Going to sleep");
  #endif

  sleepCountdown++;

  if(sleepCountdown==(SLEEPTIMER_DIV-1)){
    SetBlinker(Double);
  } else  if(sleepCountdown==SLEEPTIMER_DIV){
    SetBlinker(Tripple);
  } else if(sleepCountdown>SLEEPTIMER_DIV){
       ESP.deepSleep(0);   
  } else
    SetBlinker(Single);

}

Ticker SleepTimer(SleepCheck, SLEEPTIMEOUT_MSEC/SLEEPTIMER_DIV, 0, MILLIS);

void SleepTimerRestart(){
  sleepCountdown=0;
  SetBlinker(Single);
  SleepTimer.start(); //reset timer
}





void setup() {
  #ifdef DEBUG
    Serial.begin(BAUDRATE);
  #endif

  SetBlinker(On);
  BlinkTimer.start();  

  pinMode(PRG_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(PRG_PIN), SleepTimerRestart, RISING);


  checkFlash();
  if (SPIFFS.begin()){
    #ifdef DEBUG
      Serial.println("File system mounted");
    #endif
  }
  else {
    #ifdef DEBUG
      Serial.println("File system error");
    #endif
  }

  int br= GetBaudrate(ReadPersistent());
  #ifdef DEBUG
    Serial.println( String("return from GetBaudrate=") + br);
  #endif

  SoftSerial.begin(br);


  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    WiFi.setHostname(STR(WIFIHOSTNAME));
    SetBlinker(Single);
    #ifdef DEBUG
      Serial.print("Station connected, IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Hostname:");
      Serial.println(WiFi.getHostname());
    #endif
  });
  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    SetBlinker(On);
    #ifdef DEBUG
      Serial.println("Station disconnected");
    #endif
  });


 
  WiFi.begin(STR(SSID), STR(WIFIPASSWORD));
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  delay(200);
  OTASetup();

  server.begin();
  #ifdef DEBUG
    Serial.println("servers started");
  #endif
  SleepTimer.start();
}


void loop() {
  //static unsigned long timer=millis();
  int size = 0;

  client = server.available();              //wait for client connection 

  if (client){
    //clear any bytes so far in the Serial Buffer to avoid sending garbage to new client
    while(SoftSerial.available()>0)  {  
      SoftSerial.read();  
    }

    while (client.connected()) {
      
    
      // read data from wifi client and send to serial
      while ((size = client.available())) {
            size = (size >= BUFFER_SIZE ? BUFFER_SIZE : size);
            client.read(buff, size);
            Inspect(buff, size);
            SoftSerial.write(buff, size);
            SoftSerial.flush();
            SleepTimerRestart();
            BlinkTimer.update();
      }
    
      // read data from serial and send to wifi client
      while ((size = SoftSerial.available())) {
            size = (size >= BUFFER_SIZE ? BUFFER_SIZE : size);
            SoftSerial.readBytes(buff, size);
            Inspect(buff, size);
            client.write(buff, size);
            client.flush();
            SleepTimerRestart();
            BlinkTimer.update();
      }
      
      SleepTimer.update();
      BlinkTimer.update();
      ArduinoOTA.handle();
    }


    client.stop();
    SleepTimer.update();
    BlinkTimer.update();
    ArduinoOTA.handle();
  }


  //no longer connected
  //keep watching serial port for commands
  while ((size = SoftSerial.available())) {
        size = (size >= BUFFER_SIZE ? BUFFER_SIZE : size);
        SoftSerial.readBytes(buff, size);
        //client.write(buff, size);
        //client.flush();
        Inspect(buff, size);
        SleepTimerRestart();
        BlinkTimer.update();
  }


  SleepTimer.update();
  BlinkTimer.update();
  ArduinoOTA.handle();
  //delay(5);
}
