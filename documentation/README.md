## Design principles

ModuleInterface makes it easy to focus on the automation tasks at hand, and let communication take care of itself.
Each module declares its settings, inputs and outputs and registers these with a ModuleInterface object. This object will then make sure that they are automatically synchronized.

* _Settings_ are the configuration parameters needed for a module to do its work. The idea is that these are configured
outside of the module and synchronized to the module when changed. Typically, configuration is done in a web page.
* _Inputs_ are measurements or results from other modules that may be used instead of a local measurement. For example, a module requiring a measured outdoor temperature can subscribe to receiving this from another module.
* _Outputs_ are measurements and results that the device measures or produces, that are delivered so that other modules may subscribe to them, and so that they may be archived in a database and shown in a web page or plot.

### The Master

The principle is that one Master (or more) is the "hub" for exchanging values. This master may need more resources than the
worker devices, that can be kept minimal, especially if communicating using PJON SWBB without extra hardware. Cheap and
robust (at least if you feed them 5V and not depend on the built-in regulator, that is not extremely reliable) Arduino Nanos are ideal as workers for many tasks,
and a Mega can be used as master for controlling many workers and synchronize with a database that is used by web pages.

The modules do not know about each other or the master. The master will have a list of device ids it shall manage, without
knowing anything about each device before starting and requesting contracts from each module.

The master could also be run on a Windows PC, Linux PC or Raspberry PI using the PJON Surrogate principle to take part in a PJON
bus via Ethernet even if not having the possibility to run the PJON strategy like SWBB directly.

### Autonomy

A single point of failure is not good. The master will at some point be down for restart or maintenance, and it would be a bad
idea to let all workers stop when this happens. So instead of letting each module only do work when asked, it should
use the settings it has and continue doing its work even if communication with the master is temporarily down.

If a module is relying on time to do it's work, the recommended solution is to let it continue without new time synchronization
based on the hardware clock for some time depending on precision requirements, then drop down into a fallback mode if
meaningful. A light controller could drop all logic based on time and fall back to using ambient light measurements and motion
alone.

After a restart it can be meaningful to start in fallback mode based on persisted settings, and then increase to standard mode
when a time sync broadcast is received.

So in the worst case scenario, with a common power failure for all devices, and master not coming up, all modules will start
in fallback mode and do basic work where possible based on previous parameters. For modules without a need for time sync,
it will mean full operation without a master.

### Persistence

To make it easier to create autonomous modules, for example an exterior light controller that has settings for movement
and light intensity, ModuleInterface keeps settings stored in EEPROM so that the
latest registered settings are immediately available after reboot. It can then continue working as it did before the reboot
even if communication is down.
Saving of settings to EEPROM is done in a way that minimizes the wear on the EEPROM.

### Database connection from master

The master can run HTTP calls to a web server for:

* Reading latest settings to be distributed to each module
* Writing latest measurements from all modules
* Getting time from server to be used for clock sync of all modules

The server side can be implemented in many ways, but PHP files for transferring values to a MariDb/MySQL with a specific
table structure is provided to be used as-is or as a good starting point if another database solution is wanted.

### Database connection from web pages

The web pages can do http calls to the web server for:

* Reading latest settings
* Writing one or more setting
* Reading current-values (the latest registered measurements and outputs from the modules)
* Reading historical values for trend plots, with different resolutions

Also for these operations, PHP files are supplied, and these can be used as-is or as a starting point.

### Database structure

The proposed database schema includes the following tables:

* Settings (id, value, modified, description)
* Current values (latest measurements from all modules) (id, value, modified)
* Historical values (time, 4 scan columns, 1 column per tag)

A PHP file that is called from the master will take a snapshot of the current values table and save it as a single row
in the historical values table.

The proposed design of the historical values table, with one column per tag and one row per timestamp is not the most flexible,
as columns must be created manually when a new module is added or an existing module extended with more outputs. BUT this is done
after careful consideration. It offers the following advantage:

* It has a minimal number of rows compared to other alternatives. This minimizes the size of the table itself, and also
the size of the search trees for the keys. As a consequence of this compact size, it is fast.
* Only the tags for which the user has created columns will be archived versus time. The current values table can contain
many outputs that are nice to have but which are unnecessary to archive historically.
* The scan columns allows values to be sampled at different resolutions very fast, for example for plotting with 10 second resolution
or 1 hour resolution equally fast. The resolutions out of the box are 10s, 1m, 10m, 1h and 1d.

### Why the SQL database?

A SQL based database is not usually the best choice for storing time series. The reason this has been selected is because it
is simple to configure and maintain, it is well known, easily integrated with Apache, and it has sufficient compactness and
efficiency with the proposed schema. And it is best suited for the settings and current values.

Of course, the historian part can be replaced with a dedicated time series database (like InfluxDb, OpenTSDB and others) if needed, if maintaining the http request syntax for reading and writing to it.

To use another type of database, change the server-side php scripts.
