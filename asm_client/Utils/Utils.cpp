//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include <time.h>
#include <chrono>
#include <ctime>
#include <string>

#ifdef __unix__
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#else // windows
#include <Windows.h>
#endif

void Sleep_ms( int period )
{
#ifdef __unix__
    usleep( period * 1000 );
#else // windows
    Sleep( period );
#endif
}

double Get_Time_Monotonic()
{
#ifdef __unix__
    struct timespec monotonic_timespec;
    clock_gettime( CLOCK_MONOTONIC, &monotonic_timespec );
    return monotonic_timespec.tv_sec + monotonic_timespec.tv_nsec / (1e9);
#else // windows
    static LARGE_INTEGER count, frequency = { 0 };
    if (frequency.QuadPart == 0) QueryPerformanceFrequency( &frequency );
    QueryPerformanceCounter( &count );
    return count.QuadPart / (double)frequency.QuadPart;
#endif
}

std::string Get_Timestamp( std::chrono::system_clock::time_point tp )
{
    // Split the time in to seconds since the epoch and a fractional part
    std::chrono::seconds tp_seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
    std::chrono::duration<long long, std::nano> tp_fractional_nanoseconds = tp.time_since_epoch() - tp_seconds_since_epoch;

    // Generate the ISO 8601 timestamp from the seconds since the epoch
    char timestamp[] = "yyyy-mm-ddThh:mm:ss";
    std::time_t tp_time_t = std::chrono::system_clock::to_time_t( std::chrono::system_clock::time_point( tp_seconds_since_epoch ) );
#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif
    if (strftime( timestamp, sizeof( timestamp ), "%FT%T", std::gmtime( &tp_time_t ) ) == 0)
        return "";

    // Add the fractional milliseconds with leading zeros and return
    std::string milliseconds = std::to_string( tp_fractional_nanoseconds.count() / 1000000 );
    return std::string( timestamp ) + "." + std::string( 3 - milliseconds.length(), '0' ) + milliseconds + "Z";
}

bool Network_Link_Down( std::string hostname )
{
#ifdef __unix__
    struct ifaddrs *ifAddrs, *ifAddr;
    if (getifaddrs( &ifAddrs ) == 0)
    {
        for (ifAddr = ifAddrs; ifAddr != NULL; ifAddr = ifAddr->ifa_next)
        {
            bool linkIsUp = ifAddr->ifa_flags & IFF_RUNNING;
            if (ifAddr->ifa_addr == NULL) continue;
            if (ifAddr->ifa_addr->sa_family != AF_INET) continue;
            if (std::string( ifAddr->ifa_name ) == "lo") continue;
            freeifaddrs( ifAddrs );
            return !linkIsUp;
        }
        freeifaddrs( ifAddrs );
    }
#endif
    return false;
}
