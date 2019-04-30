//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include <string>

namespace XML
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
    std::string ParseSensorTask( struct AsmClientTask &task );

    XML::Reader *reader;
    XML::Writer *writer;

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

    int heartbeatInterval;
    double lastHeartbeatTime;
    std::string sensorType;
    std::string fieldOfViewType;

    struct StatusReportData *statusReportData;
    struct AsmClientTask *defaultTask;
};
