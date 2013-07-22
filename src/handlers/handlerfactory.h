/*
 * handlerfactory.h
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#ifndef __INCLUDE_MOD_CXX_HANDLERFACTORY_H
#define __INCLUDE_MOD_CXX_HANDLERFACTORY_H

#include <map>

#if __linux__ && defined(USE_AIO)
#include <sys/inotify.h>
#include <aio.h>
#endif

#include "config.h"
#include "../handlers.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

namespace bitforge
{

class HandlerCache
{
    friend class HandlerFactory;

private:
    HandlerCache();
    ~HandlerCache();

#if __linux__ && defined(USE_AIO)
    int watch = 0;
#endif

    void *lib = nullptr;
public:
    handle_request_t handlerfn = nullptr;
};
typedef std::map<uint32_t, HandlerCache *> HandlerCacheList;

class HandlerFactory
{
protected:
    HandlerCacheList m_handlerCache;
    apr_pool_t *m_pool;
    const char *m_handlerDirectory;

#if __linux__ && defined(USE_AIO)
    int m_notify = 0;

    struct aiocb aiowatch;
    char aiobuffer[BUF_LEN];
#endif

public:
    HandlerFactory(request_rec *_request, apr_pool_t *_pool, const char *_handlerDirectory);
    ~HandlerFactory();

    HandlerCache *getHandler(request_rec *_request, directory_config *_directoryConfig);
};

}

#endif // __INCLUDE_MOD_CXX_HANDLERFACTORY_H
