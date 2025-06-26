//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Message.h"
#include "ProtobufInterface/Writer.h"
#include "../Utils/Ulid.h"

#include <string>

struct SensorTaskACKData
{
    std::string timestamp;
    std::string nodeID;
    std::string destID;
    ulid::ULID  taskID;     // Optional
    std::string status;     // Optional
    std::string reason;     // Optional
};

class SensorTaskACK : public Message
{
public:
    // Constructs a SensorTaskACK message from the data provided
    SensorTaskACK( SensorTaskACKData *d ) : Message( "SensorTaskACK" )
    {
        data = d;
    }

    // Write message using ProtobufInterface Writer
    virtual bool Write( ProtobufInterface::Writer *w );

private:
    SensorTaskACKData *data;
};
