##ModuleInterface 0.5
ModuleInterface is an Arduino compatible library for automatic transfer of settings and values between devices, with very little programming needed for each device.

This is ideal for quickly creating a master-slave based collection of devices (modules) doing things like measuring temperatures and other sensors, turning things on and off, regulating heating and so on. With synchronization between the modules, and between the modules and a database if using the HTTP client that is available for the master module.

A typical setup can consist of multiple Arduino Nano devices connected to a master on an Arduino Mega with an Ethernet shield. 

####Features
- Transport of values (measured and calculated) between modules according to their contracts
- Transport of settings to modules from master
- HTTP client lets the master transfer settings between a database and the modules, for configuration in web pages
- HTTP client lets the master save output values from the modules into a database, for time series trending and inspection in web pages
- Optional persistence lets each module remember its last received settings at startup, for autonomous operation even if it has been disconnected from the master
- Coarse clock synchronization of all modules (within a few seconds)

####Detailed operation
It is a master-slave based system where a master can relate to multiple devices (modules) using the ModuleInterface library.

Each module does not know about any other device. The master contacts each module and retrieves its service contracts for settings, input values and output values. The master will then read and write the values in the contracts regularly at a configurable time interval.

Output values from one module will be delivered to all other modules that have an input value with the same name.

Depending on the type of master used, the settings for all modules can be configured on a GUI (LCD+buttons or similar) belonging to the master, and/or retrieved from a database using the HTTP client. The settings in the database are then typically displayed and modified from web pages.

When polling for settings from the database, the time from the database server is also retrieved, and distributed to modules as a coarse clock synchronization.

The optional persistence functionality lets new settings be kept and automatically updated in EEPROOM for each module. Each module can the start in its previous state immediately, before gaining contact with the master. So each module can continue working after a restart even if the master or network is down.

The ModuleInterface library consists of a collection of classes, and some files with functions for functionality like EEPROM based persistence and master HTTP transfer. The basic classes are ModuleVariable (keeping one setting or input or output value), ModuleVariableSet (keeping a set of settings or input or output values), ModuleInterface (keeping settings, input values, output values and functionality for a module), ModuleInterfaceSet (in the master -- a collection of ModuleInterface objects that are kept synchronized with the modules).

The ModuleInterface code in a master typically uses more storage space and RAM than within a module. It is still fine to run on an Arduino Uno or Nano, but when adding the HTTP client (and implicitly the large required Ethernet and ArduinoJson libraries), it is necessary to step up to an Arduino Mega or similar for the master.

####PJON
The ModuleInterface class that is used by a module, and the ModuleInterfaceSet class that is used by a master, implement transport logic and serialization/deserialization and other functionality, but do not implement any communication. The communication between modules is designed to be handled by deriving a class from each of these two, and letting these classes do the talking by some protocol.

This library is not worth much without a proper communication bus for letting a master and multiple modules talk together. Luckily, we have the brilliant [PJON] (https://github.com/gioblu/PJON) communication bus library created by *Giovanni Blu Mitolo* available, and this has been used as the primary choice for this library. The PJON library can be used for single-wire multi-master bus communication directly between Arduinos with no extra hardware, a brilliant feat. It can also be used for wireless communication, so ModuleInterface modules need not be wired to the master.
The PJON library also supports a lot of different devices, making it a great choice. The ModuleInterface library does only use PJON for single-master communication.

####Module implementation
Each module must declare a global object of a ModuleInterface derived class like the PJONModuleInterface that is part of the library. In the declaration of this object, the contracts (names and data types) for settings, input values and output values are specified as text parameters for simplicity.

The object's loop function must be called regularly. Contracts are exchanged automatically with the master, and settings will arrive shortly. Settings are retrieved from the object with getter functions using the same order as the contract specifies.

Measurements are registered with setter functions, and will be transferred to the master. If there are any inputs, these will be updated, and can be read with getters.

The SensorMonitor example shows the simplicity of creating a module that reads one or more sensors, to have them transported to the master and then to a database where they can be easily retrieved for display and trending in a web page. A master illustrating this is present in the ModuleMasterHttp example, ready for use (just add your own IP settings).

The SensorMonitor example is simply reading a motion detector:

```cpp
#include <PJONModuleInterface.h>
#include <PJONLink.h>

PJONLink<SoftwareBitBang> link(4); // PJON device id 4

PJONModuleInterface interface("SensMon",      // Module name
                              link,           // PJON bus
                              "",             // Settings
                              "",             // Inputs
                              "Motion:b1");   // Outputs (measurements)

// Outputs (measurements) (index/position in outputs list)
#define o_motion_ix 0

#define PIN_MOTIONSENSOR 6 // Motion sensor connected to this pin

void setup() { pinMode(PIN_MOTIONSENSOR, INPUT); link.bus.strategy.set_pin(7); }

void loop() { read_sensors(); interface.update(); }

void read_sensors() {
  interface.outputs.set_value(o_motion_ix, digitalRead(PIN_MOTIONSENSOR));
  interface.outputs.set_updated(); // Flag as completely updated, all values are set
}

```

Adding reading of more sensors is done by adding more output parameters to the input string as a space separated list, like "Motion:b1 Temp:f4" for having a variable named Temp as a 4 byte floating point value. Simple values are supported: boolean (b1), signed and unsigned byte, short and int (i1, u1, i2, u2, i4, u4), and float (f4).

After adding the variable, it must be set in a similar way as the motion in the example.

####License

```cpp
/* Copyright 2016 Fred Larsen

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
```
