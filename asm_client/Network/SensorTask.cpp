//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "SensorTask.h"
#include "XML/Writer.h"

#include <string.h>

static bool ReadRangeBearingCone( XML::Reader *reader, SensorTaskRangeBearingCone *rangeBearingCone )
{
    bool any = true;
    while (any) {

        any = false;
        any |= reader->readStringElement( "R", rangeBearingCone->r );
        any |= reader->readStringElement( "Az", rangeBearingCone->az );
        any |= reader->readStringElement( "hExtent", rangeBearingCone->he );

        // If we didn't read anything we expected, but there is still something to read, read and discard it
        if (!any && !reader->isEndElement()) {
            if (reader->readStartElement()) {
                reader->readEndElement( true );
                any = true;
            }
        }
    }
    return any;
}

static bool ReadClassFilter( XML::Reader *reader, SensorTaskClassFilter *classFilter )
{
    bool any = true;
    while (any) {

        any = false;
        /*
        <parameter name="confidence" operator="Greater Than" value="0.4" />
        <subClassFilter level = "1" type="Vehicle Class" value="4 Wheeled">
            <parameter name="confidence" operator="Greater Than" value="0.2" />
            <subClassFilter level = "2" type="Size" value = "Medium">
                <parameter name="confidence" operator="Greater Than" value="0.3" />
                <subClassFilter level = "3" type="Vehicle Type" value = "Car">
                    <parameter name="confidence" operator="Greater Than" value="0.3" />
                </subClassFilter>
            </subClassFilter>
        </subClassFilter>
        */
    }
    return any;
}

static bool ReadRegion( XML::Reader *reader, SensorTaskRegion *region )
{
    bool any = true;
    while (any) {

        any = false;
        any |= reader->readStringElement( "type", region->type );
        any |= reader->readStringElement( "regionID", region->regionID );
        any |= reader->readStringElement( "regionName", region->regionName );

        if (reader->readStartElement( "rangeBearingCone" )) {
            ReadRangeBearingCone( reader, &region->rangeBearingCone );
            reader->readEndElement( false );
            any = true;
        }

        if (reader->readStartElement( "classFilter" )) {
            std::string type;
            reader->getAttribute( "type", type );

            if (type == "All") {
                ReadClassFilter( reader, &region->classFilterAll );
            }
            if (type == "Human") {
                ReadClassFilter( reader, &region->classFilterHuman );
            }
            if (type == "Vehicle") {
                ReadClassFilter( reader, &region->classFilterVehicle );
            }

            reader->readEndElement( false );
            any = true;
        }

        if (reader->readStartElement( "behaviourFilter" )) {
            std::string type;
            reader->getAttribute( "type", type );

            if (type == "Walking") {
                ReadClassFilter( reader, &region->behaviourFilterWalking );
            }
            if (type == "Running") {
                ReadClassFilter( reader, &region->behaviourFilterRunning );
            }
            if (type == "Loitering") {
                ReadClassFilter( reader, &region->behaviourFilterLoitering );
            }
            if (type == "Crawling") {
                ReadClassFilter( reader, &region->behaviourFilterCrawling );
            }

            reader->readEndElement( false );
            any = true;
        }

        // If we didn't read anything we expected, but there is still something to read, read and discard it
        if (!any && !reader->isEndElement()) {
            if (reader->readStartElement()) {
                any = true;
                reader->readEndElement( true );
            }
        }
    }
    return any;
}

static bool ReadCommand( XML::Reader *reader, SensorTaskCommand *command )
{
    bool any = true;
    while (any) {

        any = false;
        any |= reader->readStringElement( "request", command->request );
        any |= reader->readStringElement( "detectionThreshold", command->detectionThreshold );
        any |= reader->readStringElement( "detectionReportRate", command->detectionReportRate );
        any |= reader->readStringElement( "classificationThreshold", command->classificationThreshold );
        any |= reader->readStringElement( "mode", command->mode );

        if (reader->readStartElement( "lookAt" )) {
            if (reader->readStartElement( "rangeBearingCone" )) {
                ReadRangeBearingCone( reader, &command->rangeBearingCone );
                reader->readEndElement( false );
            }
            reader->readEndElement( false );
            any = true;
        }

        // If we didn't read anything we expected, but there is still something to read, read and discard it
        if (!any && !reader->isEndElement()) {
            if (reader->readStartElement()) {
                any = true;
                reader->readEndElement( true );
            }
        }
    }
    return any;
}

SensorTask::SensorTask( XML::Reader *reader )
{
    data = new SensorTaskData();
    memset( data, 0, sizeof( *data ) );

    bool any = true;
    while (any) {

        any = false;
        any |= reader->readStringElement( "sensorID", data->sensorID );
        any |= reader->readStringElement( "taskID", data->taskID );
        any |= reader->readStringElement( "taskName", data->taskName );
        any |= reader->readStringElement( "taskDescription", data->taskDescription );
        any |= reader->readStringElement( "taskStartTime", data->taskStartTime );
        any |= reader->readStringElement( "taskEndTime", data->taskEndTime );
        any |= reader->readStringElement( "control", data->control );

        if (reader->readStartElement( "region" )) {
            ReadRegion( reader, &data->region );
            reader->readEndElement( false );
            any = true;
        }

        if (reader->readStartElement( "command" )) {
            ReadCommand( reader, &data->command );
            reader->readEndElement( false );
            any = true;
        }

        // If we didn't read anything we expected, but there is still something to read, read and discard it
        if (!any && !reader->isEndElement()) {
            if (reader->readStartElement()) {
                any = true;
                reader->readEndElement( true );
            }
        }
    }
}
