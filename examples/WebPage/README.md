##WebPage example
This is not a sketch, but a recipe of how to set up a complete setup with one or more modules being visualized and controlled from a web page.

####Components
1. A worker module, for example the BlinkSimple/BlinkModule example running on an Arduino Uno or Nano.
2. A master module with an Ethernet shield, for example the ModuleMasterHttp example running on an Arduino Mega (because of memory requirements) with an Ethernet shield.
3. A computer with a LAMP/WAMP setup. <INSERT LINK TO DOWNLOAD>

####Configuration of worker
The PJON device id of the worker must match the one used by the master for referring to it. That's it.
The worker device and the master must be connected with two wires -- one connecting digital pin 7 on both devices, and one connecting ground. 
Note that a pulldown resistor between signal and earth may improve the speed. See the PJON project for more details.
 
####Configuration of master
 The master must have a correct network configuration for the network to which it is connected. This includes:
 - A unique MAC address, just fiddle with the MAC address in the sketch.
 - A unique IP address. The example use a fixed IP address for simplicity and to not be depending on a DHCP server being present. Make sure that the assigned IP address is outside any DHCP pool in your router if you want this as a permanent setup.
 - The IP of your gateway, normally the router address. This is not strictly needed if the web server is on the same network segment.
 - The network mask for your network. Normally the mask is 255.255.255.0.
 
####Configuration of web server and database
A. The database is created as specified in the ModuleMasterHttp/database setup/Table structure.txt, or by running ModuleMasterHttp/database setup/home_control_structure.sql.
B. The files from ModuleMasterHttp/database setup are copied into the Apache/htdocs folder along with the web page from ModuleMasterHttp/web page.
C. The db_config.php file edited to contain the selected database name, user name and password.
D. The variable_write_list.txt file edited to contain the names of all columns in the time_series table that should be written to. This is a variable/column write access whitelist.

####Testing
When all is mounted and configured, check this:
1. Are new rows being created in the time_series table? You can inspect the table with HeidiSQL or other tools.
2. Are you getting the values shown in the plot in the web page?
3. Can you change the frequency in the web page and see the that the worker changes blink frequency? 

####Debugging
If it does not work right away, you can try the following steps.
1. Try to program the master with the BlinkSimple/BlinkModuleMaster (check device id match) and see if the worker blink frquency changes every 5 seconds as expected.
  - Yes: PJON communication is configured correctly. Double-check that the ModuleMasterHttp has the same settings as BlinkModuleMaster, then program with the ModuleMasterHttp and proceed.
  - No: Concentrate on getting the BlinkModule - BlinkModuleMaster setup working before proceeding.
2. Try to manually insert a value into the database by inputting the following in the address field of a browser:
  <server address>/... <TODO: complete this> 
  Do you manage to insert values?
  - Yes: Then supposedly the master is failing in doing this. Re-check the master device IP settings and try again.
  - No: You should get an error message in the browser. Try to use this to find out what is wrong. Check db_config.php. Check variable_write_list.txt.