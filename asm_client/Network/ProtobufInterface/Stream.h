// Copyright © 2008-2011 Rick Parrish
// https://github.com/Unitrunker

#pragma once

#include <stddef.h>

struct IInputStream
{
    virtual bool Read(unsigned char *pOctets, size_t iOctets, size_t &iRead) = 0;
    virtual bool ReadWithTimeout(unsigned char *pOctets, size_t iOctets, size_t &iRead, int millisecs) = 0;
    virtual void Close() = 0;
};

struct IOutputStream
{
    virtual bool Write(unsigned char *pOctets, size_t iOctets, size_t &iWrote) = 0;
    virtual bool Send(bool bTerminator) = 0;
    virtual void Close() = 0;
};
