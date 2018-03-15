# ModuleMasterHttp sketch
This is a slightly modified version of the main ModuleMasterHttp example.
It has a different module list passed to the PJONModuleInterfaceSet constructor, corresponding to the modules included in this complete example setup. It also is set to exchange values between modules every second instead of every 10 seconds.

Please make sure that you change the network setup to match your network, and set the web server IP address to what you have assigned to your web server.
(You can call ipconfig/ifconfig in a command window on the web server to get its current IP address, if you are not familiar with this yet.)

This is an illustration of the setup:
```
                                                LAN (Ethernet)
                                        _______________________________
                                       | HTTP          | HTTP          | HTTP
  __________       _________       ____|____       ____|____       ____|____
 |  MODULE1 |     | MODULE2 |     |  MASTER |     | APACHE  |     | BROWSER |
 |  ARDUINO |     | ARDUINO |     | ARDUINO |     |  LINUX  |     | ANY OS  |
 |__________|     |_________|     |_________|     |_________|     |_________|
      |                |               |
      |                |               |
      ----------------------------------
                    SWBB
```