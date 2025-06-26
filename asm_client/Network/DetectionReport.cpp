//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "DetectionReport.h"
#include "ProtobufInterface/Writer.h"

#define ELPP_DEFAULT_LOGGER "network"
#include "../Utils/Log.h"

#include "sapient_msg/bsi_flex_335_v2_0/sapient_message.pb.h"
namespace sap = sapient_msg::bsi_flex_335_v2_0;

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>


static bool SetTimestamp( google::protobuf::Timestamp* timestamp, DetectionReportData* data )
{
    google::protobuf::Timestamp ts;
    if( !google::protobuf::util::TimeUtil::FromString( data->timestamp, &ts ) )
    {
        LOG( ERROR ) << "DetectionReport failed to convert timestamp.";
        return false;
    }

    timestamp->set_seconds( ts.seconds() );
    timestamp->set_nanos(0);
    return true;
}


static void SetRangeBearing( sap::RangeBearing* rb, const DetectionReportLocationRB *data_rb )
{
    rb->set_range(                   strtof( data_rb->r.c_str(),   NULL ) );
    rb->set_azimuth(                 strtof( data_rb->az.c_str(),  NULL ) );
    rb->set_range_error(             strtof( data_rb->er.c_str(),  NULL ) );
    rb->set_azimuth_error(           strtof( data_rb->eaz.c_str(), NULL ) );

    rb->set_coordinate_system( sap::RANGE_BEARING_COORDINATE_SYSTEM_DEGREES_M );       // TODO check what type used
    rb->set_datum( sap::RANGE_BEARING_DATUM_PLATFORM );
}


static void SetTrackInfo( sap::DetectionReport* det, std::string type, const DetectionReportValue *v )
{
    if( v )
    {
        sap::DetectionReport::TrackObjectInfo *trk = det->add_track_info();
        trk->set_type( type );
        trk->set_value( v->value );
        trk->set_error( stof( v->e ) );
    }
}


static void SetObjectInfo( sap::DetectionReport* det, std::string type, const DetectionReportValue *v )
{
    if( v )
    {
        sap::DetectionReport::TrackObjectInfo *obj = det->add_object_info();
        obj->set_type( type );
        obj->set_value( v->value );
        obj->set_error( stof( v->e ) );
    }
}


static bool SetDetectionReport( sap::DetectionReport* det, DetectionReportData* data )
{
    ulid::ULID rep_id;
    ulid::EncodeTimeNow( rep_id );
    std::string str_rep_id = ulid::Marshal( rep_id );
    det->set_report_id( str_rep_id );

    std::string str_obj_id = ulid::Marshal( data->objectID );
    det->set_object_id( str_obj_id );

    if( !data->taskID.IsZero() )
    {
        std::string str_task_id = ulid::Marshal( data->taskID );
        det->set_task_id( str_task_id );
    }

    if( data->state.length() )
    {
        det->set_state( data->state );
    }

    SetRangeBearing( det->mutable_range_bearing(), data->rangeBearing );

    det->set_detection_confidence( stof( data->detectionConfidence ) );

    SetTrackInfo( det, "confidence", data->trackConfidence );
    SetTrackInfo( det, "speed",      data->trackSpeed );
    SetTrackInfo( det, "az",         data->trackAZ );
    SetTrackInfo( det, "dR",         data->trackDR );
    SetTrackInfo( det, "dAz",        data->trackDAZ );

    SetObjectInfo( det, "dopplerSpeed", data->objectDopplerSpeed );
    SetObjectInfo( det, "dopplerAz",    data->objectDopplerAz );
    SetObjectInfo( det, "majorLength",  data->objectMajorLength );
    SetObjectInfo( det, "majorAxisAz",  data->objectMajorAxisAz );
    SetObjectInfo( det, "minorLength",  data->objectMinorLength );
    SetObjectInfo( det, "height",       data->objectHeight );

    sap::DetectionReport::DetectionReportClassification* dr = nullptr;

    if( data->humanConfidence.length() )
    {
        dr = det->add_classification();
        dr->set_type( "Human" );
        dr->set_confidence( stof( data->humanConfidence ) );
    }

    if( data->vehicleConfidence.length() )
    {
        dr = det->add_classification();
        dr->set_type( "Vehicle" );
        dr->set_confidence( stof( data->vehicleConfidence ) );

        if( data->vehicleTwoWheelConfidence.length() )
        {
            sap::DetectionReport_SubClass* sdr = dr->add_sub_class();
            sdr->set_level( 1 );
            sdr->set_type( "Vehicle Class 2 Wheel" );
            sdr->set_confidence( stof( data->vehicleTwoWheelConfidence ) );
        }

        if( data->vehicleFourWheelConfidence.length() )
        {
            sap::DetectionReport_SubClass* sdr = dr->add_sub_class();
            sdr->set_level( 1 );
            sdr->set_type( "Vehicle Class 4 Wheel" );
            sdr->set_confidence( stof( data->vehicleFourWheelConfidence ) );

            if( data->vehicleFourWheelHeavyConfidence.length() )
            {
                sap::DetectionReport_SubClass* sdr = dr->add_sub_class();
                sdr->set_level( 2 );
                sdr->set_type( "Heavy" );
                sdr->set_confidence( stof( data->vehicleFourWheelHeavyConfidence ) );
            }

            if( data->vehicleFourWheelMediumConfidence.length() )
            {
                sap::DetectionReport_SubClass* sdr = dr->add_sub_class();
                sdr->set_level( 2 );
                sdr->set_type( "Medium" );
                sdr->set_confidence( stof( data->vehicleFourWheelMediumConfidence ) );
            }

            if( data->vehicleFourWheelLightConfidence.length() )
            {
                sap::DetectionReport_SubClass* sdr = dr->add_sub_class();
                sdr->set_level( 2 );
                sdr->set_type( "Light" );
                sdr->set_confidence( stof( data->vehicleFourWheelLightConfidence ) );
            }
        }
    }

    if( data->staticObjectConfidence.length() )
    {
        dr = det->add_classification();
        dr->set_type( "Static Object" );
        dr->set_confidence( stof( data->staticObjectConfidence ) );
    }

    if( data->unknownConfidence.length() )
    {
        dr = det->add_classification();
        dr->set_type( "Unknown" );
        dr->set_confidence( stof( data->unknownConfidence ) );
    }

    sap::DetectionReport_Behaviour* beh = nullptr;

    if( data->humanWalkingConfidence.length() )
    {
        beh = det->add_behaviour();
        beh->set_type( "Walking" );
        beh->set_confidence( stof( data->humanWalkingConfidence ) );
    }

    if( data->humanRunningConfidence.length() )
    {
        beh = det->add_behaviour();
        beh->set_type( "Running" );
        beh->set_confidence( stof( data->humanRunningConfidence ) );
    }

    if( data->humanLoiteringConfidence.length() )
    {
        beh = det->add_behaviour();
        beh->set_type( "Loitering" );
        beh->set_confidence( stof( data->humanLoiteringConfidence ) );
    }

    if( data->humanCrawlingConfidence.length() )
    {
        beh = det->add_behaviour();
        beh->set_type( "Crawling" );
        beh->set_confidence( stof( data->humanCrawlingConfidence ) );
    }

    return true;
}


bool DetectionReport::Write( ProtobufInterface::Writer *w )
{
    sap::SapientMessage msg;

    if( !SetTimestamp( msg.mutable_timestamp(), data ) )
    {
        return false;
    }

    msg.set_node_id( data->nodeID );
    msg.set_destination_id( data->destID );

    if( !SetDetectionReport( msg.mutable_detection_report(), data ) )
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
                LOG( ERROR ) << "Detection Report failed to serialize.";
                return false;
            }
        }
        else
        {
            LOG( ERROR ) << "Detection Report failed to allocate serialize buffer.";
            return false;
        }
    }
    else
    {
        LOG( ERROR ) << "Detection Report message not fully initialized.";
        return false;
    }

    return true;
}
