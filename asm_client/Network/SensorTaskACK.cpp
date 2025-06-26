//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "SensorTaskACK.h"
#include "ProtobufInterface/Writer.h"

#define ELPP_DEFAULT_LOGGER "network"
#include "../Utils/Log.h"

#include "sapient_msg/bsi_flex_335_v2_0/sapient_message.pb.h"
namespace sap = sapient_msg::bsi_flex_335_v2_0;

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>


static bool SetTimestamp( google::protobuf::Timestamp* timestamp, SensorTaskACKData* data )
{
    google::protobuf::Timestamp ts;
    if( !google::protobuf::util::TimeUtil::FromString( data->timestamp, &ts ) )
    {
        LOG( ERROR ) << "SensorTaskAck failed to convert timestamp.";
        return false;
    }

    timestamp->set_seconds( ts.seconds() );
    timestamp->set_nanos(0);
    return true;
}


static bool SetSensorTaskAck( sap::TaskAck* ta, SensorTaskACKData* data )
{
    if( data->taskID.IsZero() )
    {
        // Not been set and its a mandatory field
        return false;
    }
    else
    {
        std::string str_task_id = ulid::Marshal( data->taskID );
        ta->set_task_id( str_task_id );
    }

    if( data->status == "Accepted" )
    {
        ta->set_task_status( sap::TaskAck::TASK_STATUS_ACCEPTED );
    }
    else if( data->status == "Rejected" )
    {
        ta->set_task_status( sap::TaskAck::TASK_STATUS_REJECTED );
    }
    else
    {
        ta->set_task_status( sap::TaskAck::TASK_STATUS_UNSPECIFIED );
    }

    if( data->reason.length() )
    {
        ta->add_reason( data->reason );
    }

    return true;
}


bool SensorTaskACK::Write( ProtobufInterface::Writer *w )
{
    sap::SapientMessage msg;

    if( !SetTimestamp( msg.mutable_timestamp(), data ) )
    {
        return false;
    }

    msg.set_node_id( data->nodeID );
    msg.set_destination_id( data->destID );

    if( !SetSensorTaskAck( msg.mutable_task_ack(), data ) )
    {
        return false;
    }

    if( msg.IsInitialized() )
    {
#ifdef WANT_PROTOBUF_DEBUG_JSON
        std::string json;
        google::protobuf::util::MessageToJsonString( msg, &json );
        LOG( INFO ) << json;
#endif

        uint32_t len = msg.ByteSizeLong();
        uint8_t *p_bytes = new uint8_t[ len + 4 ];
        if( p_bytes != nullptr )
        {
            uint8_t *p = p_bytes;
            *p++ = len & 0xFF;
            *p++ = (len >> 8) & 0xFF;
            *p++ = (len >> 16) & 0xFF;
            *p++ = (len >> 24) & 0xFF;
            if( msg.SerializeToArray( p_bytes + 4, len ) )
            {
                w->writeBytes( p_bytes, len + 4 );
            }
            else
            {
                LOG( ERROR ) << "Sensor Task Ack failed to serialize.";
                return false;
            }
        }
        else
        {
            LOG( ERROR ) << "Sensor Tack Ack failed to allocate serialize buffer.";
            return false;
        }
    }
    else
    {
        LOG( ERROR ) << "Sensor Task Ack message not fully initialized.";
        return false;
    }

    return false;
}
