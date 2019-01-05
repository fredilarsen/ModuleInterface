# Protocol description
ModuleInterface is based on different types of packets being transferred on a communication bus like PJON.

## Packet types
The packet types are:
1. **Request packet**. A request is sent from the master to a module when the master needs something from it.
2. **Contract packet**. When requested by the master, a module will send a contract packet to the master for either the settings, inputs or outputs. The contract packet contains the list of variables, including name and value type for each, for either settings, inputs or outputs. It also includes a _contract id_ -- a checksum to make sure that contract changes are detected and automatically updated.
3. **Value packet**. Corresponding to a contract, a set of values is transferred. The value packet contains a contract id, which is checked against the already exchanged contract. If a mismatch is detected, the values are discarded and the contract is nullified so that it will be requested again by the master. The settings and inputs are packets sent by the master to the module according the the contract given by the module, and if the module receives a value packet with a contract id not matching the current contract id (could happen if module is reprogrammed and put back), it will discard the values and set a status flag that will be picked up by the master in the next status request, triggering the master to request a new contract exchange.
4. **Status packet**. The master will request the current status from each module regularly. The status packet contains information about missing time sync, missing settings or inputs, non-matching contracts of settings or inputs, locally modified settings and the uptime and memory status of the module.
5. **TimeSync packet**. This is a packet sent from master to modules. It will be broadcast regularly, but if a module has a status saying that it is missing time, a timesync packet will be sent directly to it.

### Request packet
The request packet is a 1 byte packet sent from master to module, containing an enum with the requested operation, which can be one of:
* mcSendSettingContract
* mcSendInputContract
* mcSendOutputContract
* mcSendSettings
* mcSendInputs
* mcSendOutputs
* mcSendStatus

### Contract packet
The contract packet is sent from module to master on request. It is a variable length packet containing the module's current list of variables and their data types. The packet has the following data:
* 1 byte packet type _mcSetSettingContract_, _mcSetInputContract_ or _mcSetOutputContract_.
* 4 byte contract id.
* 1 byte (unsigned) variable count.
* For each variable:
  1. 1 byte variable type, one of mvtBoolean, mvtUint8, mvtUint16, mvtUint32, mvtInt8, mvtInt16, mvtInt32 or mvtFloat32.
  2. 1 byte variable name length.
  3. 1-N bytes name, _without_ null terminator.

### Value packet
The value packet is sent from module to master for outputs and potentially settings, and from master to module for settings and inputs. It may contain a complete set of values, one for each variable in the contract, and this is the normal case. It may also be reduced to containing only changed or event values. It contains the following data:
* 1 byte packet type _mcSetSettings_, _mcSetInputs_ or _mcSetOutputs_.
* 4 byte contract id.
* 1 byte variable count. This may be 0 if the contract has not been exchanged yet. The uppermost bit in this byte is set when the packet contains events.
* For each variable:
   1. If the variable count is less then the number of variables in the contract: 1 byte variable number (0 - N-1).
   2. 1-4 bytes value according to data type in contract.

### Status packet
This is a 7 byte packet sent from a module to the master, with the following data:
* 1 byte packet type _mcSetStatus_.
* 1 byte with status bits.
* 1 byte boolean flagging out of memory conditions.
* 4 byte uint containing the uptime in seconds.

### TimeSync packet
This is a 5 byte packet sent from the master to modules by broadcast, or directly to a module that reports missing time. The modules use this to adjust the system time. Modules that do not need time can ignore it (a preprocessor define can be used to save a few bytes of code in this case). The packet contains the following data:
* 1 byte packet type _mcSetTime_.
* 4 byte uint32 with UTC (Universal Time Coordinates, seconds since 1970-01-01 GMT).

## HTTP transfer
When setting up a ModuleInterface setup to work with a web server, which is the recommended way to get all benefits, the master will communicate with the web server. The master will create a HTTP connection to the web server, deliver a request to get or set values, then disconnect. 
HTTP GET will be used to get values, and HTTP POST will be used to set values. Values will be transferred in the JSON format.

The different types of requests are:
* Get settings for one module or all modules (depending on available memory in master, getting all at once is most efficient).
* Set specific settings. This is only used where reverse transfer of settings is activated, for example if modules have their own user interface (perhaps only a button) so that the user can change settings locally.
* Set outputs from one module or all modules.
* Trigger storage from current values to the event table.

## Data exchange strategy

### Principles
ModuleInterface is basically based on state, not events. It repeats sending of all data in all directions based on a time interval, and if a value is changed but the packet is lost for some reason (network error, master or module lost power, whatever...), everything will be updated at the next delivery. This makes for a very robust setup, where the worst-case is that response is delayed or that a very brief temporary change is lost.

The repeated sending and tolerance of lost packets give some rules about how to use it. For example, a variable that is 0 most of the time but 1 when requested to lock a door, then 0 again, is not optimally suited for this protocol (or any protocol :-)). A better alternative would be to have a door state that is changed from unlocked to locked. To trigger an action, a counter can be used, so that increasing the counter will make the module execute even if a packet is lost or the module or anything else is down.

One consequence of the full data exchange every time interval is that there is a close to constant bandwidth usage, for good or worse. On a local bus like PJON, this does normally not matter, and it can actually be a good way to avoid surprises. Some systems can work well on low activity but crash on high activity, making it hard to detect during testing.
Value transfer _can_ transfer only changed values, but this is not used so far, of the reasons stated above.

### Interval based transfer
By default, the transfer time interval is set to 10 seconds, but can be changed to suit the setup.
The following actions will be executed continuously:
1. Check if any contracts needs to be updated, and request these if needed.
2. Check if any events have been received, and forward these to any subscribers.

Every interval, the following will be done:
1. Request and receive status from each module.
2. Check if it is time to do time sync, and do this when needed.
3. Request and receive outputs from each module.
4. If HTTP transfer is active, send outputs to the web server.
5. Exchange values from outputs to inputs according to subscriptions in input contracts.
6. Send updated inputs to each module.
7. Request modified settings from each module that flags this in the status.
8. If HTTP transfer is active, send any modified settings to the web server.
9. If HTTP transfer is active, get all settings from the web server.
10. Send settings to each module.

The time usage for this full data exchange is reported as a metric with name TotalTm as an output from the master. The response time for each module and each web request type is also available, making it possible to pinpoint bottlenecks and modules that may be improved. Adding a column for some of these in the timeseries table makes it possible to plot and inspect the behavior over time in the web site.
