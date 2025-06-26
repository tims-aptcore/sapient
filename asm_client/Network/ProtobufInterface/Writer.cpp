//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "Writer.h"

namespace ProtobufInterface
{

Writer::Writer() :
    _pStream(NULL)
{
}


bool Writer::open(IOutputStream *pStream)
{
    if (_pStream != NULL && _pStream != pStream)
        _pStream->Close();
    _pStream = pStream;
    return true;
}


void Writer::close()
{
    _pStream->Close();
    _pStream = NULL;
}


bool Writer::writeBytes( unsigned char* bytes, int len )
{
    size_t sz_len = len;
    size_t wrote = 0;
    return _pStream->Write( bytes, sz_len, wrote );
}

};
