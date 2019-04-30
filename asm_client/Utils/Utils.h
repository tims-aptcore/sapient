//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include <time.h>
#include <string>
#include <chrono>

// Sleep for the given period in milliseconds. Causes a thread switch, so may sleep longer on a busy CPU.
void Sleep_ms( int period );

// Return the time in seconds since an unspecified start point.
double Get_Time_Monotonic();

// Return the system time as an ISO 8601 formatted string with fractional seconds
std::string Get_Timestamp( std::chrono::system_clock::time_point tp );

// Returns true if the network interface associated with the address is known to be down
bool Network_Link_Down( std::string hostname );