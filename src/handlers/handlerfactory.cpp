/*
 * handlerfactory.cpp
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#include "handlerfactory.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(cxx);
#endif

#include <http_log.h>

#include <dlfcn.h>
#include <unistd.h>

#include <sstream>

namespace bitforge
{

#define DEBUGSTR(...) ap_log_rerror( __FILE__, __LINE__, APLOG_WARNING, OK, _request, __VA_ARGS__ );

#define check_error(int) if ( int < 0 ) report_error(_request, int, __FILE__, __LINE__)

void report_error ( request_rec *_request, int _ret, const char* _file, const int _line )
{
    ap_log_rerror ( _file, _line, APLOG_CRIT, OK, _ret, _request, "%s:%d error(%d): %s", _file, _line, _ret, strerror ( errno ) );
    abort();
}

struct timeval time_to_wait = {0, 0};

HandlerCache::HandlerCache()
{
}

HandlerCache::~HandlerCache()
{
    dlclose ( lib );
}

HandlerFactory::HandlerFactory ( request_rec *_request, apr_pool_t *_pool, const char* _handlerDirectory ) :
    m_pool ( _pool )
{
    m_handlerDirectory = strpooldup ( _handlerDirectory, strlen ( _handlerDirectory ), m_pool );

#if __linux__ && defined(USE_AIO)
    //Initialize inotify
    m_notify = inotify_init();
    check_error ( m_notify );

    memset ( &aiowatch, 0, sizeof ( aiowatch ) );

    aiowatch.aio_buf = aiobuffer;
    aiowatch.aio_nbytes = BUF_LEN;
    aiowatch.aio_fildes = m_notify;

    //Start monitoring
    int ret = aio_read ( &aiowatch );
    check_error ( ret );
#endif
}

HandlerFactory::~HandlerFactory()
{
#if __linux__ && defined(USE_AIO)
    close ( m_notify );
#endif
}

HandlerCache *HandlerFactory::getHandler ( request_rec *_request, bitforge::directory_config *_directoryConfig )
{
    uint32_t id = _directoryConfig->getID ();

#if __linux__ && defined(USE_AIO)
    //Check the watches to see if anything changed and needs to be reloaded
    int ret = aio_error ( &aiowatch );

    if ( ret == 0 )
    {
        //This doesn't work very well, so for now, just clear the whole cache.

//         struct inotify_event *event;
//
//         int i = 0;
//         int length = aio_return(&aiowatch);
//
//         while (i < length)
//         {
//             event = reinterpret_cast<struct inotify_event *>(&aiobuffer[i]);
//
//             PluginCacheList::iterator it = m_pluginCache.begin();
//
//             for (; it != m_pluginCache.end(); ++it)
//             {
//                 if (it->second->watch = event->wd)
//                 {
//                     delete it->second;
//                     m_pluginCache.erase(it);
//                     goto FOUND_PLUGIN;
//                 }
//             }
//
//         FOUND_PLUGIN:
//             inotify_rm_watch(m_notify, event->wd);
//             i += EVENT_SIZE + event->len;
//         }

        auto it = m_handlerCache.begin();
        for ( ; it != m_handlerCache.end(); ++it )
        {
            inotify_rm_watch ( m_notify, it->second->watch );
            delete it->second;
        }
        m_handlerCache.clear();


        //Restart monitoring
        ret = aio_read ( &aiowatch );
        check_error ( ret );
    }
    else if ( ret != EINPROGRESS )
    {
        report_error ( _request, ret, __FILE__, __LINE__ );
    }
    //done check
#endif

    auto it = m_handlerCache.find ( id );
    if ( it != m_handlerCache.end() )
    {
        return it->second;
    }

    std::stringstream str;

    if ( m_handlerDirectory )
        str << m_handlerDirectory << '/';

    str << _directoryConfig->getHandlerName( _request->pool );

    void *lib = dlopen( str.str().c_str(), RTLD_LAZY );
    if ( !lib )
    {
        ap_log_rerror( __FILE__, __LINE__, APLOG_ERR, OK, errno, _request, "Error loading library: '%s' reason '%s'", str.str().c_str(), dlerror() );
        return 0;
    }

    handle_request_t handlerfn = reinterpret_cast<handle_request_t> ( dlsym ( lib, "handle_request" ) );
    if ( !handlerfn )
    {
        dlclose ( lib );
        ap_log_rerror ( __FILE__, __LINE__, APLOG_ERR, OK, errno, _request, "Error finding symbol: '%s:handle_request' reason '%s'", str.str().c_str(), dlerror() );
        return 0;
    }

    HandlerCache *cache = new HandlerCache;

    cache->lib = lib;
    cache->handlerfn = handlerfn;

#if __linux__ && defined(USE_AIO)
    cache->watch = inotify_add_watch ( m_notify, str.str().c_str(),
                                       IN_ATTRIB | IN_MOVE | IN_MODIFY | IN_CREATE | IN_DELETE_SELF
                                     );
    check_error ( cache->watch );

    m_handlerCache[id] = cache;

    //Cancel any outstanding requests
    aio_cancel ( m_notify, &aiowatch );
    //ignore any error or return status - it really doesn't matter

    //Start monitoring
    ret = aio_read ( &aiowatch );
    check_error ( ret );
#endif

    ap_log_rerror ( __FILE__, __LINE__, APLOG_NOTICE, OK, errno, _request, "Loaded plugin: '%s'", str.str().c_str() );

    return cache;
}

}
// kate: indent-mode cstyle; replace-tabs off; tab-width 4;
