# ModuleMasterHttp sketch
This is master for the WebPage example setup, just like the master in the Arduino and the ESP8266 setups, but meant to run on Windows or Linux.
The PJON SWBB strategy is not available on these platforms, so the LocalUDP strategy is used. This means that all modules to be controlled must use this strategy as well, or that one or more PJON LocalUDP Switches ([Switch](https://github.com/gioblu/PJON/blob/master/examples/ARDUINO/Local/SoftwareBitBang/Tunneler/SwitchA/SwitchA.ino) or [RGB LED Switch](https://github.com/gioblu/PJON/blob/master/examples/ARDUINO/Local/SoftwareBitBang/Tunneler/BlinkingRGBSwitch/BlinkingRGBSwitch.ino)) are used as bridges to reach the modules. 

This makes it possible to have one or more groups of SWBB connected modules reachable through one Switch for each group.

These are two of many scenarios that can be used for this example setup:
```
                                                       LAN (Ethernet)
                                       _______________________________________________
                                 LUDP |          LUDP | HTTP          |               |
  _________       _________       ____|____       ____|____       ____|____       ____|____
 | MODULE1 |     | MODULE2 |     | SWITCH1 |     | MASTER  |     | APACHE  |     | BROWSER |
 | ARDUINO |     | ARDUINO |     | ARDUINO |     | LINUX   |     | LINUX   |     | ANY OS  |
 |_________|     |_________|     |_________|     |_________|     |_________|     |_________|
      |               |               |
      |               |               |
      ---------------------------------
                    SWBB           



                               LAN (Ethernet)
       _______________________________________________________________
 LUDP |          LUDP |          LUDP | HTTP          | HTTP          |
  ____|____       ____|____       ____|____       ____|____       ____|____ 
 | SWITCH1 |     | SWITCH2 |     | MASTER  |     | APACHE  |     | BROWSER |
 | ARDUINO |     | ARDUINO |     | LINUX   |     | LINUX   |     | ANY OS  |
 |_________|     |_________|     |_________|     |_________|     |_________|
      |               |
      | SWBB          | SWBB
  ____|____       ____|____
 | MODULE1 |     | MODULE2 |               
 | ARDUINO |     | ARDUINO |
 |_________|     |_________|
```

All the LocalUDP devices must be connected to the same LAN. If you need to communicate across LANs, you will need to use the GlobalUDP or EthernetTCP strategies instead. Also note that the LocalUDP strategy does not work very fast on ESP8266 because of shortcomings in the network library when it comes to UDP broadcasts. On all other architectures it is a good choice. On ESP8266 the GlobalUDP strategy works well, but requires fixed IP addresses and some more configuration than LocalUDP.

Please make sure that you change the web server IP address to what you have assigned to your web server.

## Build instructions
The ModuleInterface repo has the PJON and ArduinoJson repositiories as dependencies. You should have these located in the same place as the ModuleInterface repository. This is assumed by the build setups for this example.

### Windows
A solution file (ModuleMasterHttp.sln) for Microsoft Visual Studio 2017 is present. Open this project in Visual Studio and build the Release build. A ModuleMasterHttp.exe file will be produced and will be present in the Release subdirectory.

If you do not have this version of VS, make a new project and set the include path to include MI/src, PJON and ArduinoJson directories.

### Linux
A Makefile is present. Give the command _make_ to start the build process. An executable named ModuleMasterHttp will be produced.
