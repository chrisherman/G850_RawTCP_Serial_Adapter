
#ifndef ATSCANNER_H
  #define ATSCANNER_H

  #include <Arduino.h>
  #include "config.h"

  //scan a given input stream for AT commands
  class ATScanner {

  public:
    //look through buffer for LF and then invoke parser
    void scan(const uint8_t *buffer, size_t size);

    //looks for AT commands and invokes the necessary action
    void parse();

    //scan a given input stream for AT commands and send results to a print class
    ATScanner(Print &p, void (*sf)()):m_p(p),m_sleeproutine(sf){ 
      m_pos=0;
      m_buf[0]=0x0;
    }

  protected:
    uint8_t m_buf[JSONSIZE];
    size_t m_pos;
    Print &m_p;
    void (*m_sleeproutine)();
  };





  //looking for AT-command at beginning of line and process command if found
  void ATScanner::parse(){

    //save current configuration as failsafe configuration file
    if (strncmp((char*)m_buf, "+++AT+SAVE", 10) == 0) {
      saveFailSafeConfiguration(GlobalConfig);
      PrintFailSafeConfig(m_p);
      m_p.println("OK");
    }

    //show JSON configuration string
    if (strncmp((char*)m_buf, "+++AT+CFG?", 10) == 0) {
       m_p.print("+++AT+CFG=");
       PrintConfig(m_p);
      m_p.println("OK");
    }

   //sends device to sleep
    if (strncmp((char*)m_buf, "+++AT+SLEEP", 11) == 0) {
       m_p.print("OK");
       m_sleeproutine();
    }

    //read JSON configuration string
    if (strncmp((char*)m_buf, "+++AT+CFG=", 10) == 0) {
      #ifdef DEBUG
        Serial.println((char*)m_buf+10);
      #endif
      readConfigurationFromStream(GlobalConfig, (char*)m_buf+10);
      m_p.println("OK");
      saveConfiguration(GlobalConfig);
      PrintConfig(m_p);
      m_p.println("OK");
      delay(2000);
      ESP.reset();
    }
  }


  //copy characters into buf until LF is reached, then analyze line
  void ATScanner::scan(const uint8_t *buffer, size_t size){

    for(size_t n=0; n<size; n++){               
      switch (buffer[n]) {
        case '\r': // Ignore CR
          break;
        case '\n': // Return on new-line
          m_buf[m_pos++] = 0x0;
          parse();
          m_pos = 0;  // Reset position index ready for next time
          return;
        default:
          if (m_pos < sizeof(m_buf)) {
              m_buf[m_pos++] = buffer[n];
          }
          else{ //overflow
            m_pos=0;
            return;
          }
      }
    }
  }


#endif
