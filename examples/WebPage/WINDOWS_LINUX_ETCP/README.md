# ModuleMasterHttp sketch
This is a slightly modified version of the main ModuleMasterHttp example.
It has a different module list passed to the PJONModuleInterfaceSet constructor, corresponding to the modules included in this complete example setup. It also is set to exchange values between modules every second instead of every 10 seconds.

Note that this master based on the PJON EthernetTCP strategy is using a special one-to-one mode that assumes that that a device running the PJON Surrogate example is placed on the SWBB bus, connecting to the master. This allows the master to efficiently be part of the SWBB bus across any network distance and requiring only one-way firewall openings.

If you are only going to run the master and modules on your LAN, it is easier to use the LocalUDP (LUDP) based master instead.

This is a use case for this example setup using the ETCP based master:
```
                                                       WAN (Ethernet)
                                       ______......._______ _______________________________
                                 ETCP |               ETCP | HTTP          |               |
  _________       _________       ____|____            ____|____       ____|____       ____|____
 | MODULE1 |     | MODULE2 |     |SURROGATE|          | MASTER  |     | APACHE  |     | BROWSER |
 | ARDUINO |     | ARDUINO |     | ARDUINO |          | LINUX   |     | LINUX   |     | ANY OS  |
 |_________|     |_________|     |_________|          |_________|     |_________|     |_________|
      |               |               |
      |               |               |
      ---------------------------------
                    SWBB           

```
Please make sure that you change the web server IP address to what you have assigned to your web server.

## Build instructions
The ModuleInterface repo has the PJON and ArduinoJson repositiories as dependencies. You should have these located in the same place as the ModuleInterface repository. This is assumed by the build setups for this example.

### Windows
A solution file (ModuleMasterHttp.sln) for Microsoft Visual Studio 2017 is present. Open this project in Visual Studio and build the Release build. A ModuleMasterHttp.exe file will be produced and will be present in the Release subdirectory.

If you do not have this version of VS, make a new project and set the include path to include MI/src, PJON and ArduinoJson directories.

### Linux
A Makefile is present. Give the command _make_ to start the build process. An executable named ModuleMasterHttp will be produced.
