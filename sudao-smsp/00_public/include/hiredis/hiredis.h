/*
 * Copyright (c) 2009-2011, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2011, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __HIREDIS_H
#define __HIREDIS_H
#include <stdio.h> /* for size_t */
#include <stdarg.h> /* for va_list */
#include <sys/time.h> /* for struct timeval */
#include <sys/types.h>
#include "ae.h"

#define HIREDIS_MAJOR 0
#define HIREDIS_MINOR 11
#define HIREDIS_PATCH 0

#define REDIS_ERR -1
#define REDIS_OK 0

/* When an error occurs, the err flag in a context is set to hold the type of
 * error that occured. REDIS_ERR_IO means there was an I/O error and you
 * should use the "errno" variable to find out what is wrong.
 * For other values, the "errstr" field will hold a description. */
#define REDIS_ERR_IO 1 /* Error in read or write */
#define REDIS_ERR_EOF 3 /* End of file */
#define REDIS_ERR_PROTOCOL 4 /* Protocol error */
#define REDIS_ERR_OOM 5 /* Out of memory */
#define REDIS_ERR_OTHER 2 /* Everything else... */

/* Connection type can be blocking or non-blocking and is set in the
 * least significant bit of the flags field in redisContext. */
#define REDIS_BLOCK 0x1

/* Connection may be disconnected before being free'd. The second bit
 * in the flags field is set when the context is connected. */
#define REDIS_CONNECTED 0x2

/* The async API might try to disconnect cleanly and flush the output
 * buffer and read all subsequent replies before disconnecting.
 * This flag means no new commands can come in and the connection
 * should be terminated once all replies have been read. */
#define REDIS_DISCONNECTING 0x4

/* Flag specific to the async API which means that the context should be clean
 * up as soon as possible. */
#define REDIS_FREEING 0x8

/* Flag that is set when an async callback is executed. */
#define REDIS_IN_CALLBACK 0x10

/* Flag that is set when the async context has one or more subscriptions. */
#define REDIS_SUBSCRIBED 0x20

/* Flag that is set when monitor mode is active */
#define REDIS_MONITORING 0x40

#define REDIS_REPLY_STRING 1
#define REDIS_REPLY_ARRAY 2
#define REDIS_REPLY_INTEGER 3
#define REDIS_REPLY_NIL 4
#define REDIS_REPLY_STATUS 5
#define REDIS_REPLY_ERROR 6

#define REDIS_READER_MAX_BUF (1024*16)  /* Default max unused reader buffer. */

#define REDIS_KEEPALIVE_INTERVAL 15 /* seconds */

#ifdef __cplusplus
extern "C" {
#endif

/* This is the reply object returned by redisCommand() */
typedef struct redisReply
{
    int type; /* REDIS_REPLY_* */
    long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
    int len; /* Length of string */
    char *str; /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
    size_t elements; /* number of elements, for REDIS_REPLY_ARRAY */
    struct redisReply **element; /* elements vector for REDIS_REPLY_ARRAY */
} redisReply;

typedef struct redisReadTask
{
    int type;
    int elements; /* number of elements in multibulk container */
    int idx; /* index in parent (array) object */
    void *obj; /* holds user-generated value for a read task */
    struct redisReadTask *parent; /* parent task */
    void *privdata; /* user-settable arbitrary field */
} redisReadTask;

typedef struct redisReplyObjectFunctions
{
    void *(*createString)(const redisReadTask*, char*, size_t);
    void *(*createArray)(const redisReadTask*, int);
    void *(*createInteger)(const redisReadTask*, long long);
    void *(*createNil)(const redisReadTask*);
    void (*freeObject)(void*);
} redisReplyObjectFunctions;

/* State for the protocol parser */
typedef struct redisReader
{
    int err; /* Error flags, 0 when there is no error */
    char errstr[128]; /* String representation of error when applicable */

    char *buf; /* Read buffer */
    size_t pos; /* Buffer cursor */
    size_t len; /* Buffer length */
    size_t maxbuf; /* Max length of unused buffer */

    redisReadTask rstack[9];
    int ridx; /* Index of current read task */
    void *reply; /* Temporary reply pointer */

    redisReplyObjectFunctions *fn;
    void *privdata;
} redisReader;

/* Public API for the protocol parser. */
redisReader *redisReaderCreate(void);
void redisReaderFree(redisReader *r);
int redisReaderFeed(redisReader *r, const char *buf, size_t len);
int redisReaderGetReply(redisReader *r, void **reply);

/* Backwards compatibility, can be removed on big version bump. */
#define redisReplyReaderCreate redisReaderCreate
#define redisReplyReaderFree redisReaderFree
#define redisReplyReaderFeed redisReaderFeed
#define redisReplyReaderGetReply redisReaderGetReply
#define redisReplyReaderSetPrivdata(_r, _p) (int)(((redisReader*)(_r))->privdata = (_p))
#define redisReplyReaderGetObject(_r) (((redisReader*)(_r))->reply)
#define redisReplyReaderGetError(_r) (((redisReader*)(_r))->errstr)

/* Function to free the reply objects hiredis returns by default. */
void freeReplyObject(void *reply);

/* Functions to format a command according to the protocol. */
int redisvFormatCommand(char **target, const char *format, va_list ap);
int redisFormatCommand(char **target, const char *format, ...);
int redisFormatCommandArgv(char **target, int argc, const char **argv, const size_t *argvlen);

/* Context for a connection to Redis */
typedef struct redisContext
{
    int err; /* Error flags, 0 when there is no error */
    char errstr[128]; /* String representation of error when applicable */
    int fd;
    int flags;
    char *obuf; /* Write buffer */
    redisReader *reader; /* Protocol reader */
} redisContext;

redisContext *redisConnect(const char *ip, int port);
redisContext *redisConnectWithTimeout(const char *ip, int port, const struct timeval tv);
redisContext *redisConnectNonBlock(const char *ip, int port);
redisContext *redisConnectBindNonBlock(const char *ip, int port, const char *source_addr);
redisContext *redisConnectUnix(const char *path);
redisContext *redisConnectUnixWithTimeout(const char *path, const struct timeval tv);
redisContext *redisConnectUnixNonBlock(const char *path);
redisContext *redisConnectFd(int fd);
int redisSetTimeout(redisContext *c, const struct timeval tv);
int redisEnableKeepAlive(redisContext *c);
void redisFree(redisContext *c);
int redisFreeKeepFd(redisContext *c);
int redisBufferRead(redisContext *c);
int redisBufferWrite(redisContext *c, int *done);

/* In a blocking context, this function first checks if there are unconsumed
 * replies to return and returns one if so. Otherwise, it flushes the output
 * buffer to the socket and reads until it has a reply. In a non-blocking
 * context, it will return unconsumed replies until there are no more. */
int redisGetReply(redisContext *c, void **reply);
int redisGetReplyFromReader(redisContext *c, void **reply);

/* Write a formatted command to the output buffer. Use these functions in blocking mode
 * to get a pipeline of commands. */
int redisAppendFormattedCommand(redisContext *c, const char *cmd, size_t len);

/* Write a command to the output buffer. Use these functions in blocking mode
 * to get a pipeline of commands. */
int redisvAppendCommand(redisContext *c, const char *format, va_list ap);
int redisAppendCommand(redisContext *c, const char *format, ...);
int redisAppendCommandArgv(redisContext *c, int argc, const char **argv, const size_t *argvlen);

/* Issue a command to Redis. In a blocking context, it is identical to calling
 * redisAppendCommand, followed by redisGetReply. The function will return
 * NULL if there was an error in performing the request, otherwise it will
 * return the reply. In a non-blocking context, it is identical to calling
 * only redisAppendCommand and will always return NULL. */
void *redisvCommand(redisContext *c, const char *format, va_list ap);
void *redisCommand(redisContext *c, const char *format, ...);
void *redisCommandArgv(redisContext *c, int argc, const char **argv, const size_t *argvlen);

#ifdef __cplusplus
}
#endif

/* BEGIN: Added by fuxunhao, 2015/10/15   PN:async */
#ifdef __cplusplus
extern "C" {
#endif

struct redisAsyncContext; /* need forward declaration of redisAsyncContext */
struct dict; /* dictionary header is included in async.c */

/* Reply callback prototype and container */
typedef void (redisCallbackFn)(struct redisAsyncContext*, void*, void*);
typedef struct redisCallback
{
    struct redisCallback *next; /* simple singly linked list */
    redisCallbackFn *fn;
    void *privdata;
} redisCallback;

/* List of callbacks for either regular replies or pub/sub */
typedef struct redisCallbackList
{
    redisCallback *head, *tail;
} redisCallbackList;

/* Connection callback prototypes */
typedef void (redisDisconnectCallback)(const struct redisAsyncContext*, int status);
typedef void (redisConnectCallback)(const struct redisAsyncContext*, int status);

/* Context for an async connection to Redis */
typedef struct redisAsyncContext
{
    /* Hold the regular context, so it can be realloc'ed. */
    redisContext c;

    /* Setup error flags so they can be used directly. */
    int err;
    char *errstr;

    /* Not used by hiredis */
    void *data;

    /* Event library data and hooks */
    struct
    {
        void *data;

        /* Hooks that are called when the library expects to start
         * reading/writing. These functions should be idempotent. */
        void (*addRead)(void *privdata);
        void (*delRead)(void *privdata);
        void (*addWrite)(void *privdata);
        void (*delWrite)(void *privdata);
        void (*cleanup)(void *privdata);
    } ev;

    /* Called when either the connection is terminated due to an error or per
     * user request. The status is set accordingly (REDIS_OK, REDIS_ERR). */
    redisDisconnectCallback *onDisconnect;

    /* Called when the first write event was received. */
    redisConnectCallback *onConnect;

    /* Regular command callbacks */
    redisCallbackList replies;

    /* Subscription callbacks */
    struct
    {
        redisCallbackList invalid;
        struct dict *channels;
        struct dict *patterns;
    } sub;
} redisAsyncContext;

/* Functions that proxy to hiredis */
redisAsyncContext *redisAsyncConnect(const char *ip, int port);
redisAsyncContext *redisAsyncConnectBind(const char *ip, int port, const char *source_addr);
redisAsyncContext *redisAsyncConnectUnix(const char *path);
int redisAsyncSetConnectCallback(redisAsyncContext *ac, redisConnectCallback *fn);
int redisAsyncSetDisconnectCallback(redisAsyncContext *ac, redisDisconnectCallback *fn);
void redisAsyncDisconnect(redisAsyncContext *ac);
void redisAsyncFree(redisAsyncContext *ac);

/* Handle read/write events */
void redisAsyncHandleRead(redisAsyncContext *ac);
void redisAsyncHandleWrite(redisAsyncContext *ac);

/* Command functions for an async context. Write the command to the
 * output buffer and register the provided callback. */
int redisvAsyncCommand(redisAsyncContext *ac, redisCallbackFn *fn, void *privdata, const char *format, va_list ap);
int redisAsyncCommand(redisAsyncContext *ac, redisCallbackFn *fn, void *privdata, const char *format, ...);
int redisAsyncCommandArgv(redisAsyncContext *ac, redisCallbackFn *fn, void *privdata, int argc, const char **argv, const size_t *argvlen);



/* END:   Added by fuxunhao, 2015/10/15   PN:async */


/* BEGIN: Added by fuxunhao, 2015/10/15   PN:aeEx */
typedef struct redisAeEvents
{
    redisAsyncContext *context;
    aeEventLoop *loop;
    int fd;
    int reading, writing;
} redisAeEvents;

void redisAeReadEvent(aeEventLoop *el, int fd, void *privdata, int mask);
void redisAeWriteEvent(aeEventLoop *el, int fd, void *privdata, int mask);
void redisAeAddRead(void *privdata);
void redisAeDelRead(void *privdata);
void redisAeAddWrite(void *privdata);
void redisAeDelWrite(void *privdata);
void redisAeCleanup(void *privdata);
int redisAeAttach(aeEventLoop *loop, redisAsyncContext *ac);
/* END:   Added by fuxunhao, 2015/10/15   PN:aeEx */

/* BEGIN: Added by fuxunhao, 2015/10/15   PN:AutoConfig */
aeEventLoop *aeCreateEventLoopFace(int setsize);
void aeMainFace(aeEventLoop *eventLoop);
/* END:   Added by fuxunhao, 2015/10/15   PN:AutoConfig */
#ifdef __cplusplus
}
#endif


#endif
