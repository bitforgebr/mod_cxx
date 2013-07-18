/*
 * default.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include "mod_cxx.h"
#include "default.h"
#include "config.h"
#include "handlers.h"
#include "handlerfactory.h"
#include "apachehandler.h"

#include <globals.h>

#include <http_log.h>

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(cxx);
#endif 

using namespace bitforge;

static HandlerFactory* handlerFactory = nullptr;

#ifndef NDEBUG
int print_headers(void* _r, const char* _key, const char* _value)
{
    request_rec* _request = static_cast<request_rec*>(_r);

    ap_rputs("<tr><td>", _request);
    ap_rputs(_key, _request);
    ap_rputs("</td><td>", _request);
    ap_rputs(_value, _request);
    ap_rputs("</td></tr>", _request);

    return -1; // continue
}

void print_value_pairs(const value_pairs* _pairs, request_rec* _request)
{
    ap_rputs("<table>", _request);

    const value_pairs* it = _pairs;

    while(it)
    {
        if(!it->first.empty())
        {
            ap_rputs("<tr><td>", _request);
            ap_rputs(it->first.c_str(), _request);
            ap_rputs("</td><td>", _request);

            if(!it->second.empty())
            {
                ap_rputs(it->second.c_str(), _request);
            }
            else
            {
                ap_rputs("[NULL]", _request);
            }

            ap_rputs("</td></tr>\n", _request);
        }

        it = it->next;
    }

    ap_rputs("</table>", _request);
}
#endif

int select_protocol_ver(void* _p, const char* _key, const char* _value)
{
    if(strncasecmp(_key, "bfms-protocol", 12) == 0)
    {
        if(_value)
        {
            if(strncasecmp(_key + 12, "-type", 5) == 0)
            {
                static_cast<protocol_def*>(_p)->type = _value;
            }
            else
            {
                char* end = 0;
                long int value = strtol(_value, &end, 0);

                if(*end == 0)
                {
                    if(strncasecmp(_key + 12, "-major", 6) == 0)
                    {
                        static_cast<protocol_def*>(_p)->vmajor = value;
                    }
                    else if(strncasecmp(_key + 12, "-minor", 6) == 0)
                    {
                        static_cast<protocol_def*>(_p)->vminor = value;
                    }
                }
            }
        }
    }

    return -1; // i.e. continue looking
}

extern "C" int default_handler(request_rec* _request)
{
    // Verify that the request belongs to this handler
    if(!_request || strcmp(_request->handler, MODULE_NAME) != 0)
    {
#ifndef NDEBUG
        ap_log_error(APLOG_MARK, LOG_WARNING, 0, _request->server, "Invalid module name: '%s' != '%s'", _request->handler, MODULE_NAME);
#endif
        return DECLINED;
    }

    // Verify the method
    if((_request->method_number != M_GET) && (_request->method_number != M_POST) && (_request->method_number != M_PUT))
    {
        return HTTP_METHOD_NOT_ALLOWED;
    }

    server_config* srv_conf = static_cast<server_config*>(ap_get_module_config(_request->server->module_config, &cxx_module));
    directory_config* dir_conf = static_cast<directory_config*>(ap_get_module_config(_request->per_dir_config, &cxx_module));

    //Todo - Cache this?
    ApacheHanlderIO handlerIO;

    if(!handlerFactory)
    {
        const char* apps_dir;

        if(srv_conf && srv_conf->apps_dir)
        {
            apps_dir = srv_conf->apps_dir;
        }
        else if(dir_conf && dir_conf->apps_dir)
        {
            apps_dir = dir_conf->apps_dir;
        }
        else
            apps_dir = nullptr;

//		Pools don't work?
// 		handlerFactory = static_cast<HandlerFactory*>( apr_palloc( _request->server->process->pool, sizeof(HanlderFactory) ) );
// 		*handlerFactory = PluginFactory( _request, _request->server->process->pool, apps_dir );
//
// 		handlerIO = static_cast<ApacheHanlderIO*>( apr_palloc( _request->server->process->pool, sizeof(ApacheHanlderIO) ) );
// 		*handlerIO = ApacheHanlderIO();

        handlerFactory = new HandlerFactory(_request, _request->server->process->pool, apps_dir);

        handlerIO.setAppsDir(apps_dir);
    }

    protocol_def protocol;
    apr_table_do(select_protocol_ver, &protocol, _request->headers_in, NULL);

    HandlerCache* handler = handlerFactory->getProtocolHandler(_request, &protocol);

    if(handler)
    {
        handlerIO.setRequest(_request);
        handlerIO.setDatabaseConfig(dir_conf->db_params);
        handlerIO.setProtocolConfig(dir_conf->protocol_params);

        if(setjmp(handlerIO.err_env) == 0)
        {
            handler->handlerfn(&handlerIO);
            return OK;
        }
        else
        {
            handlerIO.write("<html><body><h1>Internal server error</h1></html></body>");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }
    else
    {
        ap_log_error(APLOG_MARK, LOG_WARNING, 0, _request->server, "Error finding plugin: '%s'", protocol.type);
    }

    return HTTP_INTERNAL_SERVER_ERROR;
}

