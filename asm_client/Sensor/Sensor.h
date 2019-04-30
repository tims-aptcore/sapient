//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

class Sensor
{
public:
    virtual ~Sensor() {};
    virtual void Initialise( const char *configFilename ) = 0;
    virtual void Loop( const struct AsmClientTask &task, struct AsmClientData &data ) = 0;
};
