//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Utils/Ulid.h"

#include <string>
#include <vector>

struct AsmClientStatus
{
    bool newStatus;
    bool externalFault;
    enum { TAMPER_OK, TAMPER_ACTIVE, TAMPER_NOT_LEVEL } tamperStatus;

    std::string powerSource;
    std::string powerStatus;
    std::string powerLevel;

    bool gnssValid;
    double gnssEast;
    double gnssNorth;
    double gnssError;

    bool compassValid;
    float compassBearing;
    float compassBearingError;

    enum NetworkStatus
    {
        NETWORK_NO_LINK,
        NETWORK_NOT_CONNECTED,
        NETWORK_CONNECTING,
        NETWORK_NOT_REGISTERED,
        NETWORK_REGISTERED,
    } network;

    int detectionsReported;
};

struct AsmClientData
{
    bool fault;
    bool clutter;
    std::string timestamp;

    struct Detection
    {
        ulid::ULID id;
        bool updated;
        float range;
        float direction;
        float directionError;
        float dopplerSpeed;
        float detectionConfidence;
        float humanConfidence;
        float vehicleConfidence;
        float unknownConfidence;
        float humanLoiteringConfidence;
        float humanRunningConfidence;
        float humanWalkingConfidence;
        float humanCrawlingConfidence;
        float vehicleTwoWheelConfidence;
        float vehicleFourWheelConfidence;
        float vehicleFourWheelHeavyConfidence;
        float vehicleFourWheelMediumConfidence;
        float vehicleFourWheelLightConfidence;
        float staticObjectConfidence;
        void *userData;
    };
    std::vector<struct Detection> detections;
};

struct AsmClientTask
{
    bool newTask;
    ulid::ULID taskID;
    std::string rejectReason;

    float bearing;
    float horizontalExtent;
    float minRange;
    float maxRange;

    enum { NONE, REGISTRATION, HEARTBEAT, TAKE_SNAPSHOT } command;
};
