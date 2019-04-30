//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

class Hardware
{
public:
    virtual ~Hardware() {};
    virtual void Initialise( const char *configFilename ) = 0;
    virtual void Loop( struct AsmClientStatus &status, const struct AsmClientData &data ) = 0;
};
