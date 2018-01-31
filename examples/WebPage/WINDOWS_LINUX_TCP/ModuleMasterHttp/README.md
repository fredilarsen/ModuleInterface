# ModuleMasterHttp sketch
This is a slightly modified version of the main ModuleMasterHttp example.
It has a different module list passed to the PJONModuleInterfaceSet constructor, corresponding to the modules included in this complete example setup. It also is set to exchange values between modules every second instead of every 10 seconds.

Please make sure that you change the web server IP address to what you have assigned to your web server.

## Build instructions
The ModuleInterface repo has the PJON and ArduinoJson repositiories as dependencies. You should have these located in the same place as the ModuleInterface repository. This is assumed by the build setups for this example.

### Windows
A solution file (ModuleMasterHttp.sln) for Microsoft Visual Studio 2017 is present. Open this project in Visual Studio and build the Release build. A ModuleMasterHttp.exe file will be produced and will be present in the Release subdirectory.

If you do not have this version of VS, make a new project and set the include path to include MI/src, PJON and ArduinoJson directories.

### Linux
A Makefile is present. Give the command _make_ to start the build process. An executable named ModuleMasterHttp will be produced.
