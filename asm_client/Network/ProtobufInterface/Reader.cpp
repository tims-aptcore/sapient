//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "Reader.h"

#include <cstdlib>

#define ELPP_DEFAULT_LOGGER "network"
#include "../../Utils/Log.h"

#define READ_BUFFER_SIZE (1024 * 1024)

#include <google/protobuf/util/json_util.h>

namespace ProtobufInterface
{


Reader::Reader() :
    network_stream( nullptr ),
    read_buffer( nullptr ),
    short_timeout( 0 ),
    long_timeout( 0 )
{
}


bool Reader::GetMessage()
{
    // Do once, make sure we have a buffer to read into
    if( read_buffer == nullptr )
    {
        read_buffer = new uint8_t[ READ_BUFFER_SIZE ];
        if( read_buffer == nullptr )
        {
            LOG( ERROR ) << "Reader failed to allocate buffer.";
            return false;
        }
    }

    if( network_stream == nullptr )
    {
        LOG( ERROR ) << "Reader not attached to stream.";
        return false;
    }

    // Protobuf can parse from a stream directly
    // But Sapient uses a non standard 4 byte little endian length field at the beginning. See bsi-flex-335.pdf section 4.2
    unsigned char msg_len_bytes[4];
    size_t bytes_read = 0;
    if( network_stream->Read( msg_len_bytes, 4, bytes_read ) == false )
    {
        LOG( WARNING ) << "Reader connection failed.";
        return false;
    }

    if( bytes_read == 0 )
    {
        // If uncommented then you would see a lot of these.
        //LOG( WARNING ) << "Reader timed out waiting for message size.";
        return false;
    }

    if( bytes_read != 4 )
    {
        LOG( WARNING ) << "Reader read mismatch in number of message size bytes. Wanted 4, got " << bytes_read;
        network_stream->Flush();
        return false;
    }

    uint32_t msg_len = (uint32_t)msg_len_bytes[0] +
                        (((uint32_t)msg_len_bytes[1]) << 8) +
                        (((uint32_t)msg_len_bytes[2]) << 16) +
                        (((uint32_t)msg_len_bytes[3]) << 24);

    //LOG( INFO ) << "Reader msg_len: " << msg_len;

    if( msg_len >= READ_BUFFER_SIZE )
    {
        LOG( WARNING ) << "Reader msg_len is too large. Skipping data.";
        network_stream->Flush();
        return false;
    }

    // Give it a short timeout to ensure all of the message has arrived
    if( network_stream->ReadWithTimeout( read_buffer, msg_len, bytes_read, short_timeout ) == false )
    {
        LOG( WARNING ) << "Reader connection failed.";
        return false;
    }

    if( bytes_read == 0 )
    {
        LOG( WARNING ) << "Reader timed out waiting for message data.";
        return false;
    }

    if( bytes_read != (size_t)msg_len )
    {
        LOG( WARNING ) << "Reader read mismatch in number of message data bytes.";
        return false;
    }

    if( msg.ParseFromArray( read_buffer, msg_len ) )
    {
        //LOG( INFO ) << "Reader has valid message.";

#ifdef WANT_PROTOBUF_DEBUG_JSON
        std::string json;
        google::protobuf::util::MessageToJsonString( msg, &json );
        LOG( INFO ) << json;
#endif

        return true;
    }
    else
    {
        LOG( WARNING ) << "Reader failed to parse message data.";
        network_stream->Flush();
        return false;
    }
}

};
