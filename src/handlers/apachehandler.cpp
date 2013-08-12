/*
 * apachehandler.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include "mod_cxx.h"
#include "config.h"

#include "handlerfactory.h"
#include "apachehandler.h"

#include <cassert>


#define SESSION_FILE_PREFIX "mod_bfms_sess_"

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

extern "C" int apache_request_handler(request_rec* _request)
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

        if (!apps_dir)
        {
            ap_log_error(APLOG_MARK, LOG_ERR, 0, _request->server, "Missing apps directory in apache's config.");
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        handlerFactory = new HandlerFactory(_request, _request->server->process->pool, apps_dir);

        handlerIO.setAppsDir(apps_dir);
        handlerIO.setAppConfig(dir_conf->app_config);
    }

    HandlerCache* handler = handlerFactory->getHandler(_request, dir_conf);

    if(handler)
    {
        handlerIO.setRequest(_request);
        handlerIO.setAppConfig(dir_conf->app_config);

        if(setjmp(handlerIO.err_env) == 0)
        {
            handler->handlerfn(&handlerIO);
            return OK;
        }
        else
        {
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }
    else
    {
        ap_log_error(APLOG_MARK, LOG_WARNING, 0, _request->server, "Error finding plugin: '%s'", dir_conf->app_name);
    }

    return HTTP_INTERNAL_SERVER_ERROR;
}

namespace bitforge
{

apr_time_t ApacheHanlderIO::m_lastCheck = 0;

ApacheHanlderIO::ApacheHanlderIO()
    : HandlerIO()
{
}

ApacheHanlderIO::~ApacheHanlderIO()
{
    if ( m_isOpen )
        close();

    checkCleanup();
}

bool ApacheHanlderIO::open(const char* contentType)
{
    if ( m_isOpen )
        return false;

    m_outbb = apr_brigade_create(m_pool, m_request->connection->bucket_alloc);
    ap_set_content_type(m_request, contentType);
    return m_isOpen = m_outbb != NULL;
}

bool ApacheHanlderIO::close()
{
    if ( !m_isOpen )
        return false;

    m_isOpen = false;

    if ( m_session )
    {
        delete m_session;
        m_session = nullptr;
    }

    if ( m_outbb )
    {
        APR_BRIGADE_INSERT_TAIL(m_outbb, apr_bucket_eos_create(m_outbb->bucket_alloc));

        return ( ap_pass_brigade(m_request->output_filters, m_outbb) == APR_SUCCESS )
               && ( m_inbb && apr_brigade_cleanup(m_inbb) == APR_SUCCESS );
    }

    return true;
}

void ApacheHanlderIO::write(const char *_data)
{
    ap_fputs(m_request->output_filters, m_outbb, _data);

    // This gives fine-grained control, but is more complicated to get it 'right'
    /*	apr_bucket *b = apr_bucket_pool_create( strpooldup(_data, m_pool), strlen(_data), m_pool, m_outbb->bucket_alloc);
    	APR_BRIGADE_INSERT_TAIL( m_outbb, b );
    */
}

std::size_t ApacheHanlderIO::write(const void *_data, std::size_t _length)
{
    ap_fwrite(m_request->output_filters, m_outbb, static_cast<const char*>(_data), _length);

    // This gives fine-grained control, but is more complicated to get it 'right'
    /*	void *buffer = apr_palloc( m_pool, _length );
    	memcpy(buffer, _data, _length);
    	apr_bucket *b = apr_bucket_pool_create( static_cast<char*>( buffer ), _length, m_pool, m_outbb->bucket_alloc);

    	APR_BRIGADE_INSERT_TAIL(m_outbb, b);
    */

    return _length;
}

bool ApacheHanlderIO::checkInput(std::size_t &_length)
{
    if (!m_inbb)
    {
        //First, check if there is any input.
        const char *len = apr_table_get(m_request->headers_in, "Content-Length");

        if ( len && atoi(len)
                //Do I need this?
                //&& strcasecmp(apr_table_get(m_request->headers_in, "Transfer-Encoding"), "chunked") != 0
           )
        {
            if (!_length)
                _length = atoi(len) + 1;

            // Create a brigade
            m_inbb = apr_brigade_create(m_pool, m_request->connection->bucket_alloc);

            // Assign input data to the brigade
            if ( ap_get_brigade(m_request->input_filters, m_inbb, AP_MODE_READBYTES, APR_BLOCK_READ, _length) == APR_SUCCESS )
            {
                return m_inbb;
            }
        }

        if ( !len )
            return false;

        APACHE_LOG(APLOG_ERR, OK, 0, m_request->server, "checkInput(" << len << " - " << atoi(len) << ") - shouldn't happen.");
    }

    APACHE_LOG(APLOG_ERR, OK, 0, m_request->server, "checkInput(" << _length << ") - shouldn't happen.");
    return false;
}

std::size_t ApacheHanlderIO::read(const char **_data, std::size_t _length)
{
#define IS_EOS (m_inBucket == APR_BRIGADE_SENTINEL(m_inbb) || APR_BUCKET_IS_EOS(m_inBucket))

    // This param is useless for Apache's bickets.  It will be overwritten anyway.
    _length = 0;

    if (m_inBucket || checkInput(_length))
    {
        if (!m_inBucket)
            m_inBucket = APR_BRIGADE_FIRST(m_inbb);
        else
            m_inBucket = APR_BUCKET_NEXT(m_inBucket);

        if (!IS_EOS)
        {
            while( APR_BUCKET_IS_METADATA(m_inBucket) && !IS_EOS )
                APR_BUCKET_NEXT(m_inBucket);

            if ( !IS_EOS )
            {
                apr_status_t status = apr_bucket_read(m_inBucket, _data, &_length, APR_BLOCK_READ);
                if ( status == APR_SUCCESS )
                {
                    const_cast<char*>(*_data)[_length] = 0;
                    return _length;
                }
            }
        }
    }

    _length = 0;
    return _length;

#undef IS_EOS
}

int readTable(void *_p, const char *_key, const char* _value)
{
    ApacheHanlderIO* _this = static_cast<ApacheHanlderIO*>( _p );

    _this->m_tmpList->next = static_cast<value_pairs*>( apr_pcalloc( _this->m_pool, sizeof(value_pairs) ));
    _this->m_tmpList = _this->m_tmpList->next;

    _this->m_tmpList->first = strpooldup( _key, _this->m_pool );
    _this->m_tmpList->second = strpooldup( _value, _this->m_pool );

    return -1; // continue
}

value_pairs* ApacheHanlderIO::getHeaders(const char* _header)
{
    value_pairs result;

    m_tmpList = &result;
    apr_table_do(readTable, this, m_request->headers_in, _header, NULL);

    if ( result.next )
        return result.next;
    else
        return NULL;
}


void *ApacheHanlderIO::malloc(std::size_t size)
{
    return apr_palloc( m_pool, size );
}

// convert Deleter to apache-style which returns APR_SUCCESS
static apr_status_t apacheDeleter(void* ptr)
{
    ApacheHanlderIO::Deleter *delRef = static_cast<ApacheHanlderIO::Deleter*>(ptr);
    auto data = (char*)ptr + sizeof(ApacheHanlderIO::Deleter);

    (*delRef)(data);

    return APR_SUCCESS;
}

void* ApacheHanlderIO::malloc(std::size_t size, AllocScope scope, Deleter d)
{
    apr_pool_t* pool;

    switch(scope)
    {
        case asServer:
        case asConfig:
            pool = getProcessPool();
            break;

        case asConection:
            pool = getConnectionPool();
            break;

        case asRequest:
            pool = getRequestPool();
            break;
    }

    auto ptr = apr_palloc(pool, size + sizeof(Deleter));

    // Copy the deleter to the new data
    Deleter *delRef = static_cast<Deleter*>(ptr);
    *delRef = std::move(d);
    // Pass the rest of the alocated data as result
    auto result = (char*)ptr + sizeof(Deleter);

    apr_pool_cleanup_register(pool, ptr, apacheDeleter, apr_pool_cleanup_null);

    return result;
}

void ApacheHanlderIO::free(void *)
{
    //No op!
    //Apache will free the pointer when it frees the pool
}

void ApacheHanlderIO::logError(ErrorLevel _level, const char* _message)
{
    int level;
    apr_status_t status = OK;

    switch( _level )
    {
    case elStatus:
        level = APLOG_NOTICE;
        break;
    case elWarning:
        level = APLOG_WARNING;
        break;
    case elError:
        level = APLOG_ERR;
        break;
    case elFatal:
        level = APLOG_CRIT;
        break;
    };

    APACHE_LOG(level, status, 0, m_request->server, _message);
}

void ApacheHanlderIO::abort(const char* _message)
{
    APACHE_LOG(APLOG_CRIT, OK, 0, m_request->server, _message);
    longjmp( err_env, -1 );
}

static void unescapeInput(char *_str)
{
    char c = 0;
    char *s = _str;

    char buffer[3];
    buffer[2] = 0;

    while( true )
    {
        switch ( *_str )
        {
        case 0:
            *s = 0;
            return;

        case '+':
            c = ' ';
            break;

        case '%':
            memcpy(buffer, _str + 1, 2);
            c = strtol(buffer, NULL, 16);
            _str += 2;
            break;

        default:
            c = *_str;
            break;
        }

        *s++ = c;
        _str++;
    }
}

static value_pairs* readValues(char *_values, apr_pool_t *_pool)
{
    char *s, *v, *p;
    p = _values;

    value_pairs *result = nullptr;

    if ( p )
    {
        value_pairs *last = nullptr;

        while ( *p )
        {
            s = p;
            v = nullptr;

            while ( *p )
            {
                if ( *p == '&' )
                {
                    *p++ = 0;
                    break;
                }

                if ( *p == '=' )
                {
                    *p = 0;

                    if ( *++p == '&' )
                        continue;

                    v = p;
                }
                p++;
            }

            if ( p > s )
            {
                unescapeInput(s);

                if ( v )
                    unescapeInput(v);

                value_pairs *pair = (value_pairs*)apr_palloc( _pool, sizeof(value_pairs));
                pair->first = s;
                pair->second = v;

                if (!result)
                {
                    result = last = pair;
                }
                else
                {
                    last->next = pair;
                    last = pair;
                }
            }
        }
    }

    return result;
}

const value_pairs* ApacheHanlderIO::getGET()
{
    if ( m_getVars )
        return m_getVars;

    if ( m_request->args && strlen( m_request->args ))
    {
        m_getVars = readValues(strpooldup( m_request->args, m_pool ), m_pool);
    }

    return m_getVars;
}

const value_pairs* ApacheHanlderIO::getPUT()
{
    if ( m_putVars )
        return m_putVars;

    const char *buffer;
    size_t size = 0;

    read( &buffer , size );

    if ( size )
    {
        m_putVars = readValues(strpooldup( buffer, size, m_pool ), m_pool);
    }

    return m_putVars;
}

//#define debugline ap_log_rerror(__FILE__, __LINE__, APLOG_ERR, OK, m_parent->m_request, "cookie: %s:%d", basename(__FILE__), __LINE__ )

Session* ApacheHanlderIO::getSession()
{
    return nullptr;
}

void ApacheHanlderIO::checkCleanup()
{
}

}
