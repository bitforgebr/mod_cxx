/*
 * ApacheHandler.h
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#ifndef __INCLUDE_MOD_CXX_APACHEHANDLER_H
#define __INCLUDE_MOD_CXX_APACHEHANDLER_H

#include <setjmp.h>

#include "handlers.h"
#include "globals_private.h"

namespace bitforge
{

class ApacheHanlderIO : public HandlerIO
{
    friend int readTable(void* _p, const char* _key, const char* _value);
    friend class ApacheSession;

private:
    bool                m_isOpen = false;
    value_pairs*        m_tmpList = nullptr;

    value_pairs*        m_getVars = nullptr;
    value_pairs*        m_putVars = nullptr;

protected:
    request_rec*        m_request = nullptr;
    apr_pool_t*         m_pool = nullptr;

    const char*         m_appsDir = nullptr;
    const value_pairs*  m_databaseConfig = nullptr;
    const value_pairs*  m_protocolConfig = nullptr;

    apr_bucket_brigade* m_inbb = nullptr;
    apr_bucket*         m_inBucket = nullptr;

    apr_bucket_brigade* m_outbb = nullptr;

    Session*            m_session = nullptr;

    inline bool checkInput(std::size_t& _length);

    //Cleanup session files on disk
    static apr_time_t m_lastCheck;
    void checkCleanup();

public:
    //Special var for aborting a request
    jmp_buf err_env;

    ApacheHanlderIO();
    virtual ~ApacheHanlderIO();

    virtual bool open(const char* contentType = nullptr) override;
    virtual bool close() override;

    virtual std::size_t read(const char** _data, std::size_t _length = 0) override;
    virtual std::size_t read(NCString& _data) override
    {
        const char* buffer = nullptr;
        std::size_t result = read(&buffer);
        _data.assign(buffer, result);
        return result;
    }

    virtual std::size_t write(const void* _data, std::size_t _length) override;
    virtual void write(const char* _data) override;
    virtual void write(const NCString& _data) override
    {
        write(_data.data(), _data.length());
    }

    virtual value_pairs* getHeaders(const char* _header = nullptr) override;

    virtual const char* getURI() override
    {
        return m_request->uri;
    }

    virtual const char* getURIPath() override
    {
        return m_request->path_info;
    }

    virtual const char* getURIArgs() override
    {
        return m_request->args;
    }

    virtual const char* getHost() override
    {
        return m_request->hostname;
    }

    virtual const value_pairs* getGET() override;
    virtual bool getGET(const char* _key, const char** _value) override
    {
        const value_pairs* v = getGET();

        if(v && (v = v->findFirst(_key)))
        {
            const std::size_t len = v->second.length();
            char* str = static_cast<char*>(this->malloc(len + 1));
            memcpy(str, v->second.c_str(), len);
            str[len] = 0;
            *_value = str;
            return true;
        }

        return false;
    }

    virtual bool getGET(const char* _key, NCString& _value) override
    {
        const value_pairs* v = getGET();

        if(v && (v = v->findFirst(_key)))
        {
            _value = v->second;
            return true;
        }

        return false;
    }

    virtual const value_pairs* getPUT() override;
    virtual bool getPUT(const char* _key, const char** _value) override
    {
        const value_pairs* v = getPUT();

        if(v && (v = v->findFirst(_key)))
        {
            const std::size_t len = v->second.length();
            char* str = static_cast<char*>(this->malloc(len + 1));
            memcpy(str, v->second.c_str(), len);
            str[len] = 0;
            *_value = str;
            return true;
        }

        return false;
    }

    virtual bool getPUT(const char* _key, NCString& _value) override
    {
        const value_pairs* v = getPUT();

        if(v && (v = v->findFirst(_key)))
        {
            _value = v->second;
            return true;
        }

        return false;
    }

    virtual const char* getAppsDir() override
    {
        return m_appsDir;
    }

    virtual const value_pairs* getDatabseConfig() override
    {
        return m_databaseConfig;
    }

    virtual const value_pairs* getProtocolConfig() override
    {
        return m_protocolConfig;
    }

    virtual Session* getSession() override;

    void setRequest(request_rec* _value)
    {
        m_request = _value;
        m_pool = m_request->pool;
    }

    void setDatabaseConfig(const value_pairs* _value)
    {
        m_databaseConfig = _value;
    }

    void setProtocolConfig(const value_pairs* _value)
    {
        m_protocolConfig = _value;
    }

    void setAppsDir(const char* _value)
    {
        m_appsDir = _value;
    }

    apr_pool_t* getRequestPool()
    {
        return m_request->pool;
    }

    apr_pool_t* getConnectionPool()
    {
        return m_request->connection->pool;
    }

    apr_pool_t* getProcessPool()
    {
        return m_request->server->process->pool;
    }


    virtual void* malloc(size_t size) override;
    virtual void free(void* ptr) override;
    virtual void* malloc(std::size_t size, AllocScope scope, Deleter d) override;


    virtual void logError(ErrorLevel _level, const char* _message) override;
    virtual void abort(const char* _message = nullptr) override;
};

}

#endif // __INCLUDE_MOD_CXX_APACHEHANDLER_H
