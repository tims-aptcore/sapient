//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "TcpClient.h"
#include "../Utils/Utils.h"

#include <string.h>

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

// Chunk size when writing to socket
#define CHUNK_SIZE 65536

struct TcpClientState
{
    std::string hostname;
    int port;

    SOCKET client_sockfd;
    struct sockaddr_in sockaddrin;
    int connected;
    double connect_time;
};

TcpClientErrno TcpClient::Set_Non_Blocking( int non_blocking )
{
    if (state == NULL) return TCPCLIENT_ERR_INVALID_STATE;
#ifdef __unix__
    int flags = fcntl( state->client_sockfd, F_GETFL );
    if (flags < 0)
    {
        return TCPCLIENT_ERR_SETTING_SOCKET_OPT;
    }
    flags &= ~O_NONBLOCK;
    if (non_blocking)
    {
        flags |= O_NONBLOCK;
    }
    fcntl( state->client_sockfd, F_SETFL, flags );
#else // windows
    u_long nb = non_blocking ? 1 : 0;
    if (ioctlsocket( state->client_sockfd, FIONBIO, &nb ))
    {
        return TCPCLIENT_ERR_SETTING_SOCKET_OPT;
    }
#endif
    return TCPCLIENT_ERR_NO_ERROR;
}

TcpClient::TcpClient( std::string hostname, int port )
{
#ifndef __unix__ // windows
    WSADATA wsaData;
    if (WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0) throw "Failed to start WSA";
#endif

    state = new struct TcpClientState;

    state->hostname = hostname;
    state->port = port;
    state->connected = 0;
    state->connect_time = 0.0;
}

TcpClient::~TcpClient()
{
    Disconnect();
    delete state;
}

TcpClientErrno TcpClient::Connect( int timeout, int *connected )
{
    FLAG on;
    fd_set writefds;
    struct timeval timeout_struct = { 0 };

    *connected = 0;

    if (state == NULL) return TCPCLIENT_ERR_INVALID_STATE;

    if (state->connected)
    {
        *connected = 1;
        return TCPCLIENT_ERR_ALREADY_CONNECTED;
    }

    if (state->connect_time == 0.0)
    {
        state->client_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
        if (state->client_sockfd < 0)
        {
            return TCPCLIENT_ERR_OPENING_SOCKET;
        }

        // Disable Nagle algorithm that waits for additional data during read
        if (setsockopt( state->client_sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof( on ) ) != 0)
        {
            return TCPCLIENT_ERR_SETTING_SOCKET_OPT;
        }

        // Prepare the socket address and port
        memset( state->sockaddrin.sin_zero, '\0', sizeof( state->sockaddrin.sin_zero ) );
        state->sockaddrin.sin_family = AF_INET;
        state->sockaddrin.sin_addr.s_addr = inet_addr( state->hostname.c_str() );
        state->sockaddrin.sin_port = htons( state->port );

        TcpClientErrno returnCode = Set_Non_Blocking( 1 );
        if (returnCode) return returnCode;

        connect( state->client_sockfd, (struct sockaddr *)(&state->sockaddrin), sizeof( struct sockaddr_in ) );

        state->connect_time = Get_Time_Monotonic();
    }

    // See if the connection is successful
    FD_ZERO( &writefds );
    FD_SET( state->client_sockfd, &writefds );

    if (select( (int)state->client_sockfd + 1, NULL, &writefds, NULL, &timeout_struct ) > 0)
    {
        int result;
        socklen_t result_len = sizeof( result );
        if (getsockopt( state->client_sockfd, SOL_SOCKET, SO_ERROR, (FLAG*)&result, &result_len ) < 0)
        {
            return TCPCLIENT_ERR_SETTING_SOCKET_OPT;
        }

        if (result == 0)
        {
            TcpClientErrno returnCode = Set_Non_Blocking( 0 );
            if (returnCode) return returnCode;

            state->connected = 1;
        }
        else
        {
            close( state->client_sockfd );
            state->connect_time = 0.0;
        }
    }
    else if (Get_Time_Monotonic() > state->connect_time + (timeout / 1000.0))
    {
        close( state->client_sockfd );
        state->connect_time = 0.0;
    }

    *connected = state->connected;

    return TCPCLIENT_ERR_NO_ERROR;
}

TcpClientErrno TcpClient::Disconnect()
{
    if (state == NULL) return TCPCLIENT_ERR_INVALID_STATE;

    if (state->connected == 0) return TCPCLIENT_ERR_NOT_CONNECTED;

    close( state->client_sockfd );
    state->connected = 0;

    return TCPCLIENT_ERR_NO_ERROR;
}

TcpClientErrno TcpClient::Get_Connected( int *connected )
{
    if (state == NULL) return TCPCLIENT_ERR_INVALID_STATE;

    *connected = state->connected;

    return TCPCLIENT_ERR_NO_ERROR;
}

TcpClientErrno TcpClient::Read( int timeout, void *data, int max_length, int *length )
{
    fd_set readfds;
    struct timeval timeout_struct;
    int waiting_data = 1, read_length;
    char *ptr = (char*)data;

    *length = 0;

    if (state == NULL) return TCPCLIENT_ERR_INVALID_STATE;

    if (state->connected == 0) return TCPCLIENT_ERR_NOT_CONNECTED;

    // See if there is data waiting to be read
    timeout_struct.tv_sec = timeout / 1000;
    timeout_struct.tv_usec = (timeout % 1000) * 1000;

    FD_ZERO( &readfds );
    FD_SET( state->client_sockfd, &readfds );

    while (waiting_data > 0 && *length < max_length)
    {
        waiting_data = select( (int)state->client_sockfd + 1, &readfds, NULL, NULL, &timeout_struct );

        // See if there is any data available
        if (waiting_data < 0)
        {
            Disconnect();
            return TCPCLIENT_ERR_CONNECTION_LOST;
        }
        else if (waiting_data > 0)
        {
            // Read the available data
            read_length = recv( state->client_sockfd, ptr, max_length - *length, MSG_NOSIGNAL );
            if (read_length <= 0)
            {
                Disconnect();
                return TCPCLIENT_ERR_CONNECTION_LOST;
            }

            ptr += read_length;
            *length += read_length;
        }
    }

    return TCPCLIENT_ERR_NO_ERROR;
}

TcpClientErrno TcpClient::Write( void *data, int length )
{
    fd_set writefds;
    struct timeval timeout_struct = { 0 };
    int space_available = 1, write_length = 1, remainder = length;
    char *ptr = (char*)data;

    if (state == NULL) return TCPCLIENT_ERR_INVALID_STATE;

    if (state->connected == 0) return TCPCLIENT_ERR_NOT_CONNECTED;

    FD_ZERO( &writefds );
    FD_SET( state->client_sockfd, &writefds );

    while (space_available > 0 && write_length > 0 && remainder > 0)
    {
        space_available = select( (int)state->client_sockfd + 1, NULL, &writefds, NULL, &timeout_struct );

        // See if we can send data
        if (space_available < 0)
        {
            Disconnect();
            return TCPCLIENT_ERR_CONNECTION_LOST;
        }
        else if (space_available > 0)
        {
            write_length = remainder > CHUNK_SIZE ? CHUNK_SIZE : remainder;

            // Write up to CHUNK_SIZE bytes at a time
            write_length = send( state->client_sockfd, ptr, write_length, MSG_NOSIGNAL );
            if (write_length == -1)
            {
                Disconnect();
                return TCPCLIENT_ERR_CONNECTION_LOST;
            }

            ptr += write_length;
            remainder -= write_length;
        }
    }
    if (remainder > 0)
    {
        return TCPCLIENT_ERR_WRITING_TO_SERVER;
    }

    return TCPCLIENT_ERR_NO_ERROR;
}
