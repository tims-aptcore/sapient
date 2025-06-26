//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "SensorRegistration.h"
#include "ProtobufInterface/Writer.h"

#define ELPP_DEFAULT_LOGGER "network"
#include "../Utils/Log.h"

#include "sapient_msg/bsi_flex_335_v2_0/sapient_message.pb.h"
namespace sap = sapient_msg::bsi_flex_335_v2_0;
#define SAP_ICD_VERSION "BSI Flex 335 v2.0"

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>


static bool SetTimestamp( google::protobuf::Timestamp* timestamp, SensorRegistrationData* data )
{
    google::protobuf::Timestamp ts;
    if( !google::protobuf::util::TimeUtil::FromString( data->timestamp, &ts ) )
    {
        LOG( ERROR ) << "Registration failed to convert timestamp.";
        return false;
    }

    timestamp->set_seconds( ts.seconds() );
    timestamp->set_nanos(0);
    return true;
}


static bool SetStatusReport( sap::Registration_StatusReport* status_report,
    std::string category, std::string type, std::string units )
{
    if( category == "sensor" )
    {
        status_report->set_category( sap::Registration_StatusReportCategory_STATUS_REPORT_CATEGORY_SENSOR );
    }
    else if( category == "power" )
    {
        status_report->set_category( sap::Registration_StatusReportCategory_STATUS_REPORT_CATEGORY_POWER );
    }
    else if( category == "mode" )
    {
        status_report->set_category( sap::Registration_StatusReportCategory_STATUS_REPORT_CATEGORY_MODE );
    }
    else if( category == "status" )
    {
        status_report->set_category( sap::Registration_StatusReportCategory_STATUS_REPORT_CATEGORY_STATUS );
    }
    else
    {
        LOG( ERROR ) << "Set Status Report Category is of unknown type.";
        return false;
    }

    status_report->set_type( type );
    status_report->set_units( units );
    status_report->set_on_change( true );

    return true;
}


static void SetStatusDef( sap::Registration_StatusDefinition* status_def, SensorRegistrationData* data )
{
    sap::Registration_Duration* status_interval = status_def->mutable_status_interval();
    status_interval->set_units( sap::Registration::TIME_UNITS_SECONDS );
    status_interval->set_value( stof( data->heartbeatInterval ) );

    sap::Registration_LocationType* loc_def = status_def->mutable_location_definition();
    loc_def->set_location_units( sap::LOCATION_COORDINATE_SYSTEM_UTM_M );
    loc_def->set_location_datum( sap::LOCATION_DATUM_WGS84_E );
    loc_def->set_zone( "30U" );

    sap::Registration_StatusReport* status_report = status_def->add_status_report();
    SetStatusReport( status_report, "sensor", "sensorLocation", "decimal degrees-metres" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "sensor", "fieldOfView",    (data->fieldOfViewType == "UTM") ? "locationList" : "rangeBearingCone" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "sensor", "coverage",       "rangeBearingCone" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "sensor", "obscurations",   "rangeBearingCone" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "power",  "status",         "OK, Fault" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "mode",   "mode",           "Default" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "status", "InternalFault",  "OK, Fault" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "status", "ExternalFault",  "OK, Fault" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "status", "Clutter",        "Low, Medium, High" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "status", "PD",             "probability" );
    status_report = status_def->add_status_report();
    SetStatusReport( status_report, "status", "FAR",            "probability" );
}


static bool SetDetReport( sap::Registration_DetectionReport* det_report,
    std::string category, std::string type, std::string units )
{
    if( category == "detection" )
    {
        det_report->set_category( sap::Registration_DetectionReportCategory_DETECTION_REPORT_CATEGORY_DETECTION );
    }
    else if( category == "object" )
    {
        det_report->set_category( sap::Registration_DetectionReportCategory_DETECTION_REPORT_CATEGORY_OBJECT );
    }
    else
    {
        LOG( ERROR ) << "Set Detection Report Category is of unknown type.";
        return false;
    }

    det_report->set_type( type );
    det_report->set_units( units );
    det_report->set_on_change( true );

    return true;
}


static void SetDetClass( sap::Registration_DetectionClassDefinition* det_class, SensorRegistrationData* data )
{
    det_class->set_confidence_definition( sap::Registration_ConfidenceDefinition_CONFIDENCE_DEFINITION_MULTI_CLASS );

    sap::Registration_ClassDefinition* class_def = det_class->add_class_definition();
    class_def->set_type( "Human" );

    class_def = det_class->add_class_definition();
    class_def->set_type( "Vehicle" );
    sap::Registration_SubClass* subclass = class_def->add_sub_class();

    subclass->set_level( 1 );
    subclass->set_type( "Vehicle Class" );
    subclass->set_units( "4 Wheeled, 2 Wheeled" );

    subclass = subclass->add_sub_class();
    subclass->set_level( 2 );
    subclass->set_type( "Size" );
    subclass->set_units( "Heavy, Medium, Light" );

    class_def = det_class->add_class_definition();
    class_def->set_type( "Static Object" );

    class_def = det_class->add_class_definition();
    class_def->set_type( "Unknown" );
}


static void SetBehaviour( sap::Registration_BehaviourDefinition* behaviour, std::string type )
{
    behaviour->set_type( type );
    behaviour->set_units( "probability" );
}


static void SetDetDef( sap::Registration_DetectionDefinition* det_def, SensorRegistrationData* data )
{
    sap::Registration_LocationType* loc_def = det_def->mutable_location_type();
    loc_def->set_location_units( sap::LOCATION_COORDINATE_SYSTEM_UTM_M );
    loc_def->set_location_datum( sap::LOCATION_DATUM_WGS84_E );
    loc_def->set_zone( "30U" );

    sap::Registration_DetectionReport* det_report = det_def->add_detection_report();
    SetDetReport( det_report, "detection", "confidence",   "probability" );
    det_report = det_def->add_detection_report();
    SetDetReport( det_report, "object",    "dopplerSpeed", "m-s"         );
    det_report = det_def->add_detection_report();
    SetDetReport( det_report, "object",    "state",        "none"        );

    sap::Registration_DetectionClassDefinition* det_class = det_def->add_detection_class_definition();
    SetDetClass( det_class, data );

    sap::Registration_BehaviourDefinition* behaviour = det_def->add_behaviour_definition();
    SetBehaviour( behaviour, "Walking" );
    behaviour = det_def->add_behaviour_definition();
    SetBehaviour( behaviour, "Running" );
    behaviour = det_def->add_behaviour_definition();
    SetBehaviour( behaviour, "Loitering" );
    behaviour = det_def->add_behaviour_definition();
    SetBehaviour( behaviour, "Crawling" );
}


static void SetRegionDef( sap::Registration_RegionDefinition *rgn_def, SensorRegistrationData* data )
{
    rgn_def->add_region_type( sap::Registration::REGION_TYPE_AREA_OF_INTEREST );
    rgn_def->add_region_type( sap::Registration::REGION_TYPE_IGNORE );

    sap::Registration_Duration *st = rgn_def->mutable_settle_time();
    st->set_units( sap::Registration::TIME_UNITS_SECONDS );
    st->set_value( 5.0f );

    sap::Registration_LocationType* area = rgn_def->add_region_area();
    area->set_location_units( sap::LOCATION_COORDINATE_SYSTEM_UTM_M );
    area->set_location_datum( sap::LOCATION_DATUM_WGS84_E );
    area->set_zone( "30U" );

    sap::Registration_ClassFilterDefinition* cfd = rgn_def->add_class_filter_definition();
    cfd->set_type( "All" );

    cfd = rgn_def->add_class_filter_definition();
    cfd->set_type( "Human" );
    sap::Registration_FilterParameter* fp = cfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );

    cfd = rgn_def->add_class_filter_definition();
    cfd->set_type( "Vehicle" );
    fp = cfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );
    sap::Registration_SubClassFilterDefinition* scfd = cfd->add_sub_class_definition();

    scfd->set_level( 1 );
    scfd->set_type( "Vehicle Class" );
    fp = scfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );
    scfd = scfd->add_sub_class_definition();

    scfd->set_level( 2 );
    scfd->set_type( "Size" );
    fp = scfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );

    cfd = rgn_def->add_class_filter_definition();
    cfd->set_type( "Static Object" );
    fp = cfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );

    sap::Registration_BehaviourFilterDefinition* bfd = rgn_def->add_behaviour_filter_definition();
    bfd->set_type( "Walking" );
    fp = bfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );

    bfd = rgn_def->add_behaviour_filter_definition();
    bfd->set_type( "Running" );
    fp = bfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );

    bfd = rgn_def->add_behaviour_filter_definition();
    bfd->set_type( "Loitering" );
    fp = bfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );

    bfd = rgn_def->add_behaviour_filter_definition();
    bfd->set_type( "Crawling" );
    fp = bfd->add_filter_parameter();
    fp->set_parameter( "confidence" );
    fp->add_operators( sap::OPERATOR_GREATER_THAN );
    fp->add_operators( sap::OPERATOR_LESS_THAN );
}


static bool SetCommand( sap::Registration_Command* cmd,
        std::string type, std::string units, std::string completion_time )
{
    cmd->set_units( units );
    sap::Registration_Duration *ct = cmd->mutable_completion_time();
    ct->set_units( sap::Registration::TIME_UNITS_SECONDS );
    ct->set_value( stof( completion_time ) );
    if( type == "Request" )
    {
        cmd->set_type( sap::Registration::COMMAND_TYPE_REQUEST );
    }
    else if( type == "DetectionThreshold" )
    {
        cmd->set_type( sap::Registration::COMMAND_TYPE_DETECTION_THRESHOLD );
    }
    else if( type == "DetectionReportRate" )
    {
        cmd->set_type( sap::Registration::COMMAND_TYPE_DETECTION_REPORT_RATE );
    }
    else if( type == "ClassificationThreshold" )
    {
        cmd->set_type( sap::Registration::COMMAND_TYPE_CLASSIFICATION_THRESHOLD );
    }
    else if( type == "Mode" )
    {
        cmd->set_type( sap::Registration::COMMAND_TYPE_MODE_CHANGE );
    }
    else if( type == "Look At" )
    {
        cmd->set_type( sap::Registration::COMMAND_TYPE_LOOK_AT );
    }
    else
    {
        LOG( ERROR ) << "Set Command Type is not in valid set. Given: " << type;
        return false;
    }

    return true;
}


static void SetTask( sap::Registration_TaskDefinition* task, SensorRegistrationData* data )
{
    task->set_concurrent_tasks( 8 );

    sap::Registration_RegionDefinition *region_def = task->mutable_region_definition();
    SetRegionDef( region_def, data );

    sap::Registration_Command *cmd = task->add_command();
    SetCommand( cmd, "Request",                 "Registration, Reset, Heartbeat, Sensor Time, Stop, Start", "1" );
    SetCommand( cmd, "DetectionThreshold",      "Auto, Low, Medium, High",                                  "1" );
    SetCommand( cmd, "DetectionReportRate",     "Auto, Low, Medium, High",                                  "1" );
    SetCommand( cmd, "ClassificationThreshold", "Auto, Low, Medium, High",                                  "1" );
    SetCommand( cmd, "Mode",                    "Default",                                                  "5" );
    SetCommand( cmd, "Look At",                 "rangeBearingCone",                                         "5" );
}


static void SetModeDef( sap::Registration_ModeDefinition* mode_def, SensorRegistrationData* data )
{
    mode_def->set_mode_name( "Default" );
    mode_def->set_mode_type( sap::Registration_ModeType_MODE_TYPE_PERMANENT );

    sap::Registration_Duration* settle_time = mode_def->mutable_settle_time();
    settle_time->set_units( sap::Registration::TIME_UNITS_SECONDS );
    settle_time->set_value( 5.0f );

    sap::Registration_Duration* max_latency = mode_def->mutable_maximum_latency();
    max_latency->set_units( sap::Registration::TIME_UNITS_SECONDS );
    max_latency->set_value( 1.0f );

    mode_def->set_scan_type( sap::Registration_ScanType_SCAN_TYPE_STEERABLE );

    mode_def->set_tracking_type( sap::Registration_TrackingType_TRACKING_TYPE_TRACKLET );

    sap::Registration_DetectionDefinition* det_def = mode_def->add_detection_definition();
    SetDetDef( det_def, data );

    sap::Registration_TaskDefinition* task = mode_def->mutable_task();
    SetTask( task, data );
}


static bool SetRegistration( sap::Registration* reg, SensorRegistrationData* data )
{
    sap::Registration_NodeDefinition* node_def = reg->add_node_definition();
    if( node_def != nullptr )
    {
        node_def->set_node_type( sap::Registration_NodeType_NODE_TYPE_PASSIVE_RF );        // PIR sensor
        node_def->add_node_sub_type( "PIR sensor" );
    }

    reg->set_icd_version( SAP_ICD_VERSION );
    reg->set_name( "AptCore Sapient Example" );

    sap::Registration_Capability* caps = reg->add_capabilities();
    caps->set_category( "Test" );
    caps->set_type( data->sensorType );

    SetStatusDef( reg->mutable_status_definition(), data );

    sap::Registration_ModeDefinition* mode_def = reg->add_mode_definition();
    SetModeDef( mode_def, data );

    sap::Registration_ConfigurationData* config = reg->add_config_data();
    config->set_manufacturer( "AptCore Ltd" );
    config->set_model( "Sapient PIR" );

    return true;
}


bool SensorRegistration::Write( ProtobufInterface::Writer *w )
{
    sap::SapientMessage msg;

    if( !SetTimestamp( msg.mutable_timestamp(), data ) )
    {
        return false;
    }

    msg.set_node_id( data->nodeID );
    //msg.set_destination_id( data->destID );

    if( !SetRegistration( msg.mutable_registration(), data ) )
    {
        LOG( ERROR ) << "Registration message was not set up.";
        return false;
    }

    if( msg.IsInitialized() )
    {
        LOG( INFO ) << "Registration message initialized correctly.";

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
                LOG( ERROR ) << "Registration failed to serialize.";
                return false;
            }
        }
        else
        {
            LOG( ERROR ) << "Registration failed to allocate serialize buffer.";
            return false;
        }
    }
    else
    {
        LOG( ERROR ) << "Registration message not fully initialized.";
        return false;
    }

    return true;
}
