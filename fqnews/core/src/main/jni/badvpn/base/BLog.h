/**
 * @file BLog.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * A global object for logging.
 */

#ifndef BADVPN_BLOG_H
#define BADVPN_BLOG_H

#include <stdarg.h>
#include <string.h>

#include <misc/debug.h>
#include <misc/memref.h>
#include <base/BMutex.h>

// auto-generated channel numbers and number of channels
#include <generated/blog_channels_defines.h>

// keep in sync with level names in BLog.c!
#define BLOG_ERROR 1
#define BLOG_WARNING 2
#define BLOG_NOTICE 3
#define BLOG_INFO 4
#define BLOG_DEBUG 5

#define BLog(...) BLog_LogToChannel(BLOG_CURRENT_CHANNEL, __VA_ARGS__)
#define BContextLog(context, ...) BLog_ContextLog((context), BLOG_CURRENT_CHANNEL, __VA_ARGS__)
#define BLOG_CCCC(context) BLog_MakeChannelContext((context), BLOG_CURRENT_CHANNEL)

typedef void (*_BLog_log_func) (int channel, int level, const char *msg);
typedef void (*_BLog_free_func) (void);

struct _BLog_channel {
    const char *name;
    int loglevel;
};

struct _BLog_global {
    #ifndef NDEBUG
    int initialized; // initialized statically
    #endif
    struct _BLog_channel channels[BLOG_NUM_CHANNELS];
    _BLog_log_func log_func;
    _BLog_free_func free_func;
    BMutex mutex;
#ifndef NDEBUG
    int logging;
#endif
    char logbuf[2048];
    int logbuf_pos;
};

extern struct _BLog_channel blog_channel_list[];
extern struct _BLog_global blog_global;

typedef void (*BLog_logfunc) (void *);

typedef struct {
    BLog_logfunc logfunc;
    void *logfunc_user;
} BLogContext;

typedef struct {
    BLogContext context;
    int channel;
} BLogChannelContext;

static int BLogGlobal_GetChannelByName (const char *channel_name);

static void BLog_Init (_BLog_log_func log_func, _BLog_free_func free_func);
static void BLog_Free (void);
static void BLog_SetChannelLoglevel (int channel, int loglevel);
static int BLog_WouldLog (int channel, int level);
static void BLog_Begin (void);
static void BLog_AppendVarArg (const char *fmt, va_list vl);
static void BLog_Append (const char *fmt, ...);
static void BLog_AppendBytes (MemRef data);
static void BLog_Finish (int channel, int level);
static void BLog_LogToChannelVarArg (int channel, int level, const char *fmt, va_list vl);
static void BLog_LogToChannel (int channel, int level, const char *fmt, ...);
static void BLog_LogViaFuncVarArg (BLog_logfunc func, void *arg, int channel, int level, const char *fmt, va_list vl);
static void BLog_LogViaFunc (BLog_logfunc func, void *arg, int channel, int level, const char *fmt, ...);
static BLogContext BLog_RootContext (void);
static BLogContext BLog_MakeContext (BLog_logfunc logfunc, void *logfunc_user);
static void BLog_ContextLogVarArg (BLogContext context, int channel, int level, const char *fmt, va_list vl);
static void BLog_ContextLog (BLogContext context, int channel, int level, const char *fmt, ...);
static BLogChannelContext BLog_MakeChannelContext (BLogContext context, int channel);
static void BLog_ChannelContextLogVarArg (BLogChannelContext ccontext, int level, const char *fmt, va_list vl);
static void BLog_ChannelContextLog (BLogChannelContext ccontext, int level, const char *fmt, ...);

void BLog_InitStdout (void);
void BLog_InitStderr (void);

int BLogGlobal_GetChannelByName (const char *channel_name)
{
    int i;
    for (i = 0; i < BLOG_NUM_CHANNELS; i++) {
        if (!strcmp(blog_channel_list[i].name, channel_name)) {
            return i;
        }
    }
    
    return -1;
}

void BLog_Init (_BLog_log_func log_func, _BLog_free_func free_func)
{
    ASSERT(!blog_global.initialized)
    
    #ifndef NDEBUG
    blog_global.initialized = 1;
    #endif
    
    // initialize channels
    memcpy(blog_global.channels, blog_channel_list, BLOG_NUM_CHANNELS * sizeof(struct _BLog_channel));
    
    blog_global.log_func = log_func;
    blog_global.free_func = free_func;
#ifndef NDEBUG
    blog_global.logging = 0;
#endif
    blog_global.logbuf_pos = 0;
    blog_global.logbuf[0] = '\0';
    
    ASSERT_FORCE(BMutex_Init(&blog_global.mutex))
}

void BLog_Free (void)
{
    ASSERT(blog_global.initialized)
#ifndef NDEBUG
    ASSERT(!blog_global.logging)
#endif
    
    BMutex_Free(&blog_global.mutex);
    
    #ifndef NDEBUG
    blog_global.initialized = 0;
    #endif
    
    blog_global.free_func();
}

void BLog_SetChannelLoglevel (int channel, int loglevel)
{
    ASSERT(blog_global.initialized)
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(loglevel >= 0 && loglevel <= BLOG_DEBUG)
    
    blog_global.channels[channel].loglevel = loglevel;
}

int BLog_WouldLog (int channel, int level)
{
    ASSERT(blog_global.initialized)
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(level >= BLOG_ERROR && level <= BLOG_DEBUG)
    
    return (level <= blog_global.channels[channel].loglevel);
}

void BLog_Begin (void)
{
    ASSERT(blog_global.initialized)
    
    BMutex_Lock(&blog_global.mutex);
    
#ifndef NDEBUG
    ASSERT(!blog_global.logging)
    blog_global.logging = 1;
#endif
}

void BLog_AppendVarArg (const char *fmt, va_list vl)
{
    ASSERT(blog_global.initialized)
#ifndef NDEBUG
    ASSERT(blog_global.logging)
#endif
    ASSERT(blog_global.logbuf_pos >= 0)
    ASSERT(blog_global.logbuf_pos < sizeof(blog_global.logbuf))
    
    int w = vsnprintf(blog_global.logbuf + blog_global.logbuf_pos, sizeof(blog_global.logbuf) - blog_global.logbuf_pos, fmt, vl);
    
    if (w >= sizeof(blog_global.logbuf) - blog_global.logbuf_pos) {
        blog_global.logbuf_pos = sizeof(blog_global.logbuf) - 1;
    } else {
        blog_global.logbuf_pos += w;
    }
}

void BLog_Append (const char *fmt, ...)
{
    ASSERT(blog_global.initialized)
#ifndef NDEBUG
    ASSERT(blog_global.logging)
#endif
    
    va_list vl;
    va_start(vl, fmt);
    BLog_AppendVarArg(fmt, vl);
    va_end(vl);
}

void BLog_AppendBytes (MemRef data)
{
    ASSERT(blog_global.initialized)
#ifndef NDEBUG
    ASSERT(blog_global.logging)
#endif
    ASSERT(blog_global.logbuf_pos >= 0)
    ASSERT(blog_global.logbuf_pos < sizeof(blog_global.logbuf))
    
    size_t avail = (sizeof(blog_global.logbuf) - 1) - blog_global.logbuf_pos;
    data.len = (data.len > avail ? avail : data.len);
    
    memcpy(blog_global.logbuf + blog_global.logbuf_pos, data.ptr, data.len);
    blog_global.logbuf_pos += data.len;
    blog_global.logbuf[blog_global.logbuf_pos] = '\0';
}

void BLog_Finish (int channel, int level)
{
    ASSERT(blog_global.initialized)
#ifndef NDEBUG
    ASSERT(blog_global.logging)
#endif
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(level >= BLOG_ERROR && level <= BLOG_DEBUG)
    ASSERT(BLog_WouldLog(channel, level))
    
    ASSERT(blog_global.logbuf_pos >= 0)
    ASSERT(blog_global.logbuf_pos < sizeof(blog_global.logbuf))
    ASSERT(blog_global.logbuf[blog_global.logbuf_pos] == '\0')
    
    blog_global.log_func(channel, level, blog_global.logbuf);
    
#ifndef NDEBUG
    blog_global.logging = 0;
#endif
    blog_global.logbuf_pos = 0;
    blog_global.logbuf[0] = '\0';
    
    BMutex_Unlock(&blog_global.mutex);
}

void BLog_LogToChannelVarArg (int channel, int level, const char *fmt, va_list vl)
{
    ASSERT(blog_global.initialized)
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(level >= BLOG_ERROR && level <= BLOG_DEBUG)
    
    if (!BLog_WouldLog(channel, level)) {
        return;
    }
    
    BLog_Begin();
    BLog_AppendVarArg(fmt, vl);
    BLog_Finish(channel, level);
}

void BLog_LogToChannel (int channel, int level, const char *fmt, ...)
{
    ASSERT(blog_global.initialized)
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(level >= BLOG_ERROR && level <= BLOG_DEBUG)
    
    if (!BLog_WouldLog(channel, level)) {
        return;
    }
    
    va_list vl;
    va_start(vl, fmt);
    
    BLog_Begin();
    BLog_AppendVarArg(fmt, vl);
    BLog_Finish(channel, level);
    
    va_end(vl);
}

void BLog_LogViaFuncVarArg (BLog_logfunc func, void *arg, int channel, int level, const char *fmt, va_list vl)
{
    ASSERT(blog_global.initialized)
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(level >= BLOG_ERROR && level <= BLOG_DEBUG)
    
    if (!BLog_WouldLog(channel, level)) {
        return;
    }
    
    BLog_Begin();
    func(arg);
    BLog_AppendVarArg(fmt, vl);
    BLog_Finish(channel, level);
}

void BLog_LogViaFunc (BLog_logfunc func, void *arg, int channel, int level, const char *fmt, ...)
{
    ASSERT(blog_global.initialized)
    ASSERT(channel >= 0 && channel < BLOG_NUM_CHANNELS)
    ASSERT(level >= BLOG_ERROR && level <= BLOG_DEBUG)
    
    if (!BLog_WouldLog(channel, level)) {
        return;
    }
    
    va_list vl;
    va_start(vl, fmt);
    
    BLog_Begin();
    func(arg);
    BLog_AppendVarArg(fmt, vl);
    BLog_Finish(channel, level);
    
    va_end(vl);
}

static void BLog__root_logfunc (void *unused)
{
}

static BLogContext BLog_RootContext (void)
{
    return BLog_MakeContext(BLog__root_logfunc, NULL);
}

static BLogContext BLog_MakeContext (BLog_logfunc logfunc, void *logfunc_user)
{
    ASSERT(logfunc)
    
    BLogContext context;
    context.logfunc = logfunc;
    context.logfunc_user = logfunc_user;
    return context;
}

static void BLog_ContextLogVarArg (BLogContext context, int channel, int level, const char *fmt, va_list vl)
{
    BLog_LogViaFuncVarArg(context.logfunc, context.logfunc_user, channel, level, fmt, vl);
}

static void BLog_ContextLog (BLogContext context, int channel, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    BLog_ContextLogVarArg(context, channel, level, fmt, vl);
    va_end(vl);
}

static BLogChannelContext BLog_MakeChannelContext (BLogContext context, int channel)
{
    BLogChannelContext ccontext;
    ccontext.context = context;
    ccontext.channel = channel;
    return ccontext;
}

static void BLog_ChannelContextLogVarArg (BLogChannelContext ccontext, int level, const char *fmt, va_list vl)
{
    BLog_ContextLogVarArg(ccontext.context, ccontext.channel, level, fmt, vl);
}

static void BLog_ChannelContextLog (BLogChannelContext ccontext, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    BLog_ChannelContextLogVarArg(ccontext, level, fmt, vl);
    va_end(vl);
}

#endif
