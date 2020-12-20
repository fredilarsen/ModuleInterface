# Overview of examples
The examples directory contains the following directories:

## BlinkSimple
The examples in this directory demonstrate the simplest use of the library on Arduino (Nano or larger) using single wire communication with the PJON SoftwareBitBang strategy, with only a couple of modules and a master and no connection to any web server or database.

They demonstrate a setup with a module _BlinkModule_ that is blinking the built-in LED on and off with a given interval. The interval in seconds is a setting that is changed regularly by the master _BlinkModuleMaster_.

There is another master _BlinkModuleMasterMultiple_ that shows how to control multiple modules, not just one. It can control two blink modules.
The blink time interval is an output from the blink module, and there is another blink module _BlinkModuleFollower_ that will subscribe to that output and follow the blink interval of the primary blink module, if they are both connected to the _BlinkModuleMasterMultiple_. One transfer interval after the main blink module has changed its blink interval, the follower will change its own interval to the same.

There is also an example _BlinkModuleEvent_ that is meant as a replacement for _BlinkModule_, demonstrating how to flag an output change as an event to have it transferred imediately without waiting for the scheduled transfer. This is useful for having immediate action based on events in other modules, for example if a motion sensor in one module should turn on the light in another module without waiting for the scheduled transfer.

## BlinkAdvanced 
The examples in this directory demonstrate some more advanced features, still with the environment from the _BlinkSimple_ example set. 

### BlinkModule
 The _BlinkModule_ gives an example of multiple settings and data types, including a duty cycle in addition to the time interval. It reports its own uptime and a heartbeat (a counter that is increased by one for each transfer).

It demonstrates the notification function that is called when new settings, inputs or time sync are delivered, and also before outputs are retrieved.

It shows the use of a custom receiver function, allowing custom PJON packets to be received and sent without influencing the ModuleInterface functionality.

It also demonstrates how the module saves settings to EEPROM and loads them when starting up so that it can continue to operate even if off-line without getting settings from the master after startup.

As RAM is valuable on Arduinos, a feature to save RAM is shown in the blink module, storing the parameter contract strings in PROGMEM and not RAM.

### BlinkModuleMaster
The arrays of settings/inputs/outputs are accessed directly, needing the position of each variable to be remembered.
As an alternative to this, the _MIVariable_ class can be used to refer to variables by name instead of position when accessing them, also if a module is stopped and restarted with a different contract (for example with a new setting present).

The notification callback function is shown used in the master, reacting immediately to new outputs from the module, and changing the duty cycle immediately before transfer of new settings to the module.

The master will print the status of the status of the heartbeat to the serial port, and also print a line when the duty cycle changes.

## NetworkBlinkSimple
This is a copy of the _BlinkSimple_ example set, with the addition of the use of a bus id to demonstrate the "network mode" of PJON.

### Short intro to PJON network mode
When not specifying a bus id ("local mode"), all modules on the same physical medium will be part of the same bus.

If specifying a bus id for each device, it is possible to put devices on different buses to allow more devices and have some isolation between the logical buses. If the logical buses are on the same physical medium, packets can be sent between buses by specifying the correct receiver bus id when sending a packet, or by using a PJON router or switch.

If separate physical media are used for the different buses, they must be connected with a PJON router or switch to be able to communicate with each other, forming a larger network. In this way, different physical media like single wire, optical, Ethernet, serial etc can form a larger network of modules of different hardware like Arduino, ESP8266, ESP32, RPI, Linux, Windows and a lot more.

Have a look at the PJON documentation and examples to get more insight if you have more devices than 254 (which is the limit for one bus) or for some other reason need multiple buses.

## SensorMonitor
This is a single simplistic example that shows how little code is needed to read a sensor (or multiple) and have the measurements transported into an existing ModuleInterface ssetup. Programming a new device with these few lines of code and hooking it up to the bus will get the measurements transported into a database for plotting in a web page, and for distribution to other modules that subscribe to it.

## ModuleMasterHttp
This example is a complete master that in addition to exchanging values between modules will get settings from a web server and log all outputs (measurements) to the web server.

The WebPage example setup described below shows this in a complete working setup.

## WebPage
This example setup includes everything needed for setting up a web server and database, with a database schema and a working responsive web page that let you inspect and configure a couple of ModuleInterface modules that work together.

There are multiple alternatives when it comes to selection of hardware.

There is an example setup for running two modules on Arduino Nanos (or larger) and the master on an Arduino Mega with an Ethernet shield, using the SoftwareBitBang strategy on a single wire. The Ethernet shield is used for talking to the web server.

There is another example setup running both the modules and the master on ESP8266, with the PJON LocalUDP strategy for communication between modules and master over WiFi.

If an Ethernet based strategy is used for the modules (like in the ESP8266 setup), or there is a switch connecting a SoftwareBitBang bus to an Ethernet based bus like LocalUDP, then the master can successfully be run as a process on a multi-process operating system like Linux or Windows. It can run on the same computer as the web server and database, or on a Raspberry PI or similar.

Therefore, masters for Windows and Linux for a few Ethernet based strategies are included.
Of these, the DualUDP strategy seems to be the best choice, offering autodiscovery of LAN devices based on their PJON id alone (like LocalUDP), while at the same time reducing the amount of broadcasts and also supporting remote modules on other buses (like GlobalUDP).

The _GenericModuleMaster_ masters are recommended, as they can be built once and will not have to be modified when a new device is added to the setup. The text containing the list of modules is read at startup from the database through the web server.

The WebPage example setup is big and has its [own documentation](https://github.com/fredilarsen/ModuleInterface/blob/master/examples/WebPage/README.md).
