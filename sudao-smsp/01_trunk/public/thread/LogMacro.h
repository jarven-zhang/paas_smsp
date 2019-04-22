#ifndef _LOG_MACRO_H_
#define _LOG_MACRO_H_

#include "CLogThread.h"


#ifdef LogDebug
#undef LogDebug
#endif

#ifdef LogInfo
#undef LogInfo
#endif

#ifdef LogNotice
#undef LogNotice
#endif

#ifdef LogWarn
#undef LogWarn
#endif

#ifdef LogAlert
#undef LogAlert
#endif

#ifdef LogError
#undef LogError
#endif

#ifdef LogCrit
#undef LogCrit
#endif

#ifdef LogEmerg
#undef LogEmerg
#endif

#ifdef LogFatal
#undef LogFatal
#endif


#define LogDebug(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_DEBUG) {\
        Log(LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogInfo(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_INFO) {\
        Log(LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogNotice(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_NOTICE) {\
        Log(LEVEL_NOTICE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogElk(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_ELK) {\
        Log(LEVEL_ELK, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogWarn(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_WARN) {\
        Log(LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogAlert(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_ALERT) {\
        Log(LEVEL_ALERT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogError(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_ERROR) {\
        Log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogCrit(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_CRIT) {\
        Log(LEVEL_CRIT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogEmerg(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_EMERG) {\
        Log(LEVEL_EMERG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

#define LogFatal(...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_FATAL) {\
        Log(LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

//////////////////////////////////////////////////////////////////////////////////////////

// for thread

#define LogDebugT(...)  LogT_(LEVEL_DEBUG, __VA_ARGS__)
#define LogNoticeT(...) LogT_(LEVEL_NOTICE, __VA_ARGS__)
#define LogWarnT(...)   LogT_(LEVEL_WARN, __VA_ARGS__)
#define LogErrorT(...)  LogT_(LEVEL_ERROR, __VA_ARGS__)
#define LogFatalT(...)  LogT_(LEVEL_FATAL, __VA_ARGS__)

#define LogT_(level, ...) \
    if (g_pLogThread != NULL && g_pLogThread->GetLevel() <= LEVEL_FATAL) {\
        LogT(level, __THREAD_NAME_IN_THREAD_POOL__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);\
    }

//////////////////////////////////////////////////////////////////////////////////////////

// for printf

#define LogNoticeP(...) LogP_(LEVEL_NOTICE, __VA_ARGS__)
#define LogWarnP(...)   LogP_(LEVEL_WARN, __VA_ARGS__)
#define LogErrorP(...)  LogP_(LEVEL_ERROR, __VA_ARGS__)
#define LogFatalP(...)  LogP_(LEVEL_FATAL, __VA_ARGS__)

#define LogP_(level, ...) \
    LogP(level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

/////////////////////////////////////////////////////////

#define VNAME(name) (#name)

#define CHK_NULL_RETURN(p)          CHK_NULL_RET_CODE_(p, return)
#define CHK_NULL_RETURN_FALSE(p)    CHK_NULL_RET_CODE_(p, return false)
#define CHK_NULL_RETURN_NULL(p)     CHK_NULL_RET_CODE_(p, return NULL)
#define CHK_NULL_RETURN_CODE(p, c)  CHK_NULL_RET_CODE_(p, return c)
#define CHK_NULL_CONTINUE(p)        CHK_NULL_RET_CODE_(p, continue)

#define CHK_NULL_RET_CODE_(p, code) \
    if (NULL == p) \
    { \
        LogError("%s is NULL.", VNAME(p)); \
        code; \
    }

///////////////////////////////////////////////////////////

#define CHK_FUNC_RET(func)          CHK_FUNC_RETURN_CODE_(func, return)
#define CHK_FUNC_RET_FALSE(func)    CHK_FUNC_RETURN_CODE_(func, return false)

#define CHK_FUNC_RETURN_CODE_(func, code) \
    if (!(func)) \
    { \
        LogError("Call %s failed.", VNAME(func)); \
        code; \
    }

/////////////////////////////////////////////////////////

#define CHK_FUNC_RET(func)          CHK_FUNC_RETURN_CODE_(func, return)
#define CHK_FUNC_RET_FALSE(func)    CHK_FUNC_RETURN_CODE_(func, return false)

#define CHK_FUNC_RETURN_CODE_(func, code) \
    if (!(func)) \
    { \
        LogError("Call %s failed.", VNAME(func)); \
        code; \
    }

/////////////////////////////////////////////////////////

#define CHK_MAP_FIND_INT_RET(map_, iter, key)           CHK_MAP_FIND_INT_RET_(map_, iter, key, return)
#define CHK_MAP_FIND_INT_RET_FALSE(map_, iter, key)     CHK_MAP_FIND_INT_RET_(map_, iter, key, return false)

#define CHK_MAP_FIND_INT_RET_(map_, iter, key, code) \
    iter = map_.find(key); \
    if (iter == map_.end()) \
    { \
        LogError("Can not find key(%d) in %s.", key, VNAME(map_)); \
        code; \
    }

/////////////////////////////////////////////////////////

#define CHK_MAP_FIND_UINT_RET(map_, iter, key)          CHK_MAP_FIND_UINT_RET_(map_, iter, key, return)
#define CHK_MAP_FIND_UINT_RET_FALSE(map_, iter, key)    CHK_MAP_FIND_UINT_RET_(map_, iter, key, return false)
#define CHK_MAP_FIND_UINT_RET_CODE(map_, iter, key, c)  CHK_MAP_FIND_UINT_RET_(map_, iter, key, return c)
#define CHK_MAP_FIND_UINT_CONTINUE(map_, iter, key)     CHK_MAP_FIND_UINT_RET_(map_, iter, key, continue)

#define CHK_MAP_FIND_UINT_RET_(map_, iter, key, code) \
    iter = map_.find(key); \
    if (iter == map_.end()) \
    { \
        LogError("Can not find key(%lu) in %s.", key, VNAME(map_)); \
        code; \
    }

/////////////////////////////////////////////////////////

#define CHK_MAP_FIND_STR_RET(map_, iter, key)           CHK_MAP_FIND_STR_RET_(map_, iter, key, return)
#define CHK_MAP_FIND_STR_RET_FALSE(map_, iter, key)     CHK_MAP_FIND_STR_RET_(map_, iter, key, return false)
#define CHK_MAP_FIND_STR_RET_CODE(map_, iter, key, c)   CHK_MAP_FIND_STR_RET_(map_, iter, key, return c)

#define CHK_MAP_FIND_STR_RET_(map_, iter, key, code) \
    iter = map_.find(key); \
    if (iter == map_.end()) \
    { \
        LogError("Can not find key(%s) in %s.", key.data(), VNAME(map_)); \
        code; \
    }

///////////////////////////////////////////////////////////

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
    if (NULL != (p)) \
    { \
        delete(p); \
        (p) = NULL; \
    }
#endif

#define INIT_CHK_NULL_RET_FALSE(p) \
    if (NULL == p) \
    { \
        LogErrorP("%s is NULL.", VNAME(p)); \
        return false; \
    }

#define INIT_CHK_FUNC_RET_FALSE(func) \
    if (!(func)) \
    { \
        LogErrorP("Call %s failed.", VNAME(func)); \
        return false; \
    } \
    else \
    { \
        LogNoticeP("%s success.", VNAME(func)); \
    }

#define INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(func) \
    if (!(func)) \
    { \
        LogErrorP("Call %s failed.", VNAME(func)); \
        MAIN_INIT_OVER; \
        usleep(1*1000*1000); \
        return false; \
    } \
    else \
    { \
        LogNoticeP("%s success.", VNAME(func)); \
    }

#define INIT_CHK_MAP_FIND_STR_RET_FALSE(map_, iter, key) \
        do{ \
            iter = map_.find(key); \
            if (iter == map_.end()) \
            { \
                LogErrorP("Can not find key(%s) in %s.", key.data(), VNAME(map_)); \
                return false; \
            } \
        }while(0);

////////////////////////////////////////////////////////////////////////////////////////////////

#define MAIN_INIT_OVER \
    { \
        usleep(1*1000*1000); \
        { \
            boost::lock_guard<boost::mutex> lk(g_mutexInitCv); \
            g_bMainInitOver = true; \
        } \
        LogFatalP("===========> Main thread init over and notify other thread <==========="); \
        g_cv.notify_all(); \
    }

#define WAIT_MAIN_INIT_OVER \
    { \
        boost::unique_lock<boost::mutex> lk(g_mutexInitCv); \
        LogWarnP("========> Thread id[%d:%u], name[%s] waiting[%d] <========", \
            GetThreadId(), \
            __THREAD_INDEX_IN_THREAD_POOL__, \
            GetThreadName().data(), \
            g_bMainInitOver); \
        while (!g_bMainInitOver) \
        { \
            g_cv.wait(lk); \
        } \
        LogWarnP("===> Thread id[%d:%u], name[%s] start[%d] <===", \
            GetThreadId(), \
            __THREAD_INDEX_IN_THREAD_POOL__, \
            GetThreadName().data(), \
            g_bMainInitOver); \
    }

////////////////////////////////////////////////////////////////////////////////////////////////


#endif /* _LOG_MACRO_H_ */

