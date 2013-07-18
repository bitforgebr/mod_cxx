/*
* handlers.h
*
*  Created on: Sep 22, 2012
*      Author: gianni
*
* BitForge http://www.bitforge.com.br
* Copyright (c) 2012 All Right Reserved,
*/

#ifndef __INCLUDE_MOD_CXX_HANDLERS_H
#define __INCLUDE_MOD_CXX_HANDLERS_H

#include "globals.h"

#include <functional>

namespace bitforge
{

class Session;

class HandlerIO
{
public:
    typedef std::function<void(void*)> Deleter;

    virtual bool open(const char* contentType = nullptr) = 0;
    virtual bool close() = 0;

    virtual std::size_t read(const char **_data, std::size_t _length = 0) = 0;
    virtual std::size_t read(NCString& _data) = 0;

    virtual std::size_t write(const void *_data, std::size_t _length) = 0;
    virtual void write(const char *_data) = 0;
    virtual void write(const NCString& _data) = 0;

    virtual value_pairs* getHeaders(const char* _header = 0) = 0;

    virtual const char* getURI() = 0;
    virtual const char* getURIPath() = 0;
    virtual const char* getURIArgs() = 0;

    virtual const char* getHost() = 0;

    virtual const value_pairs* getGET() = 0;
    virtual bool getGET(const char *_key, const char **_value) = 0;
    virtual bool getGET(const char *_key, NCString &_value) = 0;

    virtual const value_pairs* getPUT() = 0;
    virtual bool getPUT(const char *_key, const char **_value) = 0;
    virtual bool getPUT(const char *_key, NCString &_value) = 0;

    virtual const char* getAppsDir() = 0;

    virtual const value_pairs * getDatabseConfig() = 0;
    virtual const value_pairs * getProtocolConfig() = 0;

    virtual Session* getSession() = 0;

    virtual void *malloc(std::size_t size) = 0;
    virtual void free(void *ptr) = 0;

    // Determine when the deleter should be called
    enum AllocScope
    {
        asServer,
        asConfig,
        asConection,
        asRequest
    };
    virtual void *malloc(std::size_t size, AllocScope scope, Deleter d) = 0;

    template<typename T, typename... Arguments>
    T* malloc_object(AllocScope scope, Arguments... params)
    {
        void* result = this->malloc(sizeof(T), scope, [](void* ptr){
            T* obj = static_cast<T*>(ptr);
            obj->~T();
        });
        return new(result) T(params...);
    }

    enum ErrorLevel
    {
        elStatus = 0,
        elWarning,
        elError,
        elFatal
    };
    virtual void logError(ErrorLevel _level, const char* _message) = 0;
    virtual void abort(const char* _message = nullptr) = 0;

    void logError(ErrorLevel _level, const std::string& _message);
};

inline void HandlerIO::logError (HandlerIO::ErrorLevel _level, const std::string& _message)
{
    logError(_level, _message.c_str());
}

typedef void (*handle_request_t)(HandlerIO* _pluginIO);

class Session
{
public:
    virtual ~Session() {};

    virtual value_pairs* getValue(const char* _value = nullptr) = 0;
    virtual bool getValue(const char* _key, const char **_value) = 0;
    virtual bool getValue(const char* _key, NCString& _value) = 0;
    virtual bool getValue(const char* _key, int &_value) = 0;

    virtual void setValue(value_pairs* _value) = 0;
    virtual void setValue(const char* _key, const char *_value) = 0;
    virtual void setValue(const char* _key, const NCString& _value) = 0;
    virtual void setValue(const char* _key, int _value) = 0;
};

}

#endif //__INCLUDE_MOD_CXX_HANDLERS_H
