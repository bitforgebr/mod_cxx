/*
 * globals.h
 *
 *  Created on: Sep 22, 2012
 *      Author: gianni
 *
 * BitForge http://www.bitforge.com.br
 * Copyright (c) 2012 All Right Reserved,
 */

#ifndef __INCLUDE_MOD_CXX_GLOBALS_H
#define __INCLUDE_MOD_CXX_GLOBALS_H

#include <bf/ncstring.h>

namespace bitforge
{

struct value_pairs
{
    NCString first;
    NCString second;

    value_pairs* next = nullptr;

    template<typename T>
    const value_pairs* findFirst(T _val) const
    {
        const value_pairs* result = this;
        while(result && (result->first.compare(_val) != 0))
            result = result->next;

        return result;
    }

    template<typename T>
    const value_pairs* findSecond(T _val) const
    {
        const value_pairs* result = this;
        while (result && (result->second.compare(_val) != 0))
            result = result->next;

        return result;
    }

    void append(value_pairs* _value)
    {
        value_pairs* p = this;
        while ( p->next )
            p = p->next;
        p->next = _value;
    }
};

struct char_list
{
    const char* value = nullptr;
    char_list* next = nullptr;

    const char_list* findFirst(const char *_val) const
    {
        const char_list* result = this;
        while (result && (strcasecmp(value, _val) != 0))
            result = result->next;

        return result;
    }
};


#ifdef HIDDEN_VISIBILITY
#define VISIBILITY __attribute__ ((visibility("default")))
#else
#define VISIBILITY
#endif

#define EXPORT extern "C" VISIBILITY

}

#endif // __INCLUDE_MOD_CXX_GLOBALS_H
