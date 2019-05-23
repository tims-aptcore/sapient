//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "Network.h"
#include "XML/Reader.h"
#include "XML/Writer.h"
#include "XML/NetworkStream.h"
#include "SensorRegistration.h"
#include "StatusReport.h"
#include "SensorTask.h"
#include "SensorTaskACK.h"
#include "DetectionReport.h"
#include "../AsmClient.h"

#define ELPP_DEFAULT_LOGGER "network"
#include "../Utils/Log.h"
#include "../Utils/Config.h"
#include "../Utils/Utils.h"

#include <math.h>

Network::Network()
{
    networkStream = nullptr;
    reader = new XML::Reader();
    writer = new XML::Writer();
    statusReportData = new StatusReportData();
    defaultTask = new AsmClientTask();
    el::Logger* mainLogger = el::Loggers::getLogger( "network" );
}

Network::~Network()
{
    LOG( INFO ) << "Terminating Network...";

    networkStream->Close();

    delete networkStream;
    delete statusReportData;
}

void Network::Initialise( const char *configFilename )
{
    LOG( INFO ) << "Initialising Network...";

    CSimpleIniA config;
    SI_Error rc = config.LoadFile( configFilename );
    if (rc < 0)
    {
        LOG( ERROR ) << "Failed to load config file '" << configFilename << "'";
        throw "config.LoadFile returned " + std::to_string( rc );
    }

    hostname = config.GetValue( "network", "hostname", "localhost" );
    port = (int)config.GetLongValue( "network", "port", 0 );
    timeout = (int)config.GetLongValue( "network", "timeout_ms", 0 );

    delete networkStream;
    networkStream = new NetworkStream( hostname, port );

    sensorType = config.GetValue( "network", "sensorType", "Unknown ASM" );
    registrationDelay = config.GetDoubleValue( "network", "registrationDelay", 0.5 );
    registrationTimeout = config.GetDoubleValue( "network", "registrationTimeout", 5.0 );
    heartbeatInterval = (int)config.GetLongValue( "network", "heartbeatInterval", 10 );
    detectionInterval = config.GetDoubleValue( "network", "detectionInterval", 1.0 );
    suppressDetectionsDuringTamper = (int)config.GetLongValue( "network", "suppressDetectionsDuringTamper", 0 );
    fieldOfViewType = config.GetValue( "network", "fieldOfViewType", "RangeBearing" );

    statusReportData->activeTaskID = "0";

    statusReportData->coverage = new StatusReportLocationRBC();
    statusReportData->coverage->r = config.GetValue( "network", "coverageMaxRange", "100.0" );
    statusReportData->coverage->er = config.GetValue( "network", "coverageMaxRangeError", "1.0" );
    statusReportData->coverage->he = config.GetValue( "network", "coverageHorizontalExtent", "40.0" );
    statusReportData->coverage->ehe = config.GetValue( "network", "coverageHorizontalExtentError", "1.0" );
    statusReportData->coverage->ve = config.GetValue( "network", "coverageVerticalExtent", "10.0" );
    statusReportData->coverage->eve = config.GetValue( "network", "coverageVerticalExtentError", "1.0" );

    defaultTask->taskID = "0";
    defaultTask->bearing = strtof( statusReportData->coverage->az.c_str(), NULL );
    defaultTask->horizontalExtent = strtof( statusReportData->coverage->he.c_str(), NULL );

    defaultTask->minRange = (float)config.GetDoubleValue( "network", "defaultMinRange", 0.3 );
    defaultTask->maxRange = strtof( statusReportData->coverage->r.c_str(), NULL );
}

void Network::Loop( struct AsmClientStatus &status, struct AsmClientData &data, struct AsmClientTask &task )
{
    if (task.newTask)
    {
        task.newTask = false;
        if (task.rejectReason.length() == 0)
        {
            statusReportData->activeTaskID = task.taskID;
        }
    }

    if (networkStream->IsOpen())
    {
        reader->open( networkStream );
        if (reader->readStartElement( "SensorRegistrationACK" ))
        {
            if (reader->readStartElement( "sensorID" ))
            {
                reader->readPCData( statusReportData->sensorID );
                reader->readEndElement( false );
            }
            reader->readEndElement( false );
            status.network = AsmClientStatus::NETWORK_REGISTERED;
            LOG( INFO ) << "Received registration ACK with sensorID " << statusReportData->sensorID;

            task = *defaultTask;
            status.newStatus = true;
        }
        else if (reader->readStartElement( "SensorTask" ))
        {
            LOG( INFO ) << "Received sensor task";
            task.rejectReason = ParseSensorTask( task );
            reader->readEndElement( true );

            // Send the acknowledgement
            SensorTaskACKData data;
            data.timestamp = Get_Timestamp( std::chrono::system_clock::now() );
            data.sensorID = statusReportData->sensorID;
            data.taskID = task.taskID;
            data.status = task.rejectReason.length() ? "Rejected" : "Accepted";
            data.reason = task.rejectReason;

            SensorTaskACK sensorRegistration( &data );
            writer->open( networkStream );
            sensorRegistration.Write( writer );
            status.newStatus = true;
        }
        else if (reader->readStartElement( "Error" ))
        {
            LOG( WARNING ) << "Received error message";
            if (reader->readStartElement( "timestamp" ))
                reader->readEndElement( true );
            if (reader->readStartElement( "packet" ))
                reader->readEndElement( true );
            if (reader->readStartElement( "errorMessage" ))
            {
                std::string value = "";
                if (reader->readPCData( value )) {
                    LOG( WARNING ) << value;
                }
                reader->readEndElement( false );
            }
            reader->readEndElement( true );
        }
        else if (reader->readStartElement())
        {
            std::string value = "";
            reader->getElementName( value );
            LOG( WARNING ) << "Received unknown message: " << value;
            reader->readEndElement( true );
        }

        if (status.network != AsmClientStatus::NETWORK_REGISTERED && Get_Time_Monotonic() > registrationTime + registrationTimeout)
        {
            LOG( WARNING ) << "Timeout waiting for registration";
            networkStream->Close();
        }
    }
    else // Try connecting
    {
        statusReportData->sensorID = "";
        if (status.network != AsmClientStatus::NETWORK_NO_LINK &&
            status.network != AsmClientStatus::NETWORK_NOT_CONNECTED)
        {
            LOG( INFO ) << "Connecting to " << hostname << " : " << port << "...";
        }
        if (networkStream->Open( timeout ))
        {
            LOG( INFO ) << "Connected!";
            status.network = AsmClientStatus::NETWORK_NOT_REGISTERED;

            Sleep_ms( (int)(registrationDelay * 1000) );
            LOG( INFO ) << "Sending registration message...";

            SensorRegistrationData data;
            data.timestamp = Get_Timestamp( std::chrono::system_clock::now() );
            data.sensorID = statusReportData->sensorID;
            data.sensorType = sensorType;
            data.heartbeatInterval = std::to_string( heartbeatInterval );
            data.fieldOfViewType = fieldOfViewType;

            SensorRegistration sensorRegistration( &data );
            writer->open( networkStream );
            sensorRegistration.Write( writer );
            registrationTime = Get_Time_Monotonic();
            status.newStatus = true;
        }
        else if (Get_Time_Monotonic() > lastNetworkCheckTime + 1)
        {
            lastNetworkCheckTime = Get_Time_Monotonic();
            if (Network_Link_Down( hostname ))
            {
                if (status.network != AsmClientStatus::NETWORK_NO_LINK)
                    LOG( INFO ) << "Link is down";
                status.network = AsmClientStatus::NETWORK_NO_LINK;
            }
            else
            {
                if (status.network != AsmClientStatus::NETWORK_NOT_CONNECTED)
                    LOG( INFO ) << "Link is up";
                status.network = AsmClientStatus::NETWORK_NOT_CONNECTED;
            }
        }
    }

    defaultTask->bearing = status.compassBearing;
    defaultTask->horizontalExtent = strtof( statusReportData->coverage->he.c_str(), NULL );

    if (status.network == AsmClientStatus::NETWORK_REGISTERED)
    {
        double currentTime = Get_Time_Monotonic();
        if ((status.newStatus && currentTime > lastHeartbeatTime + detectionInterval) ||
            currentTime > lastHeartbeatTime + heartbeatInterval)
        {
            statusReportData->system = status.tamperStatus == AsmClientStatus::TAMPER_ACTIVE ? "Tamper" : "OK";
            statusReportData->info = status.newStatus ? "New" : "Unchanged";
            statusReportData->powerSource = status.powerSource;
            statusReportData->powerStatus = status.powerStatus;
            statusReportData->powerLevel = status.powerLevel;

            if (status.gnssValid)
            {
                if (statusReportData->sensorLocation == nullptr)
                    statusReportData->sensorLocation = new StatusReportLocationXY();
                statusReportData->sensorLocation->x = std::to_string( status.gnssEast );
                statusReportData->sensorLocation->y = std::to_string( status.gnssNorth );
                statusReportData->sensorLocation->ex = std::to_string( status.gnssError );
                statusReportData->sensorLocation->ey = std::to_string( status.gnssError );
            }

            if (statusReportData->fieldOfViewRBC == nullptr)
                statusReportData->fieldOfViewRBC = new StatusReportLocationRBC();
            statusReportData->fieldOfViewRBC->az = std::to_string( task.bearing );
            statusReportData->fieldOfViewRBC->eaz = std::to_string( status.compassBearingError );
            statusReportData->fieldOfViewRBC->r = std::to_string( task.maxRange );
            statusReportData->fieldOfViewRBC->er = statusReportData->coverage->er;
            statusReportData->fieldOfViewRBC->he = std::to_string( task.horizontalExtent );
            statusReportData->fieldOfViewRBC->ehe = statusReportData->coverage->ehe;
            statusReportData->fieldOfViewRBC->ve = statusReportData->coverage->ve;
            statusReportData->fieldOfViewRBC->eve = statusReportData->coverage->eve;

            if (status.compassValid)
            {
                statusReportData->coverage->az = std::to_string( status.compassBearing );
                statusReportData->coverage->eaz = std::to_string( status.compassBearingError );
            }

            statusReportData->internalFault = data.fault ? "Fault" : "OK";
            statusReportData->externalFault = status.externalFault ? "Fault" : "OK";
            statusReportData->clutter = data.clutter ? "High" : "Low";

            statusReportData->timestamp = Get_Timestamp( std::chrono::system_clock::now() );

            StatusReport statusReport( statusReportData );
            writer->open( networkStream );
            if (statusReport.Write( writer ) == false)
            {
                LOG( INFO ) << "Failed to send heartbeat message. Closing connection.";
                networkStream->Close();
            }
            else
            {
                LOG( INFO ) << "Sent heartbeat message";
            }

            status.newStatus = false;
            lastHeartbeatTime = currentTime;
        }

        if (data.detections.size() > 0 && currentTime > lastDetectionTime + detectionInterval)
        {
            struct DetectionReportData detectionReportData;

            detectionReportData.timestamp = data.timestamp;
            detectionReportData.sensorID = statusReportData->sensorID;
            detectionReportData.taskID = statusReportData->activeTaskID;

            detectionReportData.rangeBearing = new DetectionReportLocationRB();
            detectionReportData.objectDopplerSpeed = new DetectionReportValue();

            status.detectionsReported = 0;
            std::vector<AsmClientData::Detection>::iterator detection;
            for (detection = data.detections.begin(); detection != data.detections.end(); detection++)
            {
                float bearing = fmodf( status.compassBearing + detection->direction + 360.0f, 360.0f );
                float offsetAngle = fmodf( task.bearing - bearing + 180.0f, 360.0f ) - 180.0f;

                if (detection->range < task.minRange || detection->range > task.maxRange) continue;
                if (fabs( offsetAngle ) > task.horizontalExtent / 2.0) continue;

                if (status.tamperStatus == AsmClientStatus::TAMPER_ACTIVE && suppressDetectionsDuringTamper) continue;

                detectionReportData.objectID = std::to_string( detection->id );

                detectionReportData.rangeBearing->r = std::to_string( detection->range );
                detectionReportData.rangeBearing->er = "1.0";
                detectionReportData.rangeBearing->az = std::to_string( bearing );
                detectionReportData.rangeBearing->eaz = std::to_string( detection->directionError );

                detectionReportData.objectDopplerSpeed->value = std::to_string( detection->dopplerSpeed );
                detectionReportData.objectDopplerSpeed->e = "0.25";

                detectionReportData.detectionConfidence = std::to_string( detection->detectionConfidence );
                detectionReportData.humanConfidence = std::to_string( detection->humanConfidence );
                detectionReportData.vehicleConfidence = std::to_string( detection->vehicleConfidence );
                detectionReportData.unknownConfidence = std::to_string( detection->unknownConfidence );

                if (detection->vehicleTwoWheelConfidence + detection->vehicleFourWheelConfidence > 0)
                {
                    detectionReportData.vehicleTwoWheelConfidence = std::to_string( detection->vehicleTwoWheelConfidence );
                    detectionReportData.vehicleFourWheelConfidence = std::to_string( detection->vehicleFourWheelConfidence );
                }
                if (detection->vehicleFourWheelHeavyConfidence +
                    detection->vehicleFourWheelMediumConfidence +
                    detection->vehicleFourWheelLightConfidence > 0)
                {
                    detectionReportData.vehicleFourWheelHeavyConfidence = std::to_string( detection->vehicleFourWheelHeavyConfidence );
                    detectionReportData.vehicleFourWheelMediumConfidence = std::to_string( detection->vehicleFourWheelMediumConfidence );
                    detectionReportData.vehicleFourWheelLightConfidence = std::to_string( detection->vehicleFourWheelLightConfidence );
                }

                detectionReportData.humanWalkingConfidence = std::to_string( detection->humanWalkingConfidence );
                detectionReportData.humanRunningConfidence = std::to_string( detection->humanRunningConfidence );
                detectionReportData.humanLoiteringConfidence = std::to_string( detection->humanLoiteringConfidence );
                detectionReportData.humanCrawlingConfidence = std::to_string( detection->humanCrawlingConfidence );

                detectionReportData.staticObjectConfidence = std::to_string( detection->staticObjectConfidence );

                status.detectionsReported++;

                DetectionReport detectionReport( &detectionReportData );
                writer->open( networkStream );
                if (detectionReport.Write( writer ) == false)
                {
                    LOG( INFO ) << "Failed to send detection messages. Closing connection.";
                    networkStream->Close();
                }
            }

            delete detectionReportData.rangeBearing;
            delete detectionReportData.objectDopplerSpeed;

            if (status.tamperStatus == AsmClientStatus::TAMPER_ACTIVE && suppressDetectionsDuringTamper)
            {
                LOG( INFO ) << "Suppressed " << data.detections.size() << " detections while tamper active";
            }
            else if (networkStream->IsOpen())
            {
                LOG( INFO ) << "Sent " << status.detectionsReported << " of " << data.detections.size() << " detections";
            }

            lastDetectionTime = currentTime;
        }
        else if(currentTime > lastDetectionTime + detectionInterval)
        {
            status.detectionsReported = 0;
        }
    }
}

std::string Network::ParseSensorTask( struct AsmClientTask &task )
{
    SensorTask sensorTask( reader );
    const SensorTaskData taskData = sensorTask.GetSensorTaskData();

    task.newTask = true;
    task.taskID = taskData.taskID;

    if (taskData.sensorID != statusReportData->sensorID)
    {
        return "Wrong sensorID";
    }

    if (taskData.control == "Default")
    {
        task = *defaultTask;
    }
    if (taskData.control == "Start")
    {
        if (taskData.region.type == "Area of Interest")
        {
            float range = strtof( taskData.region.rangeBearingCone.r.c_str(), NULL );
            float bearing = strtof( taskData.region.rangeBearingCone.az.c_str(), NULL );
            float horizontalExtent = strtof( taskData.region.rangeBearingCone.he.c_str(), NULL );
            float coverageExtent = strtof( statusReportData->coverage->he.c_str(), NULL );
            float direction = bearing - strtof( statusReportData->coverage->az.c_str(), NULL );

            while (direction > 180) direction -= 360;
            while (direction < -180) direction += 360;

            if (range > strtod( statusReportData->coverage->r.c_str(), NULL ))
            {
                return "Out Of Range";
            }
            if (horizontalExtent / 2.0 > (coverageExtent / 2.0 - fabs( direction )))
            {
                return "Outside Field of View";
            }

            task.maxRange = range;
            task.bearing = bearing;
            task.horizontalExtent = horizontalExtent;
        }
        else if (!taskData.region.type.empty())
        {
            return "Not Supported";
        }

        if (!taskData.command.classificationThreshold.empty())
        {
            return "Not Supported";
        }
        if (!taskData.command.detectionReportRate.empty())
        {
            if (taskData.command.detectionReportRate == "Low") detectionInterval = 2.0;
            else if (taskData.command.detectionReportRate == "Medium") detectionInterval = 0.5;
            else if (taskData.command.detectionReportRate == "High") detectionInterval = 0.1;
            else if (taskData.command.detectionReportRate == "Lower") detectionInterval *= 2.0;
            else if (taskData.command.detectionReportRate == "Higher") detectionInterval *= 0.5;
            else return "Not Supported";
            return "";
        }
        if (!taskData.command.detectionThreshold.empty())
        {
            //TODO Add support for tasking detection threshold
            /*
            if (taskData.command.detectionThreshold == "Low") detectionInterval = 2.0;
            else if (taskData.command.detectionThreshold == "Medium") detectionInterval = 0.5;
            else if (taskData.command.detectionThreshold == "High") detectionInterval = 0.1;
            else if (taskData.command.detectionThreshold == "Lower") detectionInterval *= 2.0;
            else if (taskData.command.detectionThreshold == "Higher") detectionInterval *= 0.5;
            else return "Not Supported";
            */
            return "Not Supported";
        }
        if (!taskData.command.mode.empty())
        {
            return "Not Supported";
        }
        if (!taskData.command.rangeBearingCone.az.empty())
        {
            float range = strtof( taskData.command.rangeBearingCone.r.c_str(), NULL );
            float bearing = strtof( taskData.command.rangeBearingCone.az.c_str(), NULL );
            float horizontalExtent = strtof( taskData.command.rangeBearingCone.he.c_str(), NULL );
            float coverageExtent = strtof( statusReportData->coverage->he.c_str(), NULL );
            float direction = bearing - strtof( statusReportData->coverage->az.c_str(), NULL );

            while (direction > 180) direction -= 360;
            while (direction < -180) direction += 360;

            if (range > strtod( statusReportData->coverage->r.c_str(), NULL ))
            {
                return "Out Of Range";
            }
            if (horizontalExtent / 2.0 > (coverageExtent / 2.0 - fabs( direction )))
            {
                return "Outside Field of View";
            }

            task.maxRange = range;
            task.bearing = bearing;
            task.horizontalExtent = horizontalExtent;
        }

        if (taskData.command.request == "Start")
        {
            return "";
        }
        else if (taskData.command.request == "Stop")
        {
            task = *defaultTask;
        }
        else if (!taskData.command.request.empty())
        {
            return "Not Supported";
        }
    }
    return "";
}
