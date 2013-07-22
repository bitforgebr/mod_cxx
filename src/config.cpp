/*
 * config.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include <apr_pools.h>
#include "config.h"
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <bf/bf.h>

using namespace bitforge;

const char *directory_config::s_prefix = "libapp_";

apr_status_t destroy_pooled_object(void *_object)
{
    delete static_cast<basic_pooled_object *>(_object);
    return APR_SUCCESS;
}

void *basic_pooled_object::operator new(size_t _size, apr_pool_t *_pool)
{
    if (_pool)
        return apr_palloc(_pool, _size);
    else
    {
        void *result = malloc(_size);
        apr_pool_cleanup_register(_pool, static_cast<void *>(result), destroy_pooled_object, apr_pool_cleanup_null);
        return result;
    }
}

const char *directory_config::getHandlerName(apr_pool_t *_pool)
{
    std::stringstream name;
    name << s_prefix;
    name << app_name << ".so";

    return strpooldup(name.str().c_str(), name.tellp(), _pool);
}

uint32_t directory_config::getID()
{
    if (id)
        return id;

    id = fletcher32(app_name, strlen(app_name));

    return id;
}

void *create_server_config(apr_pool_t *_pool, server_rec *_server)
{
    server_config *result = static_cast<server_config *>(apr_pcalloc(_pool, sizeof(server_config)));
    result->config_type = ctServer;
    return result;
}

void *create_directory_config(apr_pool_t *_pool, char *_dir)
{
    directory_config *result = static_cast<directory_config *>(apr_pcalloc(_pool, sizeof(directory_config)));
    result->config_type = ctServer;
    return result;
}

const char *set_apps_dir(cmd_parms *_parms, void *_config, const char *_arg)
{
    if (_config)
    {
        const char *dir = nullptr;

        if (_arg)
        {
            /*
             * Check for acess permissions?
             */
            if (true || access(_arg, R_OK | X_OK))
            {
                dir = strpooldup(_arg, strlen(_arg), _parms->pool);
            }
            else
            {
                char *str = static_cast<char *>(apr_palloc(_parms->pool, strlen(_arg) + 9 + 1));
                memcpy(str, "invalid(", 8);
                strncpy(str + 8, _arg, strlen(_arg));
                memcpy(str + 8 + strlen(_arg), ")\0", 2);

                dir = str;
            }
        }
        else
        {
            dir = strpooldup("[NULL]", 7, _parms->pool);
        }

        if (static_cast<abstract_config *>(_config)->config_type == ctServer)
        {
            server_config *sc = static_cast<server_config *>(_config);
            sc->apps_dir = dir;
        }
        else
        {
            directory_config *dc = static_cast<directory_config *>(_config);
            dc->apps_dir = dir;
        }

    }

    return NULL;
}


const char *set_app_name(cmd_parms *_parms, void *_config, const char *_arg)
{
    if (_config && static_cast<abstract_config *>(_config)->config_type == ctDirectory)
    {
        directory_config *dc = static_cast<directory_config *>(_config);

        if (_arg)
        {
            dc->app_name = strpooldup(_arg, strlen(_arg), _parms->pool);
        }
        else
        {
            dc->app_name = strpooldup("[NULL]", 7, _parms->pool);
        }
    }

    return NULL;
}


const char *set_app_params(cmd_parms *_parms, void *_config, const char *_key, const char *_value)
{
    if (_config && _key && static_cast<abstract_config *>(_config)->config_type == ctDirectory)
    {
        directory_config *dc = static_cast<directory_config *>(_config);

        value_pairs *map = static_cast<value_pairs *>(apr_pcalloc(_parms->pool, sizeof(value_pairs)));

        if (!dc->app_config)
        {
            dc->app_config = map;
        }
        else
        {
            value_pairs *it = dc->app_config;

            while (it->next)
                it = it->next;

            it->next = map;
        }

        map->first = strpooldup(_key, _parms->pool);

        if (_value)
        {
            map->second = strpooldup(_value, _parms->pool);
        }
        else
            map->second = 0;
    }

    return NULL;
}

