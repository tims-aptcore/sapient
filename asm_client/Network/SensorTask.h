//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "XML/Reader.h"

#include <string>

struct SensorTaskRangeBearingCone
{
    std::string r;
    std::string az;
    std::string he;
};

struct SensorTaskClassFilter
{
    std::string name;
    std::string op;
    std::string value;
};

struct SensorTaskRegion
{
    std::string type;
    std::string regionID;
    std::string regionName;

    SensorTaskRangeBearingCone rangeBearingCone;

    SensorTaskClassFilter classFilterAll;
    SensorTaskClassFilter classFilterHuman;
    SensorTaskClassFilter classFilterVehicle;
    SensorTaskClassFilter behaviourFilterWalking;
    SensorTaskClassFilter behaviourFilterRunning;
    SensorTaskClassFilter behaviourFilterLoitering;
    SensorTaskClassFilter behaviourFilterCrawling;
};

struct SensorTaskCommand
{
    std::string request;
    std::string detectionThreshold;
    std::string detectionReportRate;
    std::string classificationThreshold;
    std::string mode;

    SensorTaskRangeBearingCone rangeBearingCone;
};

struct SensorTaskData
{
    std::string sensorID;
    std::string taskID;
    std::string taskName;
    std::string taskDescription;
    std::string taskStartTime;
    std::string taskEndTime;
    std::string control;

    SensorTaskRegion region;
    SensorTaskCommand command;
};

class SensorTask
{
public:
    // Constructs SensorTask message read from the reader
    SensorTask( XML::Reader *reader );

    // Gets a copy of the SensorTaskData
    virtual SensorTaskData GetSensorTaskData() { return *data; }

private:
    SensorTaskData *data;
};
