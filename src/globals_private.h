/*
 * globals_private.h
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include "globals.h"

#ifndef __INCLUDE_MOD_CXX_GLOBALS_PRIVATE_H
#define __INCLUDE_MOD_CXX_GLOBALS_PRIVATE_H

#define MODULE_NAME "cxx"

#include <glob.h>
#include <string>

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>


/**
 * Create a new copy of string using apr pools
 * @param _src The source string to read from
 * @param _length Maximum length of the string
 * @param _pool The pool to allocate from
 * @return The allocated memory
 */
inline char* strpooldup ( const char* _src, std::size_t _length, apr_pool_t* _pool )
{
    char* result = static_cast<char*> ( apr_palloc ( _pool, _length + 1 ) );
    strncpy ( result, _src, _length );
    result[_length] = 0;
    return result;
}

/**
 * Create a new copy of string using apr pools
 * @param _src The source string to read from
 * @param _pool The pool to allocate from
 * @return The allocated memory
 */
inline char* strpooldup ( const char* _src, apr_pool_t* _pool )
{
    return strpooldup ( _src, strlen ( _src ), _pool );
}

#endif // __INCLUDE_MOD_CXX_GLOBALS_PRIVATE_H
