/*
 * config.h
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#ifndef __INCLUDE_MOD_CXX_CONFIG_H
#define __INCLUDE_MOD_CXX_CONFIG_H

#include "globals_private.h"
#include <map>

namespace bitforge
{

struct basic_pooled_object
{
public:
    void* operator new ( size_t _size, apr_pool_t* _pool );
};

struct handler_config
{
protected:
    const char* getHandlerName ( const char *_prefix, apr_pool_t* _pool );

    uint32_t id = 0;

public:
    handler_config();

    const char* type = nullptr;
    int vmajor = 0;
    int vminor = 0;

    bool operator== ( handler_config &_other ) const
    {
        if ( id && _other.id )
            return id == _other.id;

        return ( strcmp ( type, _other.type ) == 0 ) &&
               vmajor == _other.vmajor && vminor == _other.vminor;
    }

    uint32_t getID ( apr_pool_t* _pool );
};

struct database_config: public handler_config
{
public:
    static const char * s_prefix;

    const char* getHandlerName ( apr_pool_t* _pool )
    {
        return handler_config::getHandlerName ( s_prefix, _pool );
    }
};

struct protocol_def: public handler_config
{
public:
    static const char * s_prefix;

    protocol_def();

    const char* getHandlerName ( apr_pool_t* _pool )
    {
        return handler_config::getHandlerName ( s_prefix, _pool );
    }
};

struct server_config
{
public:
    const char* apps_dir = nullptr;
};

struct directory_config
{
public:
    //FIXME: This should be server only
    const char*			apps_dir = nullptr;

    database_config		database;

    value_pairs*		db_params = nullptr;
    value_pairs*		protocol_params = nullptr;
};

}

extern "C" void* create_server_config ( apr_pool_t* _pool, server_rec* _server );
extern "C" void* create_directory_config ( apr_pool_t* _pool, char* _dir );

extern "C" const char * set_apps_dir ( cmd_parms* _parms, void* _config, const char* _arg );

extern "C" const char * set_db_plugin ( cmd_parms* _parms, void* _config, const char* _type, const char* _major, const char* _minor );
extern "C" const char * set_db_plugin_params ( cmd_parms* _parms, void* _config, const char* _key, const char* _value );

extern "C" const char * set_protocol_params ( cmd_parms* _parms, void* _config, const char* _key, const char* _value );

#endif // __INCLUDE_MOD_CXX_CONFIG_H
