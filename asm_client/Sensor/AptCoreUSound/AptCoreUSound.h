//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "../Sensor.h"

#include <string>
#include <vector>

class AptCoreUSound : public Sensor
{
public:
    AptCoreUSound();
    ~AptCoreUSound();
    void Initialise( const char *configFilename );
    void Loop( const struct AsmClientTask &task, struct AsmClientData &data );

private:
    void Process_Tracks( int detector, struct AsmClientData &data );

    int modbus_timeout_us;
    int slave_id;
    class ModbusComms *modbus;
    class Hardware *hware;

    int num_sensors;
    int *amplitude_threshold;
    int *min_range;
    int *max_range;
    int *track_range_diff;
    int *track_lifetime;
    int *det_direction;

    double previous_trigger;
    struct Raw_Detection
    {
        int range;
        int amplitude;
    };
    struct Track_Struct
    {
        int id;
        int range;
        int amplitude;
        int lifetime;
        int matched;
        int active;
    };
    std::vector<std::vector <Raw_Detection> >raw_detections;
    std::vector<std::vector <Track_Struct> >tracks;
};
