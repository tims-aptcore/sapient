//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "NetworkStream.h"
#include <cstdint>

#include "sapient_msg/bsi_flex_335_v2_0/sapient_message.pb.h"
namespace sap = sapient_msg::bsi_flex_335_v2_0;


namespace ProtobufInterface
{

class Reader
{
    NetworkStream *network_stream;
    uint8_t *read_buffer;
    int short_timeout;
    int long_timeout;

public:
    sap::SapientMessage msg;

    Reader();

    void setTimeouts( int short_to, int long_to )
    {
        short_timeout = short_to;
        long_timeout = long_to;
    }

    void Attach( NetworkStream *ns )
    {
        network_stream = ns;
    }

    bool GetMessage();
};

};
