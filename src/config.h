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
    void *operator new(size_t _size, apr_pool_t *_pool);
};

enum ConfigType
{
    ctNone,
    ctServer,
    ctDirectory
};

struct abstract_config
{
public:
    ConfigType config_type;
};

struct server_config: public abstract_config
{
public:
    const char *apps_dir = nullptr;
};

struct directory_config: public abstract_config
{
protected:
    static const char *s_prefix;

    uint32_t id = 0;

public:
    const char *apps_dir = nullptr;
    const char  *app_name = nullptr;
    value_pairs *app_config = nullptr;

    const char *getHandlerName(apr_pool_t *_pool);

    bool operator==(directory_config &_other) const
    {
        if (id && _other.id)
            return id == _other.id;

        return (strcmp(app_name, _other.app_name) == 0);
    }

    uint32_t getID();
};

}

extern "C" void *create_server_config(apr_pool_t *_pool, server_rec *_server);
extern "C" void *create_directory_config(apr_pool_t *_pool, char *_dir);

extern "C" const char *set_apps_dir(cmd_parms *_parms, void *_config, const char *_arg);

extern "C" const char *set_app_name(cmd_parms *_parms, void *_config, const char *_arg);
extern "C" const char *set_app_params(cmd_parms *_parms, void *_config, const char *_key, const char *_value);

#endif // __INCLUDE_MOD_CXX_CONFIG_H
