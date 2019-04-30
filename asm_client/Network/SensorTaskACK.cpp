//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "SensorTaskACK.h"
#include "XML/Writer.h"

void SensorTaskACK::Write( XML::Writer *w )
{
    w->writeStartElement( "SensorTaskACK" );

    w->writeStringElement( "timestamp", data->timestamp );
    if (data->sensorID.length()) {
        w->writeStringElement( "sensorID", data->sensorID );
    }
    if (data->taskID.length()) {
        w->writeStringElement( "taskID", data->taskID );
    }
    if (data->status.length()) {
        w->writeStringElement( "status", data->status );
    }
    if (data->reason.length()) {
        w->writeStringElement( "reason", data->reason );
    }

    w->writeEndElement();
}
