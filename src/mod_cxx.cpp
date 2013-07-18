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
#include "handlers/default.h"

/*
 * This prag is needed because of apache's macros do not initialize
 * all fields.
 */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define CAST_FN(FN) reinterpret_cast<const char*(*)()>( FN )

extern "C" const command_rec config_comands[] =
{
    // Server configs
    AP_INIT_TAKE1("apps_dir", CAST_FN(set_apps_dir), nullptr, RSRC_CONF | ACCESS_CONF,
                  "Directory where protocol and database plugins are found."),

    // Directory configs
    AP_INIT_TAKE3("db_plugin", CAST_FN(set_db_plugin), nullptr, RSRC_CONF | ACCESS_CONF,
                  "Database plugin type."),

    AP_INIT_ITERATE2("db_param", CAST_FN(set_db_plugin_params), nullptr, RSRC_CONF | ACCESS_CONF,
                     "Params to be passed onto the database."),

    AP_INIT_ITERATE2("protocol_params", CAST_FN(set_protocol_params), nullptr, RSRC_CONF | ACCESS_CONF,
                     "Params to be passed onto the protocol plugin."),

    { nullptr }
};

#undef CAST_FN
#pragma GCC diagnostic pop

extern "C" void cxx_hooks(apr_pool_t*)
{
    ap_hook_handler(default_handler, nullptr, nullptr, APR_HOOK_MIDDLE);
}

VISIBILITY module AP_MODULE_DECLARE_DATA cxx_module =
{
    STANDARD20_MODULE_STUFF,
    create_directory_config,		/* per-directory config struct */
    nullptr,						/* merge per-directoy config struct */
    create_server_config,			/* per-host config struct */
    nullptr,						/* merge per-host config struct */
    config_comands,					/* config for this module */
    cxx_hooks						/* register this module's hooks */
};
