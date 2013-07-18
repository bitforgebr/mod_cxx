/*
 * config.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include "config.h"
#include <cstdlib>
#include <apr_pools.h>
#include <unistd.h>
#include <sstream>
#include <bf/bf.h>

using namespace bitforge;

const char* database_config::s_prefix = "libdb_";
const char* protocol_def::s_prefix = "libapp_";

apr_status_t destroy_pooled_object ( void* _object )
{
    delete static_cast<basic_pooled_object*> ( _object );
    return APR_SUCCESS;
}

void * basic_pooled_object::operator new ( size_t _size, apr_pool_t * _pool )
{
    if ( _pool )
        return apr_palloc ( _pool, _size );
    else
    {
        void * result = malloc ( _size );
        apr_pool_cleanup_register ( _pool, static_cast<void*> ( result ), destroy_pooled_object, apr_pool_cleanup_null );
        return result;
    }
}

handler_config::handler_config()
{
}

const char* handler_config::getHandlerName ( const char * _prefix, apr_pool_t* _pool )
{
    std::stringstream name;
    name << _prefix;
    name << type << ".so";

    if ( vmajor )
        name << '.' << vmajor;

    if ( vminor )
        name << '.' << vminor;

    return strpooldup ( name.str().c_str(), name.tellp(), _pool );
}

uint32_t handler_config::getID( apr_pool_t* _pool )
{
    if ( id )
        return id;

    int dataLen = ( sizeof ( int ) * 2 ) + strlen ( type );

    uint16_t *data = reinterpret_cast<uint16_t*> ( apr_palloc ( _pool, dataLen ) );

    int *p = reinterpret_cast<int*> ( data );
    *p++ = vmajor;
    *p++ = vminor;

    strncpy(reinterpret_cast<char*> ( p ), type, dataLen - ( sizeof ( int ) * 2 ));

    id = fletcher32( (char*)data, dataLen );
    return id;
}

protocol_def::protocol_def()
{
    // Default type if not set by the peer
    type = "protobuf";
}

void * create_server_config(apr_pool_t * _pool, server_rec*)
{
    return apr_pcalloc ( _pool, sizeof ( server_config ) );
}

void * create_directory_config(apr_pool_t * _pool, char*)
{
    return apr_pcalloc ( _pool, sizeof ( directory_config ) );
}

const char * set_apps_dir ( cmd_parms * _parms, void * _config, const char * _arg )
{
    if ( _config )
    {
        const char *dir = 0;

        if ( _arg )
        {
            if ( true || access ( _arg, R_OK | X_OK ) )
            {
                dir = strpooldup ( _arg, strlen ( _arg ), _parms->pool );
            }
            else
            {
                char* str = static_cast<char*> ( apr_palloc ( _parms->pool, strlen ( _arg ) + 9 + 1 ) );
                memcpy ( str, "invalid(", 8 );
                strncpy ( str + 8, _arg, strlen ( _arg ) );
                memcpy ( str + 8 + strlen ( _arg ), ")\0", 2 );

                dir = str;
            }
        }
        else
        {
            dir = strpooldup ( "[NULL]", 7, _parms->pool );
        }

        if ( ap_check_cmd_context ( _parms, NOT_IN_DIR_LOC_FILE ) )
            static_cast<server_config*> ( _config )->apps_dir = dir;
        else
            static_cast<directory_config*> ( _config )->apps_dir = dir;
    }

    return NULL;
}

const char * set_db_plugin ( cmd_parms * _parms, void * _config, const char * _type, const char * _major, const char * _minor )
{
    char *end = 0;
    long int value;

    if ( _type && _config )
    {
        static_cast<directory_config*> ( _config )->database.type = strpooldup ( _type, _parms->pool );

        if ( _major )
        {
            value = strtol ( _major, &end, 0 );
            if ( *end == 0 )
                static_cast<directory_config*> ( _config )->database.vmajor = value;
        }

        if ( _minor )
        {
            value = strtol ( _minor, &end, 0 );
            if ( *end == 0 )
                static_cast<directory_config*> ( _config )->database.vminor = value;
        }
    }

    return NULL;
}

const char * set_db_plugin_params ( cmd_parms * _parms, void * _config, const char * _key, const char * _value )
{
    if ( _config && _key )
    {
        value_pairs *map = static_cast<value_pairs*> ( apr_pcalloc ( _parms->pool, sizeof ( value_pairs ) ) );

        if ( ! static_cast<directory_config*> ( _config )->db_params )
        {
            static_cast<directory_config*> ( _config )->db_params = map;
        }
        else
        {
            value_pairs *it = static_cast<directory_config*> ( _config )->db_params;
            while ( it->next )
                it = it->next;

            it->next = map;
        }

        map->first = strpooldup ( _key, _parms->pool );

        if ( _value )
            map->second = strpooldup ( _value, _parms->pool );
        else
            map->second = 0;
    }
    return NULL;
}

const char * set_protocol_params ( cmd_parms * _parms, void * _config, const char * _key, const char * _value )
{
    if ( _config && _key )
    {
        value_pairs *map = static_cast<value_pairs*> ( apr_pcalloc ( _parms->pool, sizeof ( value_pairs ) ) );

        if ( ! static_cast<directory_config*> ( _config )->protocol_params )
        {
            static_cast<directory_config*> ( _config )->protocol_params = map;
        }
        else
        {
            value_pairs *it = static_cast<directory_config*> ( _config )->protocol_params;
            while ( it->next )
                it = it->next;

            it->next = map;
        }

        map->first = strpooldup ( _key, _parms->pool );

        if ( _value )
            map->second = strpooldup ( _value, _parms->pool );
        else
            map->second = 0;
    }
    return NULL;
}
