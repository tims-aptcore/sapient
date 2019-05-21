//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "StatusReport.h"
#include "XML/Writer.h"

unsigned int StatusReport::reportID = 1;

static void WriteLocation( XML::Writer *w, const StatusReportLocationXY *xy )
{
    w->writeStartElement( "location" );
    w->writeStringElement( "X", xy->x );
    w->writeStringElement( "Y", xy->y );
    w->writeStringElement( "eX", xy->ex );
    w->writeStringElement( "eY", xy->ey );
    w->writeEndElement();
}

static void WriteRangeBearingCone( XML::Writer *w, const StatusReportLocationRBC *rbc )
{
    w->writeStartElement( "rangeBearingCone" );
    w->writeStringElement( "R", rbc->r );
    w->writeStringElement( "Az", rbc->az );
    w->writeStringElement( "hExtent", rbc->he );
    w->writeStringElement( "vExtent", rbc->ve );
    w->writeStringElement( "eR", rbc->er );
    w->writeStringElement( "eAz", rbc->eaz );
    w->writeStringElement( "ehExtent", rbc->ehe );
    w->writeStringElement( "evExtent", rbc->eve );
    w->writeEndElement();
}

static void WriteLocationList( XML::Writer *w, StatusReportLocationXY **list )
{
    w->writeStartElement( "locationList" );
    for (int i = 0; list[i] != NULL; i++) {
        WriteLocation( w, list[i] );
    }
    w->writeEndElement();
}

static void WriteStatus( XML::Writer *w, const char *level, const char *type, std::string value )
{
    if (value.length()) {
        w->writeStartElement( "status" );
        w->writeAttribute( "level", level );
        w->writeAttribute( "type", type );
        w->writeAttribute( "value", value );
        w->writeEndElement();
    }
}

bool StatusReport::Write( XML::Writer *w )
{
    w->writeStartElement( "StatusReport" );

    w->writeStringElement( "timestamp", data->timestamp );
    if (data->sensorID.length()) {
        w->writeStringElement( "sourceID", data->sensorID );
    }
    w->writeStringElement( "reportID", std::to_string( reportID++ ) );
    w->writeStringElement( "system", data->system );
    w->writeStringElement( "info", data->info );
    if (data->activeTaskID.length()) {
        w->writeStringElement( "activeTaskID", data->activeTaskID );
    }
    w->writeStringElement( "mode", "Default" );

    if (data->powerStatus.length()) {
        w->writeStartElement( "power" );
        w->writeAttribute( "source", data->powerSource );
        if (data->powerStatus.length()) {
            w->writeAttribute( "status", data->powerStatus );
        }
        if (data->powerLevel.length()) {
            w->writeAttribute( "level", data->powerLevel );
        }
        w->writeEndElement();
    }

    if (data->sensorLocation) {
        w->writeStartElement( "sensorLocation" );
        WriteLocation( w, data->sensorLocation );
        w->writeEndElement();
    }

    if (data->fieldOfViewRBC) {
        w->writeStartElement( "fieldOfView" );
        WriteRangeBearingCone( w, data->fieldOfViewRBC );
        w->writeEndElement();
    }
    if (data->fieldOfViewPolygon) {
        w->writeStartElement( "fieldOfView" );
        WriteLocationList( w, data->fieldOfViewPolygon );
        w->writeEndElement();
    }

    if (data->coverage) {
        w->writeStartElement( "coverage" );
        WriteRangeBearingCone( w, data->coverage );
        w->writeEndElement();
    }

    if (data->obscuration) {
        w->writeStartElement( "obscuration" );
        WriteRangeBearingCone( w, data->obscuration );
        w->writeEndElement();
    }

    WriteStatus( w, "Error", "InternalFault", data->internalFault );
    WriteStatus( w, "Error", "ExternalFault", data->externalFault );
    WriteStatus( w, "Information", "Clutter", data->clutter );
    WriteStatus( w, "Sensor", "MotionSensitivity", data->motionSensitivity );
    WriteStatus( w, "Sensor", "PD", data->probabilityOfDetection );
    WriteStatus( w, "Sensor", "FAR", data->falseAlarmRate );

    return w->writeEndElement();
}
