# The ModuleMasterHttp program
This is master for the WebPage example setup, just like the master in the Arduino and the ESP8266 setups, but meant to run on Windows or Linux.
The PJON SWBB strategy is not available on these platforms, so the DualUDP strategy is used. This means that all modules to be controlled must use this strategy as well, or that one or more PJON DualUDP Switches ([Switch](https://github.com/gioblu/PJON/blob/master/examples/ARDUINO/Local/SoftwareBitBang/Tunneler/SwitchA/SwitchA.ino) or [RGB LED Switch](https://github.com/gioblu/PJON/blob/master/examples/ARDUINO/Local/SoftwareBitBang/Tunneler/BlinkingRGBSwitch/BlinkingRGBSwitch.ino)) are used as bridges to reach the modules. 

This makes it possible to have one or more groups of SWBB connected modules reachable through one Switch for each group.

These are two of many scenarios that can be used for this example setup:
```
                                                       LAN (Ethernet)
                                       _______________________________________________
                                 DUDP |          DUDP | HTTP          |               |
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
 LUDP |          DUDP |          DUDP | HTTP          | HTTP          |
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

Please make sure that you change the web server IP address to what you have assigned to your web server.

## The GenericModuleMasterHttp program
This program is identical to the ModuleMasterHttp program except for that it only needs a command line parameter (the IP address of the web server), and will get all other configuration from the web server. This means that adding a new module to the setup, or removing one, can be done by editing settings in the *settings* table in the database. This can easily be made configurable in a web page as well.

So this program can be built once and used generically, instead of having to be rebuilt every time a module is added or removed like we are used to from the Arduino world.

One more parameter than the web server IP address can be specified, namely the module prefix of the master if there are more than one master in the setup. The default prefix is "m1" (for "master one") and this is used to separate the settings for this master from potential other masters. If there are more than one master, master "m1" will always be the one responsible for triggering archival of outputs to the *timeseries* table for plotting.

The relevant settings in the *settings* table are:
* _m1DevID_ - The device id of the master, by default 1.
* _m1Modules_ - This is the module list that is hardcoded in the non-generic ModuleMasterHttp, giving name, prefix and device id and optionally bus id for each module this master shall control.
* _m1IntSettings_ - The interval between each synchronization of settings, in milliseconds.
* _m1IntOutputs_ - The interval betwen each synchronization of outputs and inputs, in milliseconds.

## The TestModuleMaster program

This is a simplified master that does not communicate with a web server. It is meant to be used for testing that the modules and the switch can be reached and that everything except the web server and database is built and programmed correctly. It is only meant to be used instead of the ModuleMasterHttp or GenericModuleMasterHttp while testing.
```
                                       LAN (Ethernet)
                                       _______________
                                 DUDP |          DUDP |
  _________       _________       ____|____       ____|____ 
 | MODULE1 |     | MODULE2 |     | SWITCH1 |     | MASTER  |
 | ARDUINO |     | ARDUINO |     | ARDUINO |     | LINUX   |
 |_________|     |_________|     |_________|     |_________|
      |               |               |
      |               |               |
      ---------------------------------
                    SWBB           
```

## Build instructions
The ModuleInterface repo has the PJON and ArduinoJson repositiories as dependencies. You should have these located in the same place as the ModuleInterface repository. This is assumed by the build setups for this example.

### Windows
A solution file (ModuleMasterHttp.sln) for Microsoft Visual Studio 2017 is present. Open this project in Visual Studio and build the 32 bit Release build. A ModuleMasterHttp.exe file will be produced and will be present in the Release subdirectory.

If you do not have this version of VS, make a new project and set the include path to include MI/src, PJON and ArduinoJson directories.

### Linux
A Makefile is present. Give the command _make_ to start the build process. An executable named ModuleMasterHttp will be produced.
