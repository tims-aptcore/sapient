//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include "Stream.h"

namespace ProtobufInterface
{

class Writer
{
    IOutputStream *_pStream;

public:
    bool open(IOutputStream *);
    void close();
    bool writeBytes( unsigned char * bytes, int len );
    Writer();
};

};
