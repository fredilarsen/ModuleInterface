# ModuleInterface
Configuration and data logging framework for IoT for Arduino and similar, with [PJON][https://github.com/gioblu/PJON] protocol support.

This library is an Arduino-compatible library for configuring and logging data from multiple devices with little code needed.
Add a minimum of code and have measurements from a device pop up in your web page, and change the device configuration from the web page.

The basis can be used with multiple underlying communication protocols, but it has be special support for the [PJON][https://github.com/gioblu/PJON] protocol which is great for communicating over different media in a simple way. The greatest PJON feat is the ability to communicate on a bus between many devices without any communication shields, using only one I/O pin per device.

This library delivers a master-slave setup where many worker modules can exchange values and settings with the master. The master can be a broker that forwards values between modules.

It also has support for putting values (usually measurements) into a MySQL/MariaDb database in a standard LAMP/WAMP setup, if the master is equipped with an Ethernet shield. 
Settings can also be transferred from this database into the devices automatically.
This makes it possible to present values in a web page and change the device configuration as well.

A great basis for a home automation setup, and an alternative to MQTT and others where you have to equip every little device with a communication shield of some sort.

Keep your data at home, unlike some alternatives where everything is done in the cloud.



