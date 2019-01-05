# Protocol description
ModuleInterface is based on different types of packets being transferred on a communication bus like PJON. The packet types are:
1. **Request packet**. A request is sent from the master to a module when the master needs something from it.
2. **Contract packet**. When requested by the master, a module will send a contract packet to the master for either the settings, inputs or outputs. The contract packet contains the list of variables, including name and value type for each, for either settings, inputs or outputs. It also includes a _contract id_ -- a checksum to make sure that contract changes are detected and automatically updated.
3. **Value packet**. Corresponding to a contract, a set of values is transferred. The value packet contains a contract id, which is checked against the already exchanged contract. If a mismatch is detected, the values are discarded and the contract is nullified so that it will be requested again by the master. The settings and inputs are packets sent by the master to the module according the the contract given by the module, and if the module receives a value packet with a contract id not matching the current contract id (could happen if module is reprogrammed and put back), it will discard the values and set a status flag that will be picked up by the master in the next status request, triggering the master to request a new contract exchange.
4. **Status packet**. The master will request the current status from each module regularly. The status packet contains information about missing time sync, missing settings or inputs, non-matching contracts of settings or inputs, locally modified settings and the uptime and memory status of the module.
5. **TimeSync packet**. This is a packet sent from master to modules. It will be broadcast regularly, but if a module has a status saying that it is missing time, a timesync packet will be sent directly to it.

## Request packet
The request packet is a 1 byte packet sent from master to module, containing an enum with the requested operation, which can be one of:
* mcSendSettingContract
* mcSendInputContract
* mcSendOutputContract
* mcSendSettings
* mcSendInputs
* mcSendOutputs
* mcSendStatus

## Contract packet
The contract packet is sent from module to master on request. It is a variable length packet containing the module's current list of variables and their data types. The packet has the following data:
* 1 byte packet type _mcSetSettingContract_, _mcSetInputContract_ or _mcSetOutputContract_.
* 4 byte contract id.
* 1 byte (unsigned) variable count.
* For each variable:
  1. 1 byte variable type, one of mvtBoolean, mvtUint8, mvtUint16, mvtUint32, mvtInt8, mvtInt16, mvtInt32 or mvtFloat32.
  2. 1 byte variable name length
  3. 1-N bytes name.

## Value packet
The value packet is sent from module to master for outputs and potentially settings, and from master to module for settings and inputs. It may contain a complete set of values, one for each variable in the contract, and this is the normal case. It may also be reduced to containing only changed or event values. It contains the following data:
* 1 byte packet type _mcSetSettings_, _mcSetInputs_ or _mcSetOutputs_.
* 4 byte contract id.
* 1 byte variable count. This may be 0 if the contract has not been exchanged yet. The uppermost bit in this byte is set when the packet contains events.
* For each variable:
   1. If the variable count is less then the number of variables in the contract: 1 byte variable number (0 - N-1).
   2. 1-4 bytes value according to data type in contract.

## Status packet
This is a 7 byte packet sent from a module to the master, with the following data:
* 1 byte packet type _mcSetStatus_.
* 1 byte with status bits.
* 1 byte boolean flagging out of memory conditions.
* 4 byte uint containing the uptime in seconds.

## TimeSync packet
This is a 5 byte packet sent from the master to modules by broadcast, or directly to a module that reports missing time. The modules use this to adjust the system time. Modules that do not need time can ignore it (a preprocessor define can be used to save a few bytes of code in this case). The packet contains the following data:
* 1 byte packet type _mcSetTime_.
* 4 byte uint32 with UTC (Universal Time Coordinates, seconds since 1970-01-01 GMT).