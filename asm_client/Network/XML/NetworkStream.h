//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Stream.h"

#include <stddef.h>
#include <string>

#ifdef __unix__
#include <netinet/in.h>
#define SOCKET int
#else // windows
#include <Windows.h>
#endif

#define WRITE_BUFFER_SIZE 8192

class TcpClient;

class NetworkStream : public IInputStream, public IOutputStream
{
public:
    NetworkStream( std::string hostname, int port );
    virtual ~NetworkStream();
    bool Open( int timeout );
    bool IsOpen();
    virtual bool Read( unsigned char *pOctets, size_t iOctets, size_t &iRead );
    virtual bool Write( unsigned char *pOctets, size_t iOctets, size_t &iWrote );
    virtual bool Send( bool bTerminator );
    virtual void Close();

private:
    class TcpClient *tcpclient;
    std::string hostname;

    unsigned char writeBuffer[WRITE_BUFFER_SIZE];
    int writeBufferUsed;
};
