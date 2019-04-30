# SAPIENT open source sensor platform

This open-source software is used to make an ASM (Autonomous Sensor Module) for use within a SAPIENT installation (Sensing for Asset Protection, using Integrated Electronic Networked Technology).

## Getting started
The ASM software can be compiled on Linux or Windows. It is intended that each ASM will run on a particular hardware platform, and as such provision has been made within the code to allow a hardware platform to be selected at compile time.

It is intended that new sensors will be added to the software. At compile time, all sensors are compiled for the selected hardware platform, but at run-time only one sensor is selected via the asm_client.conf file.

### Pre-requisites
Linux - Scons is required to use the included build files
Windows - Visual Studio is required to open the included solution

### Building
#### Linux
The hardware platform must be specified using the 'target' parameter to scons. scons should be run from the root directory of the repository, e.g. 'scons target=rpi'. To see what targets are available or to add in new targets, examine the SConstuct file.

In some cases it will be possible to build on the target. e.g: Raspberry Pi installations. In other cases a cross compiler will be used and the resulting images copied onto the target along with the relevant *.conf file.

#### Windows
Open the asm_client.sln Solution and build. The hardware platform must be specified with a preprocessor definition, e.g. TARGET_RPI

### Running the software
In the root directory are a number of *.conf files. There is one for each sensor type. The relevant *.conf file should be renamed asm_client.conf. This could be achieved with a symlink on Linux eg: 'ln -sf aptcore_pir.conf asm_client.conf'

## Adding a new sensor type.
1. Create a directory for the new sensor within the Sensor directory. Create your cpp files within this directory. Use an existing file as a template. The pure abstract functions in Sensor.h will need to be fulfilled in the sensor code. Asm_client.cpp will ultimately call the Constructor, Initialise and Loop functions to read detections from the sensor.

2. If a new Hardware platform is required add a new directory into Hardware and implement the functions defined by Hardware.h in a new derived class. It is not intended that the software for a particular sensor will be able to run on multiple platforms. These hardware functions are intended to provide the ASM status. Sensor interface functions should be defined in the Sensor. For example the Serial functions used in AptCore_USound ASM.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.



