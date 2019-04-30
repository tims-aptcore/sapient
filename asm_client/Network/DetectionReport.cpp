//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "DetectionReport.h"
#include "XML/Writer.h"

unsigned int DetectionReport::reportID = 1;

static void WriteRangeBearing( XML::Writer *w, const DetectionReportLocationRB *rb )
{
    if (rb) {
        w->writeStartElement( "rangeBearing" );
        w->writeStringElement( "R", rb->r );
        w->writeStringElement( "Az", rb->az );
        w->writeStringElement( "eR", rb->er );
        w->writeStringElement( "eAz", rb->eaz );
        w->writeEndElement();
    }
}

static void WriteTrackInfo( XML::Writer *w, const char *type, const DetectionReportValue *v )
{
    if (v) {
        w->writeStartElement( "trackInfo" );
        w->writeAttribute( "type", type );
        w->writeAttribute( "value", v->value );
        w->writeAttribute( "e", v->e );
        w->writeEndElement();
    }
}

static void WriteObjectInfo( XML::Writer *w, const char *type, const DetectionReportValue *v )
{
    if (v) {
        w->writeStartElement( "objectInfo" );
        w->writeAttribute( "type", type );
        w->writeAttribute( "value", v->value );
        w->writeAttribute( "e", v->e );
        w->writeEndElement();
    }
}

static void WriteSubClassSize( XML::Writer *w, const char *value, std::string confidence )
{
    if (confidence.length()) {
        w->writeStartElement( "subClass" );
        w->writeAttribute( "level", "2" );
        w->writeAttribute( "type", "Size" );
        w->writeAttribute( "value", value );
        w->writeStringElement( "confidence", confidence );
        w->writeEndElement();
    }
}

static void WriteBehaviour( XML::Writer *w, const char *type, std::string confidence )
{
    if (confidence.length()) {
        w->writeStartElement( "behaviour" );
        w->writeAttribute( "type", type );
        w->writeStringElement( "confidence", confidence );
        w->writeEndElement();
    }
}

void DetectionReport::Write( XML::Writer *w )
{
    w->writeStartElement( "DetectionReport" );

    w->writeStringElement( "timestamp", data->timestamp );
    if (data->sensorID.length()) {
        w->writeStringElement( "sourceID", data->sensorID );
    }
    w->writeStringElement( "reportID", std::to_string( reportID++ ) );
    w->writeStringElement( "objectID", data->objectID );
    if (data->taskID.length()) {
        w->writeStringElement( "taskID", data->taskID );
    }
    if (data->state.length()) {
        w->writeStringElement( "state", data->state );
    }

    WriteRangeBearing( w, data->rangeBearing );

    w->writeStringElement( "detectionConfidence", data->detectionConfidence );

    WriteTrackInfo( w, "confidence", data->trackConfidence );
    WriteTrackInfo( w, "speed", data->trackSpeed );
    WriteTrackInfo( w, "az", data->trackAZ );
    WriteTrackInfo( w, "dR", data->trackDR );
    WriteTrackInfo( w, "dAz", data->trackDAZ );

    WriteObjectInfo( w, "dopplerSpeed", data->objectDopplerSpeed );
    WriteObjectInfo( w, "dopplerAz", data->objectDopplerAz );
    WriteObjectInfo( w, "majorLength", data->objectMajorLength );
    WriteObjectInfo( w, "majorAxisAz", data->objectMajorAxisAz );
    WriteObjectInfo( w, "minorLength", data->objectMinorLength );
    WriteObjectInfo( w, "height", data->objectHeight );

    if (data->humanConfidence.length()) {
        w->writeStartElement( "class" );
        w->writeAttribute( "type", "Human" );
        w->writeStringElement( "confidence", data->humanConfidence );
        w->writeEndElement();
    }

    if (data->vehicleConfidence.length()) {
        w->writeStartElement( "class" );
        w->writeAttribute( "type", "Vehicle" );
        w->writeStringElement( "confidence", data->vehicleConfidence );

        if (data->vehicleTwoWheelConfidence.length()) {
            w->writeStartElement( "subClass" );
            w->writeAttribute( "level", "1" );
            w->writeAttribute( "type", "Vehicle Class" );
            w->writeAttribute( "value", "2 Wheeled" );
            w->writeStringElement( "confidence", data->vehicleTwoWheelConfidence );
            w->writeEndElement();
        }

        if (data->vehicleFourWheelConfidence.length()) {
            w->writeStartElement( "subClass" );
            w->writeAttribute( "level", "1" );
            w->writeAttribute( "type", "Vehicle Class" );
            w->writeAttribute( "value", "4 Wheeled" );
            w->writeStringElement( "confidence", data->vehicleFourWheelConfidence );

            WriteSubClassSize( w, "Heavy", data->vehicleFourWheelHeavyConfidence );
            WriteSubClassSize( w, "Medium", data->vehicleFourWheelMediumConfidence );
            WriteSubClassSize( w, "Light", data->vehicleFourWheelLightConfidence );

            w->writeEndElement();
        }
        w->writeEndElement();
    }

    if (data->staticObjectConfidence.length()) {
        w->writeStartElement( "class" );
        w->writeAttribute( "type", "Static Object" );
        w->writeStringElement( "confidence", data->staticObjectConfidence );
        w->writeEndElement();
    }

    if (data->unknownConfidence.length()) {
        w->writeStartElement( "class" );
        w->writeAttribute( "type", "Unknown" );
        w->writeStringElement( "confidence", data->unknownConfidence );
        w->writeEndElement();
    }

    WriteBehaviour( w, "Walking", data->humanWalkingConfidence );
    WriteBehaviour( w, "Running", data->humanRunningConfidence );
    WriteBehaviour( w, "Loitering", data->humanLoiteringConfidence );
    WriteBehaviour( w, "Crawling", data->humanCrawlingConfidence );

    w->writeEndElement();
}
