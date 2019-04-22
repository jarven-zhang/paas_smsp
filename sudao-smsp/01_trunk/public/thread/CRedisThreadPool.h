#ifndef _CRedisThreadPOOL_h_
#define _CRedisThreadPOOL_h_

#include <string.h>
#include "Thread.h"
#include "hiredis.h"

static const UInt32 REDISTHREADNUM = 100;   //maxThread number


class CRedisThread;
class CRedisThreadPool:public CThread
{
public:
    CRedisThreadPool(const char *name, UInt32 uThreadNum,UInt32 uRedisIndex);
    ~CRedisThreadPool();
    bool Init(std::string ip, unsigned int port, int connTimeout, std::string strPwd );

    int GetMsgSize();
    UInt32 GetMsgCount();
    void SetMsgCount(int count);

    bool PostDBUpdateMsg(TMsg* pMsg);
    virtual bool PostMsg(TMsg* pMsg);

    UInt32 getIndex();

    bool PostBroadCastMsg( TMsg* pMsg );

private:
    virtual void HandleMsg(TMsg* pMsg);

    CRedisThread* m_pRedisThreadNode[REDISTHREADNUM];
    std::string m_RedisIp;
    int m_RedisPort;
    int m_RedisConnTimeout;
    UInt32 m_iIndex;
    UInt32 m_uThreadNum;
    UInt32 m_uRedisIndex;
};


#if !defined(smsp_charge)
bool redisGet(cs_t strRedisCmd, cs_t strRedisDbKey, CThread* pThread, UInt64 ulSeq = 0, std::string strSession = "", CRedisThreadPool *pRedisThreadPool[] = NULL);
bool redisGetList(cs_t strRedisCmd, cs_t strRedisDbKey, CThread* pThread, UInt64 ulSeq = 0, UInt32 uiMaxSize = 0, string strSession = "");
bool redisSet(cs_t strRedisCmd, cs_t strRedisDbKey, CThread* pThread = NULL, CRedisThreadPool *pRedisThreadPool[] = NULL);
bool redisSetTTL(cs_t strRedisCmdKey, UInt32 uiTimeOut, cs_t strRedisDbKey, CRedisThreadPool *pRedisThreadPool[] = NULL);
bool redisDel(cs_t strRedisCmdKey, cs_t strRedisDbKey);
//bool redisSetSql(cs_t strRedisCmdKey, cs_t strSql, cs_t strFlag);
//bool redisSetReportMo(UInt64 uAccessId, cs_t strData);
#endif

bool SelectRedisThreadPoolIndex( CRedisThreadPool *pCRedisThreadPool[],TMsg* pMsg );
void postRedisMngMsg( CRedisThreadPool *pCRedisThreadPool[],TMsg* pMsg );

//////////////////////////////////////////////////////////////////////////////////////////////////

class CRedisThreadPoolsInitiator
{
public:
    CRedisThreadPoolsInitiator(const std::string& strRedisIp, int iRedisPort, UInt32 uThreadNum, std::string strPwd = "" );
    bool ParseRedisDBIdxConfig(const std::string& strRedisDBsConf);
    bool CreateThreadPools(CRedisThreadPool* pThreadPools[], size_t size);

private:
    std::string m_RedisIp;
    int m_RedisPort;
    UInt32 m_uThreadNum;
    std::string m_strPwd;

    UInt32 m_RedisDBIdxBitmap;
};

#endif

