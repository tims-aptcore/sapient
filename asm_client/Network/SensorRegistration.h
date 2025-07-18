//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Message.h"
#include "ProtobufInterface/Writer.h"

#include <string>

struct SensorRegistrationData
{
    // Most of the sensor registration message is currently fixed in SensorRegistration.cpp
    // If parameters need to be changed they can be added here and in the configuration file

    std::string timestamp;
    std::string nodeID;
    std::string sensorType;
    std::string heartbeatInterval;
    std::string fieldOfViewType;
};

class SensorRegistration : public Message
{
public:
    // Constructs a SensorRegistration message from the provided data
    SensorRegistration( SensorRegistrationData *d ) : Message( "SensorRegistration" )
    {
        data = d;
    }

    // Write message using ProtobufInterface Writer
    virtual bool Write( ProtobufInterface::Writer *w );

private:
    SensorRegistrationData *data;
};
