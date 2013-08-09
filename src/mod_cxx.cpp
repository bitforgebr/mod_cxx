/*
 * mod_cxx.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include <stdio.h>
#include <stdlib.h>

#include "mod_cxx.h"
#include "config.h"
#include "handlers/apachehandler.h"

/*
 * This prag is needed because of apache's macros do not initialize
 * all fields.
 */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define CAST_FN(FN) reinterpret_cast<const char*(*)()>( FN )

extern "C" const command_rec config_comands[] =
{
    // Server configs
    AP_INIT_TAKE1("CxxAppsDir", CAST_FN(set_apps_dir), nullptr, RSRC_CONF | ACCESS_CONF,
                  "Directory where protocol and database plugins are found."),

    // Directory configs
    AP_INIT_TAKE1("CxxApp", CAST_FN(set_app_name), nullptr, ACCESS_CONF,
                  "Application to load."),
    AP_INIT_ITERATE2("CxxAppParams", CAST_FN(set_app_params), nullptr, ACCESS_CONF,
                     "Params to be passed to the application."),

    { nullptr }
};

#undef CAST_FN
#pragma GCC diagnostic pop

extern "C" void cxx_hooks(apr_pool_t*)
{
    ap_hook_handler(apache_request_handler, nullptr, nullptr, APR_HOOK_MIDDLE);
}

VISIBILITY module AP_MODULE_DECLARE_DATA cxx_module =
{
    STANDARD20_MODULE_STUFF,
    create_directory_config,    /* per-directory config struct */
    nullptr,                    /* merge per-directoy config struct */
    create_server_config,       /* per-host config struct */
    nullptr,                    /* merge per-host config struct */
    config_comands,             /* config for this module */
    cxx_hooks                   /* register this module's hooks */
};
