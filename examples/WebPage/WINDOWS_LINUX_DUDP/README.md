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

## The GenericModuleMaster program
This program is identical to the ModuleMasterHttp program except for that it only needs a command line parameter that
specifies the address of a server, and will get all other configuration from the web server. This means that adding a new module to the setup, or removing one, can be done by editing settings in the *settings* table in the database. This can easily be made configurable in a web page as well. GenericModuleMaster also support working with an MQTT broker instead of
or in addition to a web server.

So this program can be built once and used generically, instead of having to be rebuilt every time a module is added or removed like we are used to from the Arduino world.

To keep settings in a web server, using the default port 80:
`GenericModuleMaster -http 192.168.1.10`

To use a non-standard web server port:
`GenericModuleMaster -http 192.168.1.10 8080` 

One more parameter can be specified, namely the module prefix of the master if there are more than one master in the setup. The default prefix is "m1" (for "master one") and this is used to separate the settings for this master from potential other masters. If there are more than one master, master "m1" will always be the one responsible for triggering archival of outputs to the *timeseries* table for plotting.

To start a secondary master with prefix m2:
`GenericModuleMaster -http 192.168.1.10 -prefix m2`

The relevant settings in the *settings* table are:
* _m1DevID_ - The device id of the master, by default 1.
* _m1Modules_ - This is the module list that is hardcoded in the non-generic ModuleMasterHttp, giving name, prefix and device id and optionally bus id for each module this master shall control.
* _m1IntSettings_ - The interval between each synchronization of settings, outputs and inputs, in milliseconds.

The address of a MQTT broker can be specified to enable 3-way sync of settings between the web server, the broker and the 
modules. A setting changed in the web page will be sent to the corresponding module as well as to the MQTT broker.
A setting changed by a module (automatic or by a local user interaction like a button or rotary encoder), this will be
shown in the web server as well as in the MQTT broker. If a remote MQTT client (like Home Assistant or Node Red) changes
a setting, this will be reflected to the corresponding module and the web server.
This enables a ModuleInterface setup with a local tailored web site to trigger actions in e.g. Home Assistant, and
Home Assistant can trigger actions or feed values into the modules and the web page.

To start a master using both a web server and a MQTT broker, reading master config from the web server:
`GenericModuleMaster -http 192.168.1.10 -mqtt 192.168.1.11 -config http`

It is also possible to run the master with no web server, only a MQTT broker:
`GenericModuleMaster -mqtt 192.168.1.11`

In this case the master settings must be located in the topics:
```
moduleinterface/master_m1/setting/devid
moduleinterface/master_m1/setting/modules
moduleinterface/master_m1/setting/intsettings
```

It can also use MQTT in JSON mode. In this case, the master settings would reside in a single topic:
`moduleinterface/master_m1/setting`

The value of this topic would be in this format:
```
{ 
  "DevID": "1",
  "Modules": "Switch1:w1:40 ExLight1:e1:12 SnowMelt:s1:13 CarHeat1:c1:14 TeslaC:tc:7",
  "IntSettings": "10000"
}
```

## The TestModuleMaster program

This is a simplified master that does not communicate with a web server. It is meant to be used for testing that the modules and the switch can be reached and that everything except the web server and database is built and programmed correctly. It is only meant to be used instead of the ModuleMasterHttp or GenericModuleMaster while testing.
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
