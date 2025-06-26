//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "NetworkStream.h"
#include "../../Utils/TcpClient.h"

#define ELPP_DEFAULT_LOGGER "network"
#include "../../Utils/Log.h"
#include "../../Utils/Utils.h"

#ifdef __unix__
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <arpa/inet.h>
#define FLAG int
#define SOCKET int
#else // windows
#include <Windows.h>
#pragma comment(lib, "Ws2_32.lib")
#define close(s) closesocket(s)
#define socklen_t int
#define FLAG char
#define MSG_NOSIGNAL 0
#endif

NetworkStream::NetworkStream( std::string hostname, int port )
{
    writeBufferUsed = 0;
    tcpclient = new TcpClient( hostname, port );
}

NetworkStream::~NetworkStream()
{
    Close();
    delete tcpclient;
}

bool NetworkStream::Open( int timeout )
{
    int connected;
    TcpClientErrno returnCode = tcpclient->Connect( timeout, &connected );

    if (returnCode == TCPCLIENT_ERR_OPENING_SOCKET)
        LOG( WARNING ) << "Failed to open socket";
    if (returnCode == TCPCLIENT_ERR_SETTING_SOCKET_OPT)
        LOG( WARNING ) << "Failed to set socket option";

    return connected;
}

bool NetworkStream::IsOpen()
{
    if (tcpclient == nullptr) return false;

    int connected;
    tcpclient->Get_Connected( &connected );
    return (connected != 0);
}

bool NetworkStream::Read( unsigned char *pOctets, size_t iOctets, size_t &iRead )
{
    if (tcpclient == nullptr) return false;

    int length;
    TcpClientErrno returnCode = tcpclient->Read( 0, pOctets, iOctets, &length );
    iRead = length;

    if (returnCode == TCPCLIENT_ERR_INVALID_STATE)
        LOG( WARNING ) << "NetworkStream not opened";
    if (returnCode == TCPCLIENT_ERR_NOT_CONNECTED)
        LOG( WARNING ) << "Not connected";
    if (returnCode == TCPCLIENT_ERR_CONNECTION_LOST)
        LOG( WARNING ) << "Connection lost";

    return (returnCode == TCPCLIENT_ERR_NO_ERROR);
}

bool NetworkStream::ReadWithTimeout( unsigned char *pOctets, size_t iOctets, size_t &iRead, int millisecs )
{
    if (tcpclient == nullptr) return false;

    int length;
    TcpClientErrno returnCode = tcpclient->Read( millisecs, pOctets, iOctets, &length );
    iRead = length;

    if (returnCode == TCPCLIENT_ERR_INVALID_STATE)
        LOG( WARNING ) << "NetworkStream not opened";
    if (returnCode == TCPCLIENT_ERR_NOT_CONNECTED)
        LOG( WARNING ) << "Not connected";
    if (returnCode == TCPCLIENT_ERR_CONNECTION_LOST)
        LOG( WARNING ) << "Connection lost";

    return (returnCode == TCPCLIENT_ERR_NO_ERROR);
}

bool NetworkStream::Write( unsigned char *pOctets, size_t iOctets, size_t &iWrote )
{
    size_t toCopy = WRITE_BUFFER_SIZE - writeBufferUsed;
    if (toCopy > iOctets) toCopy = iOctets;
    memcpy( &writeBuffer[writeBufferUsed], pOctets, toCopy );
    writeBufferUsed += (int)toCopy;

    // If the buffer is full, send and start again (recursively)
    if (writeBufferUsed == WRITE_BUFFER_SIZE)
    {
        if (Send( false ))
        {
            return Write( &pOctets[toCopy], iOctets - toCopy, iWrote );
        }
        return false;
    }
    else
    {
        Send( false );
    }

    return true;
}

bool NetworkStream::Send( bool bTerminator )
{
    if (tcpclient == nullptr) return false;

    if (bTerminator)
    {
        size_t iWrote;
        unsigned char terminator = '\0';
        Write( &terminator, 1, iWrote );
    }

    TcpClientErrno returnCode = tcpclient->Write( writeBuffer, writeBufferUsed );
    writeBufferUsed = 0;

    if (returnCode == TCPCLIENT_ERR_INVALID_STATE)
        LOG( WARNING ) << "NetworkStream not opened";
    if (returnCode == TCPCLIENT_ERR_NOT_CONNECTED)
        LOG( WARNING ) << "Not connected";
    if (returnCode == TCPCLIENT_ERR_CONNECTION_LOST)
        LOG( WARNING ) << "Connection lost";
    if (returnCode == TCPCLIENT_ERR_WRITING_TO_SERVER)
        LOG( WARNING ) << "Failed to write";

    return (returnCode == TCPCLIENT_ERR_NO_ERROR);
}

void NetworkStream::Close()
{
    if (tcpclient == nullptr) return;

    tcpclient->Disconnect();
}

void NetworkStream::Flush()
{
    // Flush all the bytes from the input stream and get it into a state where the first byte read is the start of a new message
    // Used when we have mismatches in the input expected data stream possibly due to data corruption.

    size_t read = 0;
    unsigned char black_hole[256];
    while( Read( black_hole, 256, read ) )
    {
    }
}
