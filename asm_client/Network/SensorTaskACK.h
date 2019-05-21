//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Message.h"
#include "XML/Writer.h"

#include <string>

struct SensorTaskACKData
{
    std::string timestamp;
    std::string sensorID;   // Optional
    std::string taskID;     // Optional
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

    // Write message using XML Writer
    virtual bool Write( XML::Writer *w );

private:
    SensorTaskACKData *data;
};
