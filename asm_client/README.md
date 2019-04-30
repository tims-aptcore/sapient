# SAPIENT open source sensor platform

This open-source software is used to make an ASM (Autonomous Sensor Module) for use within a SAPIENT (Sensing for Asset Protection, using Integrated Electronic Networked Technology) installation.

## Getting started
The ASM software can be compiled on Linux or Windows. It is intended that each ASM will run on a particular hardware platform, and as such provision should be made within the code to allow this.

It is intended that new sensors will be added to the software. At compile time, all sensors are compiled but at run-time only one is selected via the asm_client.conf file.

### Pre-requisites
Linux - Scons whould be installed.
Windows - Visual Studio should be used to build the ASM.

### Building
#### Linux
It is necessary to provide a target hardware setup for scons. scons should be run from root directory of the repository: 'scons target=rpi'. To examine what targets are available or to add in new targets examine the SConstuct file.

In some cases it will be possible to build on the target. e.g: Raspberry Pi installations. In other cases a cross compiler will be used and the resulting images copied onto the target along with the relevant *.conf file.

#### Windows
Open the asm_client.sln Solution and build.

### Running the software
In the root directory are a number of *.conf files. There is one for each sensor type. The relevant *.conf file should be renamed asm_client.conf. This could be achieved with a symlink on Linux eg: 'ln -sf aptcore_pir.conf asm_client.conf'

## Adding a new sensor type.
1. Create a directory for the new sensor within the Sensor directory. Create your cpp files within this directory. Use an existing file as a template. The pure abstract functions in Sensor.h will need to be fulfilled in the sensor code. Asm_client.cpp will ultimately call the new Initialise and Loop functions.

2. If a new Hardware platform is required add a new directory into Hardware and realise the functions the Hardware.h in the new code. It is not intended that the software for an ASM will be able to run on multiple platforms. These hardware functions are intended for general initialisation. Hardware functions specific to as ASM should be put in the Sensor. For example the Serial functions used in AptCore_USound ASM. 

## License
This project is licensed under teh MIT Licesnse - see the [LICENSE.md](LICENSE.md) file for details.



