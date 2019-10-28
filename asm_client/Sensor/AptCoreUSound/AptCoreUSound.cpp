//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "AptCoreUSound.h"
#include "Modbus.h"
#include "../Sensor.h"
#include "../../AsmClient.h"

#define ELPP_DEFAULT_LOGGER "sensor"
#include "../../Utils/Log.h"
#include "../../Utils/Config.h"
#include "../../Utils/Utils.h"

#ifdef __unix__
#include <unistd.h>
#endif

#define TRIGGER_PERIOD_MS 15

AptCoreUSound::AptCoreUSound()
{
    el::Loggers::getLogger( "sensor" );
    previous_trigger = 0;
}

AptCoreUSound::~AptCoreUSound()
{
}

void AptCoreUSound::Initialise( const char *configFilename )
{
    LOG( INFO ) << "AptCoreUSound Initialise called";

    int index;
    char config_key[32];
    CSimpleIniA config;
    SI_Error rc = config.LoadFile( configFilename );
    if (rc < 0)
    {
        LOG( ERROR ) << "Failed to load config file '" << configFilename << "'";
        throw "config.LoadFile returned " + std::to_string( rc );
    }

    // Extract the required configuration.
    struct HWSerial serial;
    serial.device = config.GetValue( "sensor", "modbus_device", "None" );
    serial.baud = (int)config.GetLongValue( "sensor", "modbus_baud", 0 );

    struct ModbusState mbus;
    mbus.timeout = (int)config.GetLongValue( "sensor", "modbus_timeout_us", 0 );
    mbus.slave_id = (int)config.GetLongValue( "sensor", "modbus_slave_id", 0 );
    mbus.tx_enable_gpio = (int)config.GetLongValue( "sensor", "tx_enable_gpio", 0 );

    if ((serial.device == "None") || (serial.baud == 0) ||
        (mbus.timeout == 0) || (mbus.slave_id == 0) || (mbus.timeout == 0))
    {
        LOG( ERROR ) << "Missing param in config file";
        throw "Missing config param";
    }

    modbus = new ModbusComms( serial, mbus );

    int default_amplitude_threshold = (int)config.GetLongValue( "sensor", "amplitude_threshold", 10 );
    int default_max_range = (int)config.GetLongValue( "sensor", "max_range", 250 );
    int default_min_range = (int)config.GetLongValue( "sensor", "min_range", 15 );
    int default_track_range_diff = (int)config.GetLongValue( "sensor", "track_range_diff", 20 );
    int default_track_lifetime = (int)config.GetLongValue( "sensor", "track_lifetime", 20 );

    //Read the number of sensors.
    num_sensors = config.GetLongValue( "sensor", "num_sensors", 1 );

    //Allow for independant control of threshold and range.
    amplitude_threshold = (int*)malloc( num_sensors * sizeof( amplitude_threshold ) );
    max_range = (int*)malloc( num_sensors * sizeof( max_range ) );
    min_range = (int*)malloc( num_sensors * sizeof( min_range ) );
    track_range_diff = (int*)malloc( num_sensors * sizeof( track_range_diff ) );
    track_lifetime = (int*)malloc( num_sensors * sizeof( track_lifetime ) );
    det_direction = (int*)malloc( num_sensors * sizeof( det_direction ) );

    for (index = 0; index < num_sensors; index++)
    {
        snprintf( config_key, sizeof( config_key ), "amplitude_threshold%d", index );
        amplitude_threshold[index] = (int)config.GetLongValue( "sensor", config_key, default_amplitude_threshold );

        snprintf( config_key, sizeof( config_key ), "max_range%d", index );
        max_range[index] = (int)config.GetLongValue( "sensor", config_key, default_max_range );

        snprintf( config_key, sizeof( config_key ), "min_range%d", index );
        min_range[index] = (int)config.GetLongValue( "sensor", config_key, default_min_range );

        snprintf( config_key, sizeof( config_key ), "track_range_diff%d", index );
        track_range_diff[index] = (int)config.GetLongValue( "sensor", config_key, default_track_range_diff );

        snprintf( config_key, sizeof( config_key ), "track_lifetime%d", index );
        track_lifetime[index] = (unsigned int)config.GetLongValue( "sensor", config_key, default_track_lifetime );

        snprintf( config_key, sizeof( config_key ), "det_direction%d", index );
        det_direction[index] = (int)config.GetLongValue( "sensor", config_key, 0 );
    }

    // Initialise the vector for number of results
    raw_detections.resize( num_sensors );
    tracks.resize( num_sensors );
}

void AptCoreUSound::Loop( const struct AsmClientTask &task, struct AsmClientData &data )
{
    ModbusErrno returncode;
    int d, function, adu_length, datapoint, delay_ms;
    uint8_t *adu;

    // Clear out all 'updated' flags before the loop for each sensor
    for (d = 0; d < (int)data.detections.size(); d++)
    {
        data.detections[d].updated = false;
    }
    // Cycle around the sensors triggering and reading.
    for (d = 0; d < num_sensors; d++)
    {
        // Clear all the previous detections.
        raw_detections[d].clear();

        // Trigger for this detector
        function = (0x20 | 1 << d);
        returncode = modbus->SendADU( function, NULL, 0 );
        if (returncode != MODBUS_ERR_NO_ERROR)
        {
            LOG( ERROR ) << "Modbus Failure: " << returncode;
        }
        // Receive the response to the trigger.
        returncode = modbus->ReceiveADU( function, &adu, &adu_length );
        if (returncode != MODBUS_ERR_NO_ERROR)
        {
            LOG( ERROR ) << "Modbus Rx Failure: " << returncode;
        }
        // Wait for 15msec before reading.
        delay_ms = (int)(TRIGGER_PERIOD_MS - (1e3 * (Get_Time_Monotonic() - previous_trigger)));
        if (delay_ms > 0) { Sleep_ms( delay_ms ); }

        // Read the result.
        function = (0x40 | 1 << d);
        returncode = modbus->SendADU( function, NULL, 0 );
        if (returncode != MODBUS_ERR_NO_ERROR)
        {
            LOG( ERROR ) << "Modbus Failure: " << returncode;
        }
        // Receive the response to the read.
        returncode = modbus->ReceiveADU( function, &adu, &adu_length );
        if (returncode != MODBUS_ERR_NO_ERROR)
        {
            LOG( ERROR ) << "Modbus Rx Failure: " << returncode;
        }

        // Check for detections from this sensor. Format of response:
        // range[], amplitude[].
        for (datapoint = 0; datapoint < (adu_length / 2); datapoint++)
        {
            int range = adu[datapoint];
            int amplitude = adu[(datapoint + adu_length) / 2];

            // Check for detections above threshold
            if (amplitude > amplitude_threshold[d] &&
                range > min_range[d] &&
                range < max_range[d])
            {
                struct Raw_Detection raw_data;
                raw_data.amplitude = amplitude;
                raw_data.range = range;
                raw_detections[d].push_back( raw_data );
            }
        }
        // Once we've pulled in all the detections we can process the data and report any tracks.
        Process_Tracks( d, data );
    }

    // Clear out any tracks not updated. This is independant of detector number so
    // needs to be outside the loop for each detector.
    int det;
    for (det = 0; det < (int)data.detections.size(); det++)
    {
        if (data.detections[det].updated == false)
        {
            data.detections.erase( data.detections.begin() + det-- );
        }
    }

    // Setup the timestamp
    data.timestamp = Get_Timestamp( std::chrono::system_clock::now() );
}

void AptCoreUSound::Process_Tracks( int detector, struct AsmClientData &data )
{
    int track_loop, raw_det_loop;
    int match_found;
    int d;
    static unsigned int uid = 0;

    // Search through provided raw data for a reasonable match
    for (raw_det_loop = 0; raw_det_loop < (int)raw_detections[detector].size(); raw_det_loop++)
    {
        match_found = 0;
        // Cycle through the tracks
        for (track_loop = 0; track_loop < (int)tracks[detector].size(); track_loop++)
        {
            // Check for a close match on range.
            if ((abs( tracks[detector][track_loop].range - raw_detections[detector][raw_det_loop].range )
                <= track_range_diff[detector]) &&
                tracks[detector][track_loop].matched == 0)
            {
                tracks[detector][track_loop].matched = 1;
                match_found++;
                // Found something close. Update range and lifetime.
                tracks[detector][track_loop].range = raw_detections[detector][raw_det_loop].range;
                tracks[detector][track_loop].lifetime += 2; // 1 is subtracted later, 2 gives an incrementing counter;
                if (tracks[detector][track_loop].lifetime > track_lifetime[detector])
                {
                    // The track is keeping a maintaining a lifetime. Cap it and report it.
                    tracks[detector][track_loop].lifetime = track_lifetime[detector];
                    // Asign it an ID
                    if (tracks[detector][track_loop].id == 0)
                    {
                        uid++;
                        if (uid == 0) // just in case of rollover...
                        {
                            uid = 1;
                        }
                        tracks[detector][track_loop].id = uid;
                    }
                    // Report the track
                    tracks[detector][track_loop].active = 1;
                }
                else
                {
                    tracks[detector][track_loop].active = 0;
                }
                break; // no need to search the rest of the tracks.
            }
        }
        if (match_found == 0)
        {
            struct Track_Struct new_track;
            new_track.id = 0;
            new_track.range = raw_detections[detector][raw_det_loop].range;
            new_track.amplitude = raw_detections[detector][raw_det_loop].amplitude;
            new_track.lifetime = 2; // Will be decremented shortly.
            new_track.active = 0;
            tracks[detector].push_back( new_track );
        }
    }

    // Go back through tracks (without detections) to decrement and report.
    for (track_loop = 0; track_loop < (int)tracks[detector].size(); track_loop++)
    {
        struct AsmClientData::Detection *detection = nullptr;
        tracks[detector][track_loop].lifetime--;
        tracks[detector][track_loop].matched = 0;

        if (tracks[detector][track_loop].active > 0)
        {
            for (d = 0; d < (int)data.detections.size(); d++)
            {
                if (tracks[detector][track_loop].id == data.detections[d].id)
                {
                    detection = &data.detections[d];
                }
            }
            if (detection == nullptr)
            {
                AsmClientData::Detection newDetection;
                newDetection.id = tracks[detector][track_loop].id;
                data.detections.push_back( newDetection );
                detection = &data.detections.back();
            }
            detection->updated = true;
            detection->range = (float)((float)tracks[detector][track_loop].range / 100);
            detection->direction = (float)det_direction[detector];
            detection->directionError = 90;
            detection->dopplerSpeed = 0;
            detection->detectionConfidence = 1;
            detection->humanConfidence = 0;
            detection->vehicleConfidence = 0;
            detection->unknownConfidence = 1;
            LOG( INFO ) << "TRACK. Detector: " << detector << " ID: "
                << detection->id << " Range: "
                << detection->range;
        }

        // Kill tracks with a life of 0
        if (tracks[detector][track_loop].lifetime == 0)
        {
            tracks[detector].erase( tracks[detector].begin() + track_loop );
        }
    }
}
