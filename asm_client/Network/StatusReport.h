//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Message.h"
#include "XML/Writer.h"

#include <string>

struct StatusReportLocationXY
{
    std::string x;
    std::string y;
    std::string ex;
    std::string ey;
};

struct StatusReportLocationRBC
{
    std::string r;
    std::string az;
    std::string he;
    std::string ve;
    std::string er;
    std::string eaz;
    std::string ehe;
    std::string eve;
};

struct StatusReportData
{
    std::string timestamp;
    std::string sensorID;     // Optional
    std::string system;
    std::string info;
    std::string activeTaskID; // Optional
    std::string powerSource;  // Optional
    std::string powerStatus;  // Optional
    std::string powerLevel;   // Optional

    // These fields will be ommitted if pointers are NULL
    struct StatusReportLocationXY *sensorLocation;
    struct StatusReportLocationRBC *fieldOfViewRBC;
    struct StatusReportLocationXY **fieldOfViewPolygon;
    struct StatusReportLocationRBC *coverage;
    struct StatusReportLocationRBC *obscuration;

    std::string internalFault; // Optional
    std::string externalFault; // Optional
    std::string clutter;       // Optional
    std::string motionSensitivity; // Optional
    std::string probabilityOfDetection; // Optional
    std::string falseAlarmRate; // Optional

    StatusReportData()
    {
        sensorLocation = nullptr;
        fieldOfViewRBC = nullptr;
        fieldOfViewPolygon = nullptr;
        coverage = nullptr;
        obscuration = nullptr;
    }
};

class StatusReport : public Message
{
public:
    // Constructs an empty StatusReport message
    StatusReport( StatusReportData *d ) : Message( "StatusReport" )
    {
        data = d;
    }

    // Write message using XML Writer
    virtual void Write( XML::Writer *w );

private:
    StatusReportData *data;
    static unsigned int reportID;
};
