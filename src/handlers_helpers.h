/*
 * handlers.h
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#ifndef __INCLUDE_MOD_CXX_HANDLERS_HELPERS_H
#define __INCLUDE_MOD_CXX_HANDLERS_HELPERS_H

#include <bf/inthex.h>
#include <handlers.h>

#include <cassert>

namespace bitforge
{

inline HandlerIO& operator<< ( HandlerIO& _handlerIO, const std::string& str )
{
    _handlerIO.write ( str.c_str(), str.length() );
    return _handlerIO;
}

inline HandlerIO& operator<< ( HandlerIO& _handlerIO, const char chr )
{
    _handlerIO.write ( &chr, 1 );
    return _handlerIO;
}

inline HandlerIO& operator<< ( HandlerIO& _handlerIO, const char* const str )
{
    _handlerIO.write ( str, strlen ( str ) );
    return _handlerIO;
}

inline HandlerIO& operator<< ( HandlerIO& _handlerIO, int value )
{
    const int bufferSize = 32;
    char buffer[bufferSize + 1];
    const char* v = inttostr ( buffer, bufferSize, value );

    ::std::size_t len = ( bufferSize - ( v - buffer ) ) - 1;

    _handlerIO.write ( v, len );
    return _handlerIO;
}

inline HandlerIO& operator<< ( HandlerIO& _handlerIO, double value )
{
    const int bufferSize = 32;
    char buffer[bufferSize + 1];

    snprintf ( buffer, bufferSize, "%g", value );

    _handlerIO.write ( buffer, strlen ( buffer ) );
    return _handlerIO;
}

}

#endif // __INCLUDE_MOD_CXX_HANDLERS_HELPERS_H
