//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "SensorTask.h"
#include "ProtobufInterface/Writer.h"

#include <string.h>

#include <google/protobuf/util/time_util.h>


static void ReadRangeBearingCone( sap::RangeBearingCone& msg_range_bearing, SensorTaskRangeBearingCone *rangeBearingCone )
{
    rangeBearingCone->r  = std::to_string( msg_range_bearing.range() );
    rangeBearingCone->az = std::to_string( msg_range_bearing.azimuth() );
    rangeBearingCone->he = std::to_string( msg_range_bearing.horizontal_extent() );
}


static void ReadClassFilter( sap::Task::ClassFilter& cf, SensorTaskClassFilter *classFilter )
{
    // Were not using the filters but if you wanted to then the deocde would go here
}


static void ReadBehaviourFilter( sap::Task::BehaviourFilter& bf, SensorTaskBehaviourFilter *behaviourFilter )
{
    // Were not using the filters but if you wanted to then the deocde would go here
}


static void ReadRegion( sap::Task& msg_task, SensorTaskRegion *region )
{
    // The SensorTaskRegion has only one space but the msg_task.region is an array of them.
    // So pick off the first one if it exists.

    int num_regions = msg_task.region_size();
    if( num_regions <= 0 )
    {
        return;
    }

    sap::Task_Region rgn = msg_task.region(0);

    switch( rgn.type() )
    {
        case sap::Task::REGION_TYPE_AREA_OF_INTEREST:
            region->type = "Area of interest";
            break;

        case sap::Task::REGION_TYPE_IGNORE:
            region->type = "Ignore";
            break;

        case sap::Task::REGION_TYPE_BOUNDARY:
            region->type = "Boundary";
            break;

        case sap::Task::REGION_TYPE_MOBILE_NODE_NO_GO_AREA:
            region->type = "No Go Area";
            break;

        case sap::Task::REGION_TYPE_MOBILE_NODE_GO_AREA:
            region->type = "Go Area";
            break;

        case sap::Task::REGION_TYPE_UNSPECIFIED:
        default:
            region->type = "Unspecified";
            break;
    }

    region->regionID   = rgn.region_id();
    region->regionName = rgn.region_name();

    sap::LocationOrRangeBearing region_area = rgn.region_area();
    if( region_area.has_range_bearing() )
    {
        sap::RangeBearingCone msg_range_bearing = region_area.range_bearing();
        ReadRangeBearingCone( msg_range_bearing, &region->rangeBearingCone );
    }

    int num_class_filters = rgn.class_filter_size();
    for( int c=0; c<num_class_filters; c++ )
    {
        sap::Task::ClassFilter cf = rgn.class_filter(c);
        if( cf.type() == "All" )
        {
            ReadClassFilter( cf, &region->classFilterAll );
        }
        else if( cf.type() == "Human" )
        {
            ReadClassFilter( cf, &region->classFilterHuman );
        }
        else if( cf.type() == "Vehicle" )
        {
            ReadClassFilter( cf, &region->classFilterVehicle );
        }
    }

    int num_beh_filters = rgn.behaviour_filter_size();
    for( int b=0; b<num_beh_filters; b++ )
    {
        sap::Task::BehaviourFilter bf = rgn.behaviour_filter(b);
        if( bf.type() == "Walking" )
        {
            ReadBehaviourFilter( bf, &region->behaviourFilterWalking );
        }
        else if( bf.type() == "Running" )
        {
            ReadBehaviourFilter( bf, &region->behaviourFilterRunning );
        }
        else if( bf.type() == "Loitering" )
        {
            ReadBehaviourFilter( bf, &region->behaviourFilterLoitering );
        }
        else if( bf.type() == "Crawling" )
        {
            ReadBehaviourFilter( bf, &region->behaviourFilterCrawling );
        }
    }
}


static void ReadCommand( sap::Task_Command& msg_cmd, SensorTaskCommand* command )
{
    if( msg_cmd.has_request() )
    {
        command->request = msg_cmd.request();
    }
    else if( msg_cmd.has_detection_threshold() )
    {
        command->detectionThreshold = msg_cmd.detection_threshold();
    }
    else if( msg_cmd.has_detection_report_rate() )
    {
        command->detectionReportRate = msg_cmd.detection_report_rate();
    }
    else if( msg_cmd.has_classification_threshold() )
    {
        command->classificationThreshold = msg_cmd.classification_threshold();
    }
    else if( msg_cmd.has_mode_change() )
    {
        command->mode = msg_cmd.mode_change();
    }
    else if( msg_cmd.has_look_at() )
    {
        sap::LocationOrRangeBearing look_at = msg_cmd.look_at();
        if( look_at.has_range_bearing() )
        {
            sap::RangeBearingCone msg_look_at = look_at.range_bearing();
            ReadRangeBearingCone( msg_look_at, &command->rangeBearingCone );
        }
    }
    // Not doing: move_to, patrol, follow nor the command_parameter
}


SensorTask::SensorTask( sap::Task& msg_task )
{
    data = new SensorTaskData();

    std::string tid = msg_task.task_id();
    if( tid.length() == 26 )    // ULID length
    {
        ulid::UnmarshalFrom( tid.c_str(), data->taskID );
    }
    // else it will leave it as '0' from the ULID default constructor

    data->taskName        = msg_task.task_name();
    data->taskDescription = msg_task.task_description();
    data->taskStartTime   = google::protobuf::util::TimeUtil::ToString( msg_task.task_start_time() );
    data->taskEndTime     = google::protobuf::util::TimeUtil::ToString( msg_task.task_end_time() );

    sap::Task::Control control = msg_task.control();
    switch( control )
    {
        case sap::Task::CONTROL_START:
            data->control = "Start";
            break;
        case sap::Task::CONTROL_STOP:
            data->control = "Stop";
            break;
        case sap::Task::CONTROL_PAUSE:
            data->control = "Pause";
            break;

        case sap::Task::CONTROL_UNSPECIFIED:
        default:
            data->control = "Unspecified";
            break;
    }

    ReadRegion( msg_task, &data->region );

    if( msg_task.has_command() )
    {
        sap::Task_Command msg_cmd = msg_task.command();
        ReadCommand( msg_cmd, &data->command );
    }
}
