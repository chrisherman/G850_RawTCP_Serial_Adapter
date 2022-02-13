#G850 Raw-TCP Serial Adapter
##A Serial to TCP/IP adapter for the Sharp G850V(S) using the 11-pin port and an ESP8266

##Initial Configuration
config.h contains all relevant default parameters 
The firmware looks for a config.ini file in LittleFS and will only use the firmware defaults if no config file can be loaded
(note that if you initialize your ESL with SPIFFs, you can upload a config.ini file until your hair falls out - it will always be wiped when the software initializes LittleFS, so make sure you upload any configuration file using LittleFS filesystem)




**AT commands**  (can be set via netcat/telnet or via the G850, e.g. you could save the AT command in text editor and then "save" via the SIO port)
+++AT+CFG?    

displays the active configuration


+++AT+CFG={ "rev":\<n>,
            "sleep":\<n>,
            "baud":\<n>,
            "port":\<n>,
            "ssid":"GUEST",
            "wifipw":"pw",
            "host":"G850V.local",
            "otapw":"myOTAPW"} 

sets a new configuration. you can set just one parameter or all at once.
rev: integer, referencing the version of the configuration. I recommend leaving at 1<br>
sleep: seconds until unit goes into deep sleep
baud: baudrate for connection with G850 (600...9600)
port: TCP/IP port (use 23 fir telnet compatibility)
ssid: the ssid name of your wifi
wifipw: password of your wifi network
host: hostname for use when requesting an IP address 
otapw: password for protrcting your otapassowrds (use espota protocol)


Example:
+++AT+CFG={"ssid":"GUEST","wifipw":"pw"}

+++AT+SAVE
saves current configuration as failsafe.ini to LittleFS Flash file system.
**Pressing the PRG button for longer than 5 sec will load failsafe.ini configuration and reboot.**
This allows you to recover from a fucked-up config.ini (e.g. wrong wifi credentials)









