# ModuleMasterHttp sketch
This is a slightly modified version of the main ModuleMasterHttp example.
It has a different module list passed to the PJONModuleInterfaceSet constructor, corresponding to the modules included in this complete example setup. It also is set to exchange values between modules every second instead of every 10 seconds.

These examples rely on a DHCP server being available. If there is none, you need to to add "manual" network configuration in the sketch.

Please make sure that you change the web server IP address to what you have assigned to your web server.
(You can call ipconfig/ifconfig in a command window on the web server to get its current IP address, if you are not familiar with this yet.)

These ESP8266 examples have been built and tested with the "NodeMCU 0.9" and the "Wemos D1 R1" board profiles in the Arduino IDE.
