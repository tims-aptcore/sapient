//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "RaspPiHW.h"
#include "../Hardware.h"
#include "../../AsmClient.h"

#define ELPP_DEFAULT_LOGGER "hardware"
#include "../../Utils/Log.h"
#include "../../Utils/Config.h"
#include "../../Utils/Utils.h"

RaspPiHW::RaspPiHW()
{
    el::Loggers::getLogger( "hardware" );
}

RaspPiHW::~RaspPiHW()
{
}

void RaspPiHW::Initialise( const char *configFilename )
{
    LOG( INFO ) << "Initialising RaspPi Hardware...";

    CSimpleIniA config;
    SI_Error rc = config.LoadFile( configFilename );
    if (rc < 0)
    {
        LOG( ERROR ) << "Failed to load config file '" << configFilename << "'";
        throw "config.LoadFile returned " + std::to_string( rc );
    }

    compassBearing = (float)config.GetDoubleValue( "hardware", "compassBearing", -1 );
    compassBearingError = (float)config.GetDoubleValue( "hardware", "compassBearingError", 5 );
    gnssEast = config.GetDoubleValue( "hardware", "gnssEast", 0 );
    gnssNorth = config.GetDoubleValue( "hardware", "gnssNorth", 0 );
    gnssError = config.GetDoubleValue( "hardware", "gnssError", 5 );
}

void RaspPiHW::Loop( AsmClientStatus &status, const struct AsmClientData &data )
{
    status.compassValid = 0;
    if (compassBearing != -1.0)
    {
        status.compassValid = 1;
        status.compassBearing = compassBearing;
        status.compassBearingError = compassBearingError;
    }
    else
    {
        throw (" Compass bearing needs to be set in config file");
    }

    status.gnssValid = 0;
    if (gnssEast != 0.0 && gnssNorth != 0.0)
    {
        status.gnssValid = 1;
        status.gnssNorth = gnssNorth;
        status.gnssEast = gnssEast;
        status.gnssError = gnssError;
    }
    else
    {
        throw (" GNSS location needs to be set in config file");
    }
}
