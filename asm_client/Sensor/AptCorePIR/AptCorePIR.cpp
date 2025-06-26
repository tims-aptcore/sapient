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
#include "../../Utils/Ulid.h"

AptCorePIR::AptCorePIR()
{
    el::Loggers::getLogger( "sensor" );
}

AptCorePIR::~AptCorePIR()
{
}

void AptCorePIR::Initialise( const char *configFilename )
{
    LOG( INFO ) << "AptCorePIR Initialise called";
    std::string sys_command;
    CSimpleIniA config;
    SI_Error rc = config.LoadFile( configFilename );
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
#ifdef __unix__
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
            detection_num = 0;
            detection_active[t] = 0;
            // Set up the gpio pin for input / pull-hi.
            sys_command = "pinctrl set " + sensors[t].substr( 4 ) + " ip pu";
            const char *sys_command_c = sys_command.c_str();
            LOG( INFO ) << sys_command_c;
            system( sys_command_c );
        }
    }
#else
    LOG( ERROR ) << "Initialisation for GPIO has only been implemented for the Raspberry Pi. You will need to write a GPIO interface for your specific platform.\n";
#endif
}

void AptCorePIR::Loop( const struct AsmClientTask &task, struct AsmClientData &data )
{
    data.detections.resize( num_sensors );
    data.timestamp = Get_Timestamp( std::chrono::system_clock::now() );
    int num_detections = 0;

    for (int t = 0; t < num_sensors; t++) // cycle through the sensors checking for detections.
    {
        if (GPIO_Read_Raspi( t ) != 0)
        {
            if (detection_active[t] == 0)
            {
                struct AsmClientData::Detection *detection = &data.detections[num_detections];
                detection_active[t] = 1;
                detection_num++;

                // Protobuf interface now uses ULID's for object IDs.
                //detection->id = detection_num;
                ulid::ULID det_ulid;
                ulid::EncodeTimeNow( det_ulid );
                detection->id = det_ulid;

                LOG( INFO ) << "Detection sensor: " << t + 1 << " Detection number: " << detection_num;
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

int AptCorePIR::GPIO_Read_Raspi( int sensor )
{
    int retval = 0;

    std::string str_gpio( sensors[sensor].substr(4) );
    int gpio_num = std::stoi( str_gpio );
    if( gpio_num < 0 || gpio_num > 53 )
    {
        LOG( ERROR ) << "GPIO Pin number is out of range";
        return 0;
    }

#ifdef __unix__
    char cmd_res[64] = { 0 };
    std::string sys_command = "pinctrl get " + std::to_string( gpio_num );
    const char *sys_command_c = sys_command.c_str();

    // We need the result of the sys command
    FILE *fp = popen( sys_command_c, "r" );
    if( fp )
    {
        fgets( cmd_res, 64, fp );

        pclose( fp );
        fp = NULL;
    }

    // E.g: 4: ip    -- | lo // GPIO4 = input
    std::string s( cmd_res );
    std::string::size_type pos = s.find( "| " );
    if( pos == std::string::npos )
    {
        LOG( ERROR ) << "GPIO read result is malformed";
        return 0;
    }

    if( s[ pos+2 ] != 'l' )        // for "lo"
    {
        retval = 1;
    }
#else
    LOG( ERROR ) << "GPIO has only been implemented for the Raspberry Pi. You will need to write a GPIO interface for your specific platform.\n";
#endif

    return retval;
}
