//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "../Sensor.h"

#include <string>

#define MAX_SENSORS 4

class AptCorePIR : public Sensor
{
public:
    AptCorePIR();
    ~AptCorePIR();
    void Initialise( const char *configFilename );
    void Loop( const struct AsmClientTask &task, struct AsmClientData &data );

private:
    int GPIO_Read( int pin );

    std::string sensors[MAX_SENSORS];
    int num_sensors;

    int detection_num;
    int detection_active[MAX_SENSORS];
};
