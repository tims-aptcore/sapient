//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "SensorRegistration.h"
#include "XML/Writer.h"

static void WriteLocationType( XML::Writer *w, std::string type )
{
    w->writeStartElement( "locationType" );
    w->writeAttribute( "units", "decimal degrees-metres" );
    if (type == "GPS") {
        w->writeAttribute( "datum", "WGS84" );
    }
    if (type == "UTM") {
        w->writeAttribute( "zone", "30U" );
    }
    w->writeAttribute( "north", "Grid" );
    w->writePCData( type.c_str() );
    w->writeEndElement();
}

static void WriteHeartbeatReport( XML::Writer *w, const char *category, const char *type, const char *units )
{
    w->writeStartElement( "heartbeatReport" );
    w->writeAttribute( "category", category );
    w->writeAttribute( "type", type );
    w->writeAttribute( "units", units );
    w->writeAttribute( "onChange", "true" );
    w->writeEndElement();
}

static void WriteHeartbeatDefinition( XML::Writer *w, const SensorRegistrationData *data )
{
    w->writeStartElement( "heartbeatDefinition" );

    w->writeStartElement( "heartbeatInterval" );
    w->writeAttribute( "units", "seconds" );
    w->writeAttribute( "value", data->heartbeatInterval );
    w->writeEndElement();

    w->writeStartElement( "sensorLocationDefinition" );
    WriteLocationType( w, "UTM" );
    w->writeEndElement();

    w->writeStartElement( "fieldOfViewDefinition" );
    WriteLocationType( w, data->fieldOfViewType );
    w->writeEndElement();

    w->writeStartElement( "coverageDefinition" );
    WriteLocationType( w, "RangeBearing" );
    w->writeEndElement();

    w->writeStartElement( "obscurationDefinition" );
    WriteLocationType( w, "RangeBearing" );
    w->writeEndElement();

    WriteHeartbeatReport( w, "sensor", "sensorLocation", "decimal degrees-metres" );
    WriteHeartbeatReport( w, "sensor", "fieldOfView", (data->fieldOfViewType == "UTM") ? "locationList" : "rangeBearingCone" );
    WriteHeartbeatReport( w, "sensor", "coverage", "rangeBearingCone" );
    WriteHeartbeatReport( w, "sensor", "obscurations", "rangeBearingCone" );
    WriteHeartbeatReport( w, "power", "status", "OK, Fault" );
    WriteHeartbeatReport( w, "mode", "", "Default" );
    WriteHeartbeatReport( w, "status", "InternalFault", "OK, Fault" );
    WriteHeartbeatReport( w, "status", "ExternalFault", "OK, Fault" );
    WriteHeartbeatReport( w, "status", "Clutter", "Low, Medium, High" );
    WriteHeartbeatReport( w, "status", "PD", "probability" );
    WriteHeartbeatReport( w, "status", "FAR", "probability" );

    w->writeEndElement();
}

static void WriteDetectionReport( XML::Writer *w, const char *category, const char *type, const char *units )
{
    w->writeStartElement( "detectionReport" );
    w->writeAttribute( "category", category );
    w->writeAttribute( "type", type );
    w->writeAttribute( "units", units );
    w->writeEndElement();
}

static void WriteConfidence( XML::Writer *w )
{
    w->writeStartElement( "confidence" );
    w->writeAttribute( "units", "probability" );
    w->writeEndElement();
}

static void WriteDetectionClassDefinition( XML::Writer *w )
{
    w->writeStartElement( "detectionClassDefinition" );

    w->writeStringElement( "confidenceDefinition", "Multiple Class" );

    w->writeStartElement( "classDefinition" );
    w->writeAttribute( "type", "Human" );
    WriteConfidence( w );
    w->writeEndElement();

    w->writeStartElement( "classDefinition" );
    w->writeAttribute( "type", "Vehicle" );
    WriteConfidence( w );
    {
        w->writeStartElement( "subClassDefinition" );
        w->writeAttribute( "level", "1" );
        w->writeAttribute( "type", "Vehicle Class" );
        w->writeAttribute( "values", "4 Wheeled, 2 Wheeled" );
        WriteConfidence( w );
        {
            w->writeStartElement( "subClassDefinition" );
            w->writeAttribute( "level", "2" );
            w->writeAttribute( "type", "Size" );
            w->writeAttribute( "values", "Heavy, Medium, Light" );
            WriteConfidence( w );
            w->writeEndElement();
        }
        w->writeEndElement();
    }
    w->writeEndElement();

    w->writeStartElement( "classDefinition" );
    w->writeAttribute( "type", "Static Object" );
    WriteConfidence( w );
    w->writeEndElement();

    w->writeStartElement( "classDefinition" );
    w->writeAttribute( "type", "Unknown" );
    WriteConfidence( w );
    w->writeEndElement();

    w->writeEndElement();
}

static void WriteBehaviourDefinition( XML::Writer *w, const char *type )
{
    w->writeStartElement( "behaviourDefinition" );
    w->writeAttribute( "type", type );

    w->writeStartElement( "confidence" );
    w->writeAttribute( "units", "probability" );
    w->writeEndElement();

    w->writeEndElement();
}

static void WriteDetectionDefinition( XML::Writer *w )
{
    w->writeStartElement( "detectionDefinition" );

    WriteLocationType( w, "RangeBearing" );

    WriteDetectionReport( w, "detection", "confidence", "probability" );
    WriteDetectionReport( w, "object", "dopplerSpeed", "m-s" );
    WriteDetectionReport( w, "object", "state", "none" );

    WriteDetectionClassDefinition( w );

    WriteBehaviourDefinition( w, "Walking" );
    WriteBehaviourDefinition( w, "Running" );
    WriteBehaviourDefinition( w, "Loitering" );
    WriteBehaviourDefinition( w, "Crawling" );

    w->writeEndElement();
}

static void WriteFilterParameter( XML::Writer *w )
{
    w->writeStartElement( "filterParameter" );
    w->writeAttribute( "name", "confidence" );
    w->writeAttribute( "operators", "Greater Than, Less Than" );
    w->writeEndElement();
}

static void WriteBehaviourFilterDefinition( XML::Writer *w, const char *type )
{
    w->writeStartElement( "behaviourFilterDefinition" );
    w->writeAttribute( "type", type );
    WriteFilterParameter( w );
    w->writeEndElement();
}

static void WriteRegionDefinition( XML::Writer *w, const SensorRegistrationData *data )
{
    w->writeStartElement( "regionDefinition" );

    w->writeStringElement( "regionType", "Area of Interest" );
    w->writeStringElement( "regionType", "Ignore" );

    w->writeStartElement( "settleTime" );
    w->writeAttribute( "units", "seconds" );
    w->writeAttribute( "value", "5" );
    w->writeEndElement();

    WriteLocationType( w, "RangeBearing" );

    w->writeStartElement( "classFilterDefinition" );
    w->writeAttribute( "type", "All" );
    w->writeEndElement();

    w->writeStartElement( "classFilterDefinition" );
    w->writeAttribute( "type", "Human" );
    WriteFilterParameter( w );
    w->writeEndElement();

    w->writeStartElement( "classFilterDefinition" );
    w->writeAttribute( "type", "Vehicle" );
    WriteFilterParameter( w );
    {
        w->writeStartElement( "subClassFilterDefinition" );
        w->writeAttribute( "level", "1" );
        w->writeAttribute( "type", "Vehicle Class" );
        WriteFilterParameter( w );
        {
            w->writeStartElement( "subClassFilterDefinition" );
            w->writeAttribute( "level", "2" );
            w->writeAttribute( "type", "Size" );
            WriteFilterParameter( w );
            w->writeEndElement();
        }
        w->writeEndElement();
    }
    w->writeEndElement();

    w->writeStartElement( "classFilterDefinition" );
    w->writeAttribute( "type", "Static Object" );
    WriteFilterParameter( w );
    w->writeEndElement();

    WriteBehaviourFilterDefinition( w, "Walking" );
    WriteBehaviourFilterDefinition( w, "Running" );
    WriteBehaviourFilterDefinition( w, "Loitering" );
    WriteBehaviourFilterDefinition( w, "Crawling" );

    w->writeEndElement();
}

static void WriteCommand( XML::Writer *w, const char *name, const char *units, const char *completionTime )
{
    w->writeStartElement( "command" );
    w->writeAttribute( "name", name );
    w->writeAttribute( "units", units );
    w->writeAttribute( "completionTime", completionTime );
    w->writeAttribute( "completionTimeUnits", "seconds" );
    w->writeEndElement();
}

static void WriteTaskDefinition( XML::Writer *w, const SensorRegistrationData *data )
{
    w->writeStartElement( "taskDefinition" );

    w->writeStringElement( "concurrentTasks", "8" );

    WriteRegionDefinition( w, data );

    WriteCommand( w, "Request", "Registration, Reset, Heartbeat, Sensor Time, Stop, Start", "1" );
    WriteCommand( w, "DetectionThreshold", "Auto, Low, Medium, High", "1" );
    WriteCommand( w, "DetectionReportRate", "Auto, Low, Medium, High", "1" );
    WriteCommand( w, "ClassificationThreshold", "Auto, Low, Medium, High", "1" );
    WriteCommand( w, "Mode", "Default", "5" );
    WriteCommand( w, "LookAt", "rangeBearingCone", "5" );

    w->writeEndElement();
}

static void WriteModeDefinition( XML::Writer *w, const SensorRegistrationData *data )
{
    w->writeStartElement( "modeDefinition" );
    w->writeAttribute( "type", "Permanent" );

    w->writeStringElement( "modeName", "Default" );

    w->writeStartElement( "settleTime" );
    w->writeAttribute( "units", "seconds" );
    w->writeAttribute( "value", "5" );
    w->writeEndElement();

    w->writeStartElement( "maximumLatency" );
    w->writeAttribute( "units", "seconds" );
    w->writeAttribute( "value", "1" );
    w->writeEndElement();

    w->writeStringElement( "scanType", "Steerable" );

    w->writeStringElement( "trackingType", "Tracklet" );

    WriteDetectionDefinition( w );

    WriteTaskDefinition( w, data );

    w->writeEndElement();
}

void SensorRegistration::Write( XML::Writer *w )
{
    w->writeStartElement( "SensorRegistration" );

    w->writeStringElement( "timestamp", data->timestamp );
    if (data->sensorID.length()) {
        w->writeStringElement( "sensorID", data->sensorID );
    }
    w->writeStringElement( "sensorType", data->sensorType );

    WriteHeartbeatDefinition( w, data );

    WriteModeDefinition( w, data );

    w->writeEndElement();
}
