//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "AsmClient.h"
#include "Hardware/Hardware.h"
#include "Network/Network.h"
#include "Sensor/Sensor.h"
#include "Utils/Config.h"

// Supported hardware types
#if defined TARGET_RPI
#include "Hardware/RaspPiHW/RaspPiHW.h"
#elif defined TARGET_ZYNQ
#include "Hardware/ZynqHW/ZynqHW.h"
#endif

// Supported sensor types
#if defined TARGET_RPI
#include "Sensor/AptCorePIR/AptCorePIR.h"
#include "Sensor/AptCoreUSound/AptCoreUSound.h"
#elif defined TARGET_ZYNQ
#include "Sensor/AptCoreRadar/AptCoreRadar.h"
#endif

#define ELPP_DEFAULT_LOGGER "main"
#include "Utils/Log.h"

INITIALIZE_EASYLOGGINGPP

#define CONFIG_FILENAME "asm_client.conf"

static int global_shutdown = 0;
#ifdef __unix__
static void main_signal_handler( int signal )
{
    CLOG( INFO, "main" ) << "Interrupt signal received. Shutting down...";
    global_shutdown = 1;
}
#else // windows
BOOL WINAPI main_signal_handler( _In_ DWORD dwCtrlType )
{
    CLOG( INFO, "main" ) << "Interrupt signal received. Shutting down...";
    global_shutdown = 1;
    return TRUE;
}
#endif

int main( int argc, char* argv[] )
{
    struct AsmClientStatus status = { 0 };
    struct AsmClientData data = { 0 };
    struct AsmClientTask task = { 0 };

    Hardware *hardware = nullptr;
    Network *network = nullptr;
    Sensor *sensor = nullptr;

    START_EASYLOGGINGPP( argc, argv );
    el::Logger* mainLogger = el::Loggers::getLogger( "main" );

    CSimpleIniA config;
    SI_Error rc = config.LoadFile( CONFIG_FILENAME );
    if (rc < 0)
    {
        LOG( ERROR ) << "Failed to load config file '" << CONFIG_FILENAME << "'";
        throw "config.LoadFile returned " + std::to_string( rc );
    }

    network = new Network();

    std::string sensorType = config.GetValue( "sensor", "type", "None" );
    // Construct selected sensor type
    if (false) {}
#if defined TARGET_RPI
    else if (sensorType == "AptCorePIR")
    {
        hardware = new RaspPiHW();
        sensor = new AptCorePIR();
    }
    else if (sensorType == "AptCoreUSound")
    {
        hardware = new RaspPiHW();
        sensor = new AptCoreUSound();
    }
#elif defined TARGET_ZYNQ
    else if (sensorType == "AptCoreRadar")
    {
        hardware = new ZynqHW();
        sensor = new AptCoreRadar();
    }
#endif
    else
    {
        LOG( ERROR ) << "Unknown Sensor Type '" << sensorType << "'";
        return 1;
    }

    LOG( INFO ) << "Initialising...";
    try
    {
        hardware->Initialise( CONFIG_FILENAME );
    }
    catch (const char *msg)
    {
        LOG( ERROR ) << "Exception caught while initialising hardware: " << msg;
        return 2;
    }
    try
    {
        network->Initialise( CONFIG_FILENAME );
    }
    catch (const char *msg)
    {
        LOG( ERROR ) << "Exception caught while initialising network: " << msg;
        return 3;
    }
    try
    {
        sensor->Initialise( CONFIG_FILENAME );
    }
    catch (const char *msg)
    {
        LOG( ERROR ) << "Exception caught while initialising sensor: " << msg;
        return 4;
    }

    // Register the signal handler for termination signals
#ifdef __unix__
    signal( SIGINT, main_signal_handler );
#else // windows
    SetConsoleCtrlHandler( main_signal_handler, 1 );
#endif

    LOG( INFO ) << "Running...";
    while (!global_shutdown)
    {
        try
        {
            hardware->Loop( status, data );
        }
        catch (const char *msg)
        {
            LOG( ERROR ) << "Exception caught while running hardware: " << msg;
            break;
        }
        try
        {
            sensor->Loop( task, data );
        }
        catch (const char *msg)
        {
            LOG( ERROR ) << "Exception caught while running sensor: " << msg;
            break;
        }
        try
        {
            network->Loop( status, data, task );
        }
        catch (const char *msg)
        {
            LOG( ERROR ) << "Exception caught while running network: " << msg;
            break;
        }
    }
    LOG( INFO ) << "Terminating...";

    delete hardware;
    delete network;
    delete sensor;

    el::Loggers::flushAll();

    return global_shutdown ? 0 : 5;
}
