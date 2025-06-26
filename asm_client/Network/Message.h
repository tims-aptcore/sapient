//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "ProtobufInterface/Writer.h"

#include <string>

class Message
{
public:
    Message( std::string description ) { _description = description; }

    virtual bool Write( ProtobufInterface::Writer *w ) = 0;

    virtual std::string GetDescription() { return _description; }

protected:
    std::string _description;
};
