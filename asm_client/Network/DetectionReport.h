//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Message.h"
#include "ProtobufInterface/Writer.h"
#include "../Utils/Ulid.h"

#include <string>

struct DetectionReportLocationRB
{
    std::string r;
    std::string az;
    std::string er;
    std::string eaz;
};

struct DetectionReportValue
{
    std::string value;
    std::string e;
};

struct DetectionReportData
{
    std::string timestamp;
    std::string nodeID;
    std::string destID;
    ulid::ULID  objectID;
    ulid::ULID  taskID;
    std::string state;

    struct DetectionReportLocationRB *rangeBearing;

    std::string detectionConfidence;

    struct DetectionReportValue *trackConfidence;
    struct DetectionReportValue *trackSpeed;
    struct DetectionReportValue *trackAZ;
    struct DetectionReportValue *trackDR;
    struct DetectionReportValue *trackDAZ;

    struct DetectionReportValue *objectDopplerSpeed;
    struct DetectionReportValue *objectDopplerAz;
    struct DetectionReportValue *objectMajorLength;
    struct DetectionReportValue *objectMajorAxisAz;
    struct DetectionReportValue *objectMinorLength;
    struct DetectionReportValue *objectHeight;

    std::string humanConfidence;
    std::string vehicleConfidence;
    std::string vehicleTwoWheelConfidence;
    std::string vehicleFourWheelConfidence;
    std::string vehicleFourWheelHeavyConfidence;
    std::string vehicleFourWheelMediumConfidence;
    std::string vehicleFourWheelLightConfidence;
    std::string staticObjectConfidence;
    std::string unknownConfidence;
    std::string humanWalkingConfidence;
    std::string humanRunningConfidence;
    std::string humanLoiteringConfidence;
    std::string humanCrawlingConfidence;

    DetectionReportData()
    {
        trackConfidence = nullptr;
        trackSpeed = nullptr;
        trackAZ = nullptr;
        trackDR = nullptr;
        trackDAZ = nullptr;

        objectDopplerSpeed = nullptr;
        objectDopplerAz = nullptr;
        objectMajorLength = nullptr;
        objectMajorAxisAz = nullptr;
        objectMinorLength = nullptr;
        objectHeight = nullptr;
    }
};

class DetectionReport : public Message
{
public:
    // Constructs a DetectionReport message from the provided data
    DetectionReport( DetectionReportData *d ) : Message( "DetectionReport" )
    {
        data = d;
    }

    // Write message using protobuf Writer
    virtual bool Write( ProtobufInterface::Writer *w );

private:
    DetectionReportData *data;
};

