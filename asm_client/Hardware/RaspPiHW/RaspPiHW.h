//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "../Hardware.h"
#include <string>

class RaspPiHW : public Hardware
{
public:
    RaspPiHW();
    ~RaspPiHW();
    void Initialise( const char *configFilename );
    void Loop( struct AsmClientStatus &status, const struct AsmClientData &data );

private:
    double gnssEast;
    double gnssNorth;
    double gnssError;
    float compassBearing;
    float compassBearingError;
};
