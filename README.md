**WiFi Raw-TCP Serial Adapter for the Sharp PC-G850** <br>
A Serial to TCP/IP adapter for the Sharp G850V(S) using the 11-pin port and an ESP8266<br>

**Initial Configuration**<br>
config.h contains all relevant default parameters.<br> 
The firmware looks for a config.ini file in LittleFS and will only use the firmware defaults if no config file can be loaded<br>
(note that if you initialize your ESL with SPIFFs, you can upload a config.ini file until your hair falls out - it will always be wiped when the software initializes LittleFS, so make sure you upload any configuration file using LittleFS filesystem)


**AT commands**  (can be set via netcat/telnet or via the G850, e.g. you could save the AT command in text editor and then "save" via the SIO port) to change the WiFI SSID and password or other parameters.<br>
**+++AT+CFG?**<br>
Returns: +++AT+CFG={"rev":1,"sleep":60,"baud":9600,"port":23,"ssid":"GUEST","wifipw":"your_pw_here","host":"G850V.local","otapw":"myOTAPW"}<br> 
The command displays the active configuration<br>


**+++AT+CFG**={ "rev":\<n>,<br>
            "sleep":\<n>,<br>
            "baud":\<n>,<br>
            "port":\<n>,<br>
            "ssid":"GUEST",<br>
            "wifipw":"new_pw",<br>
            "host":"G850V.local",<br>
            "otapw":"myOTAPW"} <br>**
Returns: OK

Sets a new configuration. you can set just one parameter or all at once.<br>
**rev**: integer, referencing the version of the configuration. I recommend leaving at 1<br>
**sleep**: seconds until unit goes into deep sleep<br>
**baud**: baudrate for connection with G850 (600...9600)<br>
**port**: TCP/IP port (use 23 for telnet compatibility)<br>
**ssid**: the ssid name of your wifi<br>
**wifipw**: password of your wifi network<br>
**host**: hostname for use when requesting an IP address<br> 
**otapw**: password for protrcting your ota passwords (use espota protocol)<br>


Example:<br>
+++AT+CFG={"ssid":"GUEST","wifipw":"pw"}<br>

**+++AT+SAVE**<br>
Returns: OK
saves current configuration as failsafe.ini to LittleFS Flash file system.<br>
**Pressing the PRG button for longer than 5 sec will load failsafe.ini configuration and reboot.**
This allows you to recover from a messed-up config.ini (e.g. wrong wifi credentials)<br>

**+++AT+SLEEP**<BR>
Puts the adapter immediately into sleep.<br>
Returns: OK<br>










