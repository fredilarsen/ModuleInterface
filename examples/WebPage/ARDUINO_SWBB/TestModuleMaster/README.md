# TestModuleMaster sketch
This is a simplified version of the ModuleMasterHttp example, meant for simply testing that the SensorMonitor and LightController modules are correctly built and can talk to each other.

This is an illustration of the setup:
```                                      
  __________       _________       _________ 
 |  MODULE1 |     | MODULE2 |     |  MASTER |
 |  ARDUINO |     | ARDUINO |     | ARDUINO |
 |__________|     |_________|     |_________|
      |                |               |
      |                |               |
      ----------------------------------
                    SWBB
```
If the LightController reacts to varying light levels on the SensorMonitor's light sensor, and the TestModuleMaster prints varying light measurements and LED status to the serial monitor, it means that everything is built correctly at this level, and that the TestModuleMaster module can be replaced with a module with an Ethernet card with either:

1. The ModuleMasterHttp sketch, talking directly to a web server.
2. The PJON BlinkingRGBSwitch example, talking via Ethernet UDP to a ModuleMasterHttp running somewhere else, for example on a Linux or Windows computer.

