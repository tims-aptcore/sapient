//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include <string>

typedef enum
{
    TCPCLIENT_ERR_NO_ERROR = 0,
    TCPCLIENT_ERR_INVALID_STATE,
    TCPCLIENT_ERR_OPENING_SOCKET,
    TCPCLIENT_ERR_SETTING_SOCKET_OPT,
    TCPCLIENT_ERR_NOT_CONNECTED,
    TCPCLIENT_ERR_ALREADY_CONNECTED,
    TCPCLIENT_ERR_WRITING_TO_SERVER,
    TCPCLIENT_ERR_READING_FROM_SERVER,
    TCPCLIENT_ERR_CONNECTION_LOST
} TcpClientErrno;

struct TcpClientState;

class TcpClient
{
public:
    TcpClient( std::string hostname, int port );
    ~TcpClient();

    // Attempts to connect to a server and sets 'connected' if successful
    // Returns immediately. Subsequent calls continue same connection until
    // timeout milliseconds have elapsed, when a new connection is started
    // Returns 0 on success, otherwise an error code
    TcpClientErrno Connect( int timeout, int *connected );

    // Disconnects from the server
    // Returns 0 on success, otherwise an error code
    TcpClientErrno Disconnect();

    // Returns the internal 'connected' state, which is updated on Receive or Write
    // Returns 0 on success, otherwise an error code
    TcpClientErrno Get_Connected( int *connected );

    // Waits for data and then reads up to max_length bytes to data address
    // from the server. Actual number of bytes read provided in *length
    // The read will wait for the specified number of milliseconds before
    // returning with no error and length set to zero if no data is received.
    // Returns 0 on success, otherwise an error code
    TcpClientErrno Read( int timeout, void *data, int max_length, int *length );

    // Writes the specified number of bytes from the data address to the server
    // Returns 0 on success, otherwise an error code
    TcpClientErrno Write( const void *data, int length );

private:
    TcpClientErrno Set_Non_Blocking( int non_blocking );

    struct TcpClientState *state;
};
