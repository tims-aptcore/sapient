//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "AptCorePIR.h"
#include "../Sensor.h"
#include "../../AsmClient.h"
#include "../../Hardware/Hardware.h"

#define ELPP_DEFAULT_LOGGER "sensor"
#include "../../Utils/Log.h"
#include "../../Utils/Config.h"
#include "../../Utils/Utils.h"

AptCorePIR::AptCorePIR()
{
    el::Logger* mainLogger = el::Loggers::getLogger( "sensor" );
}

AptCorePIR::~AptCorePIR()
{
}

void AptCorePIR::Initialise( const char *configFilename )
{
    LOG( INFO ) << "AptCorePIR Initiliase called";
    std::string sys_command;
    CSimpleIniA config;
    SI_Error rc = config.LoadFile( configFilename );
    const char *sys_command_c;
    if (rc < 0)
    {
        LOG( ERROR ) << "Failed to load config file '" << configFilename << "'";
        throw "config.LoadFile returned " + std::to_string( rc );
    }

    num_sensors = config.GetLongValue( "sensor", "num_sensors", 4 );
    if (num_sensors > MAX_SENSORS)
    {
        throw "num_sensors config exceeds MAX_SENSORS";
    }
    else
    {
        LOG( INFO ) << "Configured for " << num_sensors << " sensors.";
    }
    for (int t = 0; t < num_sensors; t++)
    {
        std::string Config_name = "sensor" + std::to_string( t + 1 );
        sensors[t] = config.GetValue( "sensor", Config_name.c_str(), "None" );
        if (sensors[t] == "None")
        {
            LOG( ERROR ) << "Unable to get config for sensor: " << Config_name;
        }
        else
        {
            detection_num[t] = 0;
            detection_active[t] = 0;
            // Initialise the gpio pins. Output with pull-ups.
            sys_command = "echo " + sensors[t].substr( 4 ) + " > /sys/class/gpio/export";
            sys_command_c = sys_command.c_str();
#ifdef __unix__
            system( sys_command_c );
#else
            LOG( INFO ) << sys_command_c;
#endif
            // Set up the gpio for input/ pull-hi. This is special command for rasp pi.
            sys_command = "sudo raspi-gpio set " + sensors[t].substr( 4 ) + " pu";
            sys_command_c = sys_command.c_str();
#ifdef __unix__
            system( sys_command_c );
#else
            LOG( INFO ) << sys_command_c;
#endif
        }

    }
}

void AptCorePIR::Loop( const struct AsmClientTask &task, struct AsmClientData &data )
{
    data.detections.resize( num_sensors );
    data.timestamp = Get_Timestamp( std::chrono::system_clock::now() );
    int num_detections = 0;

    for (int t = 0; t < num_sensors; t++) // cycle through the sensors checking for detections.
    {
        if (GPIO_Read( t ) != 0)
        {
            if (detection_active[t] == 0)
            {
                struct AsmClientData::Detection *detection = &data.detections[num_detections];
                detection_active[t] = 1;
                detection_num[t]++;
                detection->id = detection_num[t];
                LOG( INFO ) << "Detection sensor: " << t + 1 << " Detection number: " << detection_num[t];
                detection->range = 6;
                detection->direction = (float)(t * 90);
                detection->directionError = 45;
                detection->dopplerSpeed = 0;
                detection->detectionConfidence = 1;
                detection->humanConfidence = 0;
                detection->vehicleConfidence = 0;
                detection->unknownConfidence = 1;
                num_detections++;
            }
        }
        else
        {
            detection_active[t] = 0;
        }
    }
    data.detections.resize( num_detections );
}

int AptCorePIR::GPIO_Read( int sensor )
{
    int retval;
    std::string value;
    std::string getval_str = "/sys/class/gpio/" + sensors[sensor] + "/value";
#ifdef __unix__
    std::ifstream getvalgpio( getval_str.c_str() );

    if (!getvalgpio.is_open()) {
        LOG( ERROR ) << "Unable to open gpio file";
    }

    getvalgpio >> value;

    if (value != "0")
    {
        retval = 1;
    }
    else
    {
        retval = 0;
    }
#else
    retval = 0;
#endif
    return retval;
}
