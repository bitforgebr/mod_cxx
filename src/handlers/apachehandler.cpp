/*
 * apachehandler.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include "apachehandler.h"

#include <http_log.h>

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(cxx);
#endif 

#define SESSION_FILE_PREFIX "mod_bfms_sess_"

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

        ap_log_rerror(__FILE__, __LINE__, APLOG_ERR, OK, 0, m_request, "checkInput(%s - %d) - shouldn't happen.", len, atoi(len));
    }

    ap_log_rerror(__FILE__, __LINE__, APLOG_ERR, OK, 0, m_request, "checkInput(%ld) - shouldn't happen.", _length);
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

    ap_log_rerror(__FILE__, __LINE__, level, 0, status, m_request, "%s", _message);
}

void ApacheHanlderIO::abort(const char* _message)
{
    ap_log_rerror(__FILE__, __LINE__, APLOG_CRIT, 0, 0, m_request, "%s", _message);
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

class ApacheSession: public Session
{
private:
    ApacheHanlderIO	*m_parent;
    value_pairs		*m_values;
    long long unsigned int		m_uuid;
    const char		*m_fileName;

    void init()
    {
#define BUFFER_SIZE 256
        char buffer[BUFFER_SIZE];

        value_pairs *cookie = m_parent->getHeaders("Cookie");

        if ( cookie && !cookie->second.empty() )
        {
            strncpy( buffer, cookie->second.c_str(), BUFFER_SIZE );
            buffer[BUFFER_SIZE - 1] = 0;
            char *p = buffer;

            while( *p )
            {
                if ( strncmp("SESSIONID=", p, 10) == 0 )
                {
                    p += 10;
                    char *e = p;

                    while ( *e && ( *e != ';' ) && ( !isspace( *e )))
                        e++;

                    *e = 0;

                    m_uuid = strtoull(p, NULL, 16);
                    break;
                }

                p++;
            }
        }

        if ( !m_uuid )
        {
            apr_generate_random_bytes( reinterpret_cast<unsigned char *>(&m_uuid), sizeof(m_uuid));

            //Set-Cookie: RMID=732423sdfs73242;expires=Fri, 31-Dec-2010 23:59:59 GMT;path=/;domain=.example.net
            //@see http://www.ietf.org/rfc/rfc2109.txt
            snprintf(buffer, BUFFER_SIZE, "SESSIONID=%llx;Max-Age=21600", m_uuid);
            apr_table_add(m_parent->m_request->headers_out, "Set-Cookie", buffer);
        }

        const char *dir = nullptr;
        apr_temp_dir_get( &dir, m_parent->m_pool );
        snprintf(buffer, BUFFER_SIZE, "%s/" SESSION_FILE_PREFIX "%llx", dir, m_uuid);

        m_fileName = strpooldup(buffer, m_parent->m_pool);

        apr_file_t *file = nullptr;
        if ( apr_file_open( &file, m_fileName, APR_READ | APR_WRITE | APR_CREATE | APR_BINARY | APR_BUFFERED,
                            APR_OS_DEFAULT, m_parent->m_pool ) == APR_SUCCESS )
        {
            apr_finfo_t fileInfo;
            apr_file_info_get( &fileInfo, APR_FINFO_SIZE, file );

            value_pairs values;
            value_pairs *v = &values;

            char *data = reinterpret_cast<char*>( apr_palloc(m_parent->m_pool, fileInfo.size + 1 ));
            char *end = data + fileInfo.size;
            *end = 0; // Make sure it ends in 0

            apr_size_t read = fileInfo.size;
            if (( apr_file_read( file, data, &read ) != APR_SUCCESS ) || ( read != (apr_size_t)fileInfo.size ))
            {
                ap_log_rerror(__FILE__, __LINE__, APLOG_ERR, OK, 0, m_parent->m_request, "Error reading cookie file: '%s'", m_fileName);
            }
            else
            {
                while( data < end )
                {
                    v->next = reinterpret_cast<value_pairs*>( apr_pcalloc( m_parent->m_pool, sizeof(value_pairs) ));
                    v = v->next;

                    v->first = data;

                    //Walk to the next value
                    do {
                        ++data;
                    }
                    while( *data && data < end );

                    if ( ++data < end )
                        v->second = data;

                    //Walk to the next value
                    do {
                        ++data;
                    }
                    while( *data && data < end );
                    ++data;
                }

                m_values = values.next;
            }

            apr_file_close( file );
        }
        else
            ap_log_rerror(__FILE__, __LINE__, APLOG_ERR, OK, 0, m_parent->m_request, "Error reading cookie file: '%s'", m_fileName);
#undef BUFFER_SIZE
    }

public:
    ApacheSession(ApacheHanlderIO *_parent):
        m_parent( _parent ),
        m_values( 0 ),
        m_uuid( 0 )
    {
        init();
    }

    virtual ~ApacheSession()
    {
        apr_file_t *file = nullptr;
        if ( apr_file_open( &file, m_fileName, APR_READ | APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY | APR_BUFFERED,
                            APR_OS_DEFAULT, m_parent->m_pool ) == APR_SUCCESS )
        {
            char ZERO = 0;

            apr_size_t wrote;

            value_pairs *v = m_values;
            while( v )
            {
#define CHECK_WRITE(FN) if ( (FN) != APR_SUCCESS ) break;

                wrote =  v->first.length();
                CHECK_WRITE( apr_file_write( file, const_cast<char*>( v->first.c_str() ), &wrote ) );

                wrote =  1;
                CHECK_WRITE( apr_file_write( file, &ZERO, &wrote ) );

                wrote = v->second.length();
                CHECK_WRITE( apr_file_write( file, const_cast<char*>( v->second.c_str() ), &wrote ) );

                wrote =  1;
                CHECK_WRITE( apr_file_write( file, &ZERO, &wrote ) );

#undef CHECK_WRITE

                v = v->next;
            }
            apr_file_close( file );
        }
    }

    virtual value_pairs* getValue(const char* _value = nullptr) override
    {
        if ( !m_values )
        {
            if ( !m_uuid )
                init();
        }

        if ( _value )
            return const_cast<value_pairs*>( m_values->findFirst( _value ) );
        else
            return m_values;
    }

    virtual void setValue(value_pairs* _value) override
    {
        if ( !m_values )
        {
            if ( !m_uuid )
                init();
        }

        value_pairs *src = _value;

        while( src )
        {
            value_pairs *v = const_cast<value_pairs*>( m_values->findFirst( src->first ) );

            if ( v )
                v->second = src->second;
            else
            {
                v = reinterpret_cast<value_pairs*>( apr_palloc( m_parent->m_pool, sizeof(value_pairs) ));
                if ( !m_values )
                    m_values = v;
                else
                    m_values->append( v );

                v->first = src->first;
                v->second = src->second;
            }

            src = src->next;
        }
    }

    virtual bool getValue(const char* _key, const char **_value) override
    {
        value_pairs* v = getValue(_key);
        if ( v && !v->second.empty() )
        {
            const std::size_t len = v->second.length();
            char* str = static_cast<char*>(m_parent->malloc(len + 1));
            memcpy(str, v->second.c_str(), len);
            str[len] = 0;
            *_value = str;
            return true;
        }
        return false;
    }

    virtual bool getValue(const char* _key, NCString &_value) override
    {
        value_pairs* v = getValue(_key);
        if ( v && !v->second.empty() )
        {
            _value = v->second;
            return true;
        }
        return false;
    }

    virtual bool getValue(const char* _key, int &_value) override
    {
        value_pairs* v = getValue(_key);
        if ( v && !v->second.empty() )
        {
            _value = atoi( v->second.c_str() );
            return true;
        }
        return false;
    }

    virtual void setValue(const char* _key, const char *_value) override
    {
        value_pairs v;
        v.first = strpooldup( _key, m_parent->m_pool );
        v.second = strpooldup( _value, m_parent->m_pool );
        setValue( &v );
    }

    virtual void setValue(const char* _key, const NCString& _value) override
    {
        value_pairs v;
        v.first = strpooldup( _key, m_parent->m_pool );
        v.second = strpooldup( _value.c_str(), m_parent->m_pool );
        setValue( &v );
    }

    virtual void setValue(const char* _key, int _value) override
    {
        char buffer[10];
        snprintf( buffer, 9, "%d", _value );
        buffer[9] = 0;

        value_pairs v;
        v.first = strpooldup( _key, m_parent->m_pool );
        v.second = strpooldup( buffer, m_parent->m_pool );
        setValue( &v );
    }
};

Session* ApacheHanlderIO::getSession()
{
    if ( !m_session )
    {
        m_session = new ApacheSession( this );
    }
    return m_session;
}

void ApacheHanlderIO::checkCleanup()
{
    if ( m_lastCheck < apr_time_now() + ( APR_USEC_PER_SEC * 60 ) )
    {
        m_lastCheck = apr_time_now();

        const char *path = nullptr;
        apr_temp_dir_get( &path, m_pool );

        char buffer[1024];
        strncpy( buffer, path, 1024 );
        int pathlen = strlen( path );
        buffer[pathlen++] = '/';

        apr_dir_t *dir = nullptr;
        apr_dir_open( &dir, path, m_pool );

        apr_time_t t = apr_time_now() - ( APR_USEC_PER_SEC * 60 * 60 * 6 ); //6hrs
        apr_finfo_t finfo;

        while ( apr_dir_read( &finfo, APR_FINFO_MTIME | APR_FINFO_NAME, dir ) != ENOENT )
        {
            if ( strncasecmp( SESSION_FILE_PREFIX, finfo.name, strlen(SESSION_FILE_PREFIX)) == 0
                    && std::max(finfo.mtime, finfo.ctime) < t )
            {
                strcpy( &buffer[pathlen], finfo.name );
                apr_file_remove( buffer, m_pool );

#ifndef NDEBUG
                ap_log_rerror(__FILE__, __LINE__, APLOG_WARNING, OK, 0, m_request, "Removing cookie file: '%s'", buffer);
#endif
            }
        }

        apr_dir_close( dir );
    }
}

}
