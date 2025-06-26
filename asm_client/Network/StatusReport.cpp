//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "StatusReport.h"
#include "ProtobufInterface/Writer.h"

#define ELPP_DEFAULT_LOGGER "network"
#include "../Utils/Log.h"

#include "sapient_msg/bsi_flex_335_v2_0/sapient_message.pb.h"
namespace sap = sapient_msg::bsi_flex_335_v2_0;

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>


static bool SetTimestamp( google::protobuf::Timestamp* timestamp, StatusReportData* data )
{
    google::protobuf::Timestamp ts;
    if( !google::protobuf::util::TimeUtil::FromString( data->timestamp, &ts ) )
    {
        LOG( ERROR ) << "Status failed to convert timestamp.";
        return false;
    }

    timestamp->set_seconds( ts.seconds() );
    timestamp->set_nanos(0);
    return true;
}


static void SetRangeBearing( sap::RangeBearingCone* rbc, StatusReportLocationRBC *data_rbc )
{
    rbc->set_range(                   strtof( data_rbc->r.c_str(),   NULL ) );
    rbc->set_azimuth(                 strtof( data_rbc->az.c_str(),  NULL ) );
    rbc->set_horizontal_extent(       strtof( data_rbc->he.c_str(),  NULL ) );
    rbc->set_vertical_extent(         strtof( data_rbc->ve.c_str(),  NULL ) );
    rbc->set_range_error(             strtof( data_rbc->er.c_str(),  NULL ) );
    rbc->set_azimuth_error(           strtof( data_rbc->eaz.c_str(), NULL ) );
    rbc->set_horizontal_extent_error( strtof( data_rbc->ehe.c_str(), NULL ) );
    rbc->set_vertical_extent_error(   strtof( data_rbc->eve.c_str(), NULL ) );

    rbc->set_coordinate_system( sap::RANGE_BEARING_COORDINATE_SYSTEM_DEGREES_M );
    rbc->set_datum( sap::RANGE_BEARING_DATUM_PLATFORM );
}


static void SetLocationList( sap::LocationList* ll, StatusReportLocationXY **data_ll )
{
    for( int i = 0; data_ll[i] != NULL; i++ )
    {
        sap::Location* loc = ll->add_locations();

        loc->set_x(       stod( data_ll[i]->x  ) );
        loc->set_y(       stod( data_ll[i]->y  ) );
        loc->set_x_error( stod( data_ll[i]->ex ) );
        loc->set_y_error( stod( data_ll[i]->ey ) );
        loc->set_coordinate_system( sap::LOCATION_COORDINATE_SYSTEM_LAT_LNG_DEG_M );
        loc->set_datum( sap::LOCATION_DATUM_WGS84_E );
    }
}


static void SetStatus( sap::StatusReport* sr, std::string level, sap::StatusReport::StatusType type, std::string value )
{
    if( value.length() )
    {
        sap::StatusReport::Status* status = sr->add_status();

        if( level == "Information" )
        {
            status->set_status_level( sap::StatusReport::STATUS_LEVEL_INFORMATION_STATUS );
        }
        else if( level == "Warning" )
        {
            status->set_status_level( sap::StatusReport::STATUS_LEVEL_WARNING_STATUS );
        }
        else if( level == "Error" )
        {
            status->set_status_level( sap::StatusReport::STATUS_LEVEL_ERROR_STATUS );
        }
        else
        {
            // Which includes "Status"
            status->set_status_level( sap::StatusReport::STATUS_LEVEL_UNSPECIFIED );
        }

        status->set_status_type( type );

        status->set_status_value( value );
    }
}


static bool SetStatusReport( sap::StatusReport* sr, StatusReportData* data )
{
    ulid::ULID rep_id;
    ulid::EncodeTimeNow( rep_id );
    std::string str_rep_id = ulid::Marshal( rep_id );
    sr->set_report_id( str_rep_id );

    if( data->system == "Tamper" )
    {
        sr->set_system( sap::StatusReport::SYSTEM_ERROR );        // TAMPER option has been deprecated
    }
    else if( data->system == "OK" )
    {
        sr->set_system( sap::StatusReport::SYSTEM_OK );
    }
    else
    {
        sr->set_system( sap::StatusReport::SYSTEM_UNSPECIFIED );
    }

    if( data->info == "New" )
    {
        sr->set_info( sap::StatusReport::INFO_NEW );
    }
    else if( data->info == "Unchanged" )
    {
        sr->set_info( sap::StatusReport::INFO_UNCHANGED );
    }
    else
    {
        sr->set_info( sap::StatusReport::INFO_UNSPECIFIED );
    }

    if( !data->activeTaskID.IsZero() )
    {
        std::string str_active_task_id = ulid::Marshal( data->activeTaskID );
        sr->set_active_task_id( str_active_task_id );
    }

    sr->set_mode( "default" );

    if( data->powerStatus.length() )
    {
        sap::StatusReport_Power* pow = sr->mutable_power();
        if( data->powerLevel.length() )
        {
            pow->set_level( stoi( data->powerLevel ) );
        }

        if( data->powerSource == "OTHER" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_OTHER );
        }
        else if( data->powerSource == "MAINS" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_MAINS );
        }
        else if( data->powerSource == "INTERNAL_BATTERY" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_INTERNAL_BATTERY );
        }
        else if( data->powerSource == "EXTERNAL_BATTERY" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_EXTERNAL_BATTERY );
        }
        else if( data->powerSource == "GENERATOR" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_GENERATOR );
        }
        else if( data->powerSource == "SOLAR_PV" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_SOLAR_PV );
        }
        else if( data->powerSource == "WIND_TURBINE" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_WIND_TURBINE );
        }
        else if( data->powerSource == "FUEL_CELL" )
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_FUEL_CELL );
        }
        else
        {
            pow->set_source( sap::StatusReport::POWERSOURCE_UNSPECIFIED );
        }

        if( data->powerStatus.length() )
        {
            if( data->powerStatus == "OK" )
            {
                pow->set_status( sap::StatusReport::POWERSTATUS_OK );
            }
            else if( data->powerStatus == "FAULT" )
            {
                pow->set_status( sap::StatusReport::POWERSTATUS_FAULT );
            }
            else
            {
                pow->set_status( sap::StatusReport::POWERSTATUS_UNSPECIFIED );
            }
        }
    }

    if( data->sensorLocation )
    {
        sap::Location* loc = sr->mutable_node_location();
        loc->set_x(       stod( data->sensorLocation->x  ) );
        loc->set_y(       stod( data->sensorLocation->y  ) );
        loc->set_x_error( stod( data->sensorLocation->ex ) );
        loc->set_y_error( stod( data->sensorLocation->ey ) );
        loc->set_coordinate_system( sap::LOCATION_COORDINATE_SYSTEM_LAT_LNG_DEG_M );
        loc->set_datum( sap::LOCATION_DATUM_WGS84_E );
    }

    if( data->fieldOfViewRBC )
    {
        sap::RangeBearingCone* rb = sr->mutable_field_of_view()->mutable_range_bearing();
        SetRangeBearing( rb, data->fieldOfViewRBC );
    }

    if( data->fieldOfViewPolygon )
    {
        sap::LocationList* ll = sr->mutable_field_of_view()->mutable_location_list();
        SetLocationList( ll, data->fieldOfViewPolygon );
    }

    if( data->coverage )
    {
        sap::RangeBearingCone* rb = sr->add_coverage()->mutable_range_bearing();
        SetRangeBearing( rb, data->coverage );
    }

    if( data->obscuration )
    {
        sap::RangeBearingCone* rb = sr->add_obscuration()->mutable_range_bearing();
        SetRangeBearing( rb, data->obscuration );
    }

    SetStatus( sr, "Error",       sap::StatusReport::STATUS_TYPE_INTERNAL_FAULT,     data->internalFault );
    SetStatus( sr, "Error",       sap::StatusReport::STATUS_TYPE_EXTERNAL_FAULT,     data->externalFault );
    SetStatus( sr, "Information", sap::StatusReport::STATUS_TYPE_CLUTTER,            data->clutter );
    SetStatus( sr, "Sensor",      sap::StatusReport::STATUS_TYPE_MOTION_SENSITIVITY, data->motionSensitivity );
    SetStatus( sr, "Sensor",      sap::StatusReport::STATUS_TYPE_PD,                 data->probabilityOfDetection );
    SetStatus( sr, "Sensor",      sap::StatusReport::STATUS_TYPE_FAR,                data->falseAlarmRate );

    return true;
}


bool StatusReport::Write( ProtobufInterface::Writer *w )
{
    sap::SapientMessage msg;

    if( !SetTimestamp( msg.mutable_timestamp(), data ) )
    {
        return false;
    }

    msg.set_node_id( data->nodeID );
    msg.set_destination_id( data->destID );

    if( !SetStatusReport( msg.mutable_status_report(), data ) )
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
                LOG( ERROR ) << "Status failed to serialize.";
                return false;
            }
        }
        else
        {
            LOG( ERROR ) << "Status failed to allocate serialize buffer.";
            return false;
        }
    }
    else
    {
        LOG( ERROR ) << "Status message not fully initialized.";
        return false;
    }

    return true;
}
