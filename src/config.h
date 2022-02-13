/*****************************************************************************
 * 
 * 
 *  handles all reading and writing and defaulting of configuration items
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * *****************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>
#include <LittleFS.h>


#define SERIALBAUDRATE 9600
#define SOFTBAUDRATE0 600
#define SOFTBAUDRATE1 1200
#define SOFTBAUDRATE2 2400
#define SOFTBAUDRATE3 4800
#define SOFTBAUDRATE4 9600



// Our configuration structure.
//
// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config {
    int revision;
    char wifissid[33];
    char wifipassword[64];
    char hostname[255];  
    char otapw[64];
    int sleeptimeout;
    int softbaudrate;
    int rawport;
};
#define JSONSIZE 512

#define REVISION_TAG "rev"
#ifndef REVISION 
  #define REVISION 1 
#endif

#define RAW_TCP_PORT_TAG "port"
#ifndef RAW_TCP_PORT
  #define RAW_TCP_PORT 23 
#endif

#define WIFISSID_TAG "ssid"
#ifndef WIFISSID
  #define WIFISSID "GUEST"
#endif

#define WIFIPASSWORD_TAG "wifipw"
#ifndef WIFIPASSWORD
  #define WIFIPASSWORD "AAAAAAAAAA"
#endif

#define HOSTNAME_TAG "host"
#ifndef HOSTNAME 
  #define HOSTNAME "G850V.local"
#endif

#define OTAPW_TAG "otapw"
#ifndef OTAPW
#define OTAPW "myOTAPW"
#endif

#define SLEEPTIMEOUT_TAG "sleep"
#ifndef SLEEPTIMEOUT
#define SLEEPTIMEOUT 600
#endif

#ifndef MINIMUMSLEEPTIMEOUT
#define MINIMUMSLEEPTIMEOUT 30
#endif

#define SOFTBAUDRATE_TAG "baud"
#ifndef SOFTBAUDRATE
#define SOFTBAUDRATE 9600
#endif

const char* CONFIGFILENAME= "/config.ini";
const char* FAILSAFEFILENAME= "/failsafe.ini";

Config GlobalConfig;                         // <- global configuration object


// Loads the default configuration
void loadDefaultConfiguration(Config &cfg) {
      cfg.revision = REVISION;
      cfg.sleeptimeout = SLEEPTIMEOUT;
      cfg.softbaudrate= SOFTBAUDRATE;
      cfg.rawport= RAW_TCP_PORT;
      strlcpy(cfg.wifissid,  WIFISSID, sizeof(cfg.wifissid));
      strlcpy(cfg.wifipassword, WIFIPASSWORD, sizeof(cfg.wifipassword));
      strlcpy(cfg.hostname, HOSTNAME, sizeof(cfg.hostname));
      strlcpy(cfg.otapw, OTAPW, sizeof(cfg.otapw));
}



void saveConfiguration(const Config &cfg);

// Loads the configuration from a file or string
void readConfigurationFromFile(Config &cfg, const char *filename) {
  #ifdef DEBUG
    Serial.println((String)"Loading Configuration" + filename  );
  #endif

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<JSONSIZE> doc;
  DeserializationError error;

  #ifdef DEBUG
    Serial.println(F("deserialize from file"));
  #endif
  File file = LittleFS.open(filename, "r");
  // Deserialize the JSON document
  error = deserializeJson(doc, file);
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

  if (error){
    #ifdef DEBUG
      Serial.println(F("Failed to read file, using default configuration"));
    #endif
    loadDefaultConfiguration(cfg);
    saveConfiguration(cfg);
  }
  else{
    // Copy values from the JsonDocument to the Config
    cfg.revision = doc[REVISION_TAG]|REVISION;
    cfg.sleeptimeout = doc[SLEEPTIMEOUT_TAG]|SLEEPTIMEOUT;
    cfg.sleeptimeout= ((cfg.sleeptimeout<60)?60:cfg.sleeptimeout);
    cfg.softbaudrate= doc[SOFTBAUDRATE_TAG]|SOFTBAUDRATE;
    cfg.rawport= doc[RAW_TCP_PORT_TAG]|RAW_TCP_PORT;
    strlcpy(cfg.wifissid, doc[WIFISSID_TAG]|WIFISSID, sizeof(cfg.wifissid)); 
    strlcpy(cfg.wifipassword, doc[WIFIPASSWORD_TAG]|WIFIPASSWORD, sizeof(cfg.wifipassword));
    strlcpy(cfg.hostname, doc[HOSTNAME_TAG]|HOSTNAME, sizeof(cfg.hostname));         
    strlcpy(cfg.otapw, doc[OTAPW_TAG]|OTAPW, sizeof(cfg.otapw));         
  }
}


// Loads the configuration from a file or string
void readConfigurationFromStream(Config &cfg, char *buf) {
  #ifdef DEBUG
    Serial.println("Loading Configuration from Stream");
  #endif

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<JSONSIZE> doc;
  DeserializationError error;

  #ifdef DEBUG
    Serial.println(F("deserialize from buffer"));
  #endif
  error = deserializeJson(doc, buf);

  if (error){
    #ifdef DEBUG
      Serial.println(F("Failed to read file, using default configuration"));
    #endif
  }
  else{
    // Copy values from the JsonDocument to the Config
    cfg.revision = doc[REVISION_TAG]|cfg.revision;
    cfg.sleeptimeout = doc[SLEEPTIMEOUT_TAG]|cfg.sleeptimeout;
    cfg.sleeptimeout= ((cfg.sleeptimeout<MINIMUMSLEEPTIMEOUT)?MINIMUMSLEEPTIMEOUT:cfg.sleeptimeout);
    cfg.softbaudrate= doc[SOFTBAUDRATE_TAG]|cfg.softbaudrate;
    cfg.rawport= doc[RAW_TCP_PORT_TAG]| cfg.rawport;
    strlcpy(cfg.wifissid, doc[WIFISSID_TAG]|cfg.wifissid, sizeof(cfg.wifissid)); 
    strlcpy(cfg.wifipassword, doc[WIFIPASSWORD_TAG]|cfg.wifipassword, sizeof(cfg.wifipassword));
    strlcpy(cfg.hostname, doc[HOSTNAME_TAG]|cfg.hostname, sizeof(cfg.hostname));         
    strlcpy(cfg.otapw, doc[OTAPW_TAG]|cfg.otapw, sizeof(cfg.otapw));         
  }
}



void loadFailSafeConfiguration(Config &cfg){
  readConfigurationFromFile(cfg, FAILSAFEFILENAME);
}

void loadConfiguration(Config &cfg) {
  readConfigurationFromFile(cfg, CONFIGFILENAME);
}

  
  
void writeConfiguration(const Config &cfg, const char *filename){
  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);
  // Open file for writing
  File file = LittleFS.open(filename, "w");
  if (!file) {
    #ifdef DEBUG
      Serial.println(F("Failed to create file"));
    #endif
    return;
  }

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<JSONSIZE> doc;


    // Copy values from the JsonDocument to the Config
    doc[REVISION_TAG] = cfg.revision;
    doc[SLEEPTIMEOUT_TAG] = cfg.sleeptimeout;
    doc[SOFTBAUDRATE_TAG]=  cfg.softbaudrate;
    doc[RAW_TCP_PORT_TAG]= cfg.rawport;
    doc[WIFISSID_TAG]= cfg.wifissid;
    doc[WIFIPASSWORD_TAG]= cfg.wifipassword;
    doc[HOSTNAME_TAG] = cfg.hostname;
    doc[OTAPW_TAG] = cfg.otapw;     
    
    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
}

void saveFailSafeConfiguration(const Config &cfg) {
  writeConfiguration( cfg, FAILSAFEFILENAME);
}


// Saves the configuration to standard ini-file
void saveConfiguration(const Config &cfg){
  writeConfiguration( cfg, CONFIGFILENAME);
}







// Prints the content of a config file to the print object specified
void PrintIniFile(Print &p, const char *filename) {

  #ifdef DEBUG
    // Open file for reading
    File file = LittleFS.open(filename, "r");
    if (!file) {
        #ifdef DEBUG
          Serial.print(F("PrintCOnfig: Failed to read file"));
        #endif
        return;
    }

    while (file.available()) {
        p.print((char)file.read());
    }
    p.println();
    file.close();
  #endif
}

void PrintConfig(Print &p) {
  PrintIniFile(p, CONFIGFILENAME);
}

void PrintFailSafeConfig(Print &p) {
  PrintIniFile(p, FAILSAFEFILENAME);
}

#endif
