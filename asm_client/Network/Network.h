//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include <string>

#include "sapient_msg/bsi_flex_335_v2_0/sapient_message.pb.h"
namespace sap = sapient_msg::bsi_flex_335_v2_0;

namespace ProtobufInterface
{
    class Reader;
    class Writer;
}
class NetworkStream;
struct StatusReportData;
struct AsmClientTask;

class Network
{
public:
    Network();
    virtual ~Network();
    void Initialise( const char *configFilename );
    void Loop( struct AsmClientStatus &status, struct AsmClientData &data, struct AsmClientTask &task );

private:
    std::string ParseSensorTask( struct AsmClientTask &task, sap::Task& msg_task );

    ProtobufInterface::Reader *reader;
    ProtobufInterface::Writer *writer;

    NetworkStream *networkStream;
    std::string hostname;
    int port;
    int timeout;
    double lastNetworkCheckTime;
    double registrationDelay;
    double registrationTime;
    double registrationTimeout;
    double detectionInterval;
    double lastDetectionTime;

    std::string nodeID;
    std::string destID;
    int heartbeatInterval;
    double lastHeartbeatTime;
    std::string sensorType;
    int suppressDetectionsDuringTamper;
    int suppressFovDuringTamper;
    std::string fieldOfViewType;

    struct StatusReportData *statusReportData;
    struct AsmClientTask *defaultTask;
};
