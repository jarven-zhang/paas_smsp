#include "CRedisThreadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include "ssl/md5.h"
#include "Comm.h"
#include "Fmt.h"


CRedisThreadPool::CRedisThreadPool(const char *name, UInt32 uThreadNum,UInt32 uRedisIndex):
    CThread(name)
{
	m_iIndex = 0;
	m_uThreadNum = uThreadNum;
	m_uRedisIndex = uRedisIndex;
	pthread_mutex_init(&m_mutex,NULL);
}

CRedisThreadPool::~CRedisThreadPool()
{

    LogDebug("~Redis redisFree!! OK. \n");
	////pthread_mutex_destroy(&m_mutex);
}

int CRedisThreadPool::GetMsgSize()
{
    pthread_mutex_lock(&m_mutex);
    int iMsgNum = 0;
	for(UInt32 i = 0; i< m_uThreadNum; i++)
	{
		iMsgNum += m_pRedisThreadNode[i]->GetMsgSize();
	}
    pthread_mutex_unlock(&m_mutex);
    return iMsgNum;
}

UInt32 CRedisThreadPool::GetMsgCount()
{
	UInt32 iCount = 0;
	pthread_mutex_lock(&m_mutex);
	for(UInt32 i = 0; i< m_uThreadNum; i++)
	{
		iCount += m_pRedisThreadNode[i]->GetMsgCount();
	}
	pthread_mutex_unlock(&m_mutex);
	return iCount;
}

void CRedisThreadPool::SetMsgCount(int count)
{
	pthread_mutex_lock(&m_mutex);
	for(UInt32 i = 0; i< m_uThreadNum; i++)
	{
		m_pRedisThreadNode[i]->SetMsgCount(count);
	}
    pthread_mutex_unlock(&m_mutex);
}


UInt32 CRedisThreadPool::getIndex()
{
	pthread_mutex_lock(&m_mutex);
	if(m_iIndex >= m_uThreadNum)
	{
		m_iIndex = 0;
	}
	int ret = m_iIndex;
	m_iIndex++;
	pthread_mutex_unlock(&m_mutex);

	return ret;
}

bool CRedisThreadPool::PostDBUpdateMsg( TMsg* pMsg )
{
	for( UInt32 idx =0; idx < m_uThreadNum; idx++ )
	{
		TMsg *pCopyMsg = NULL;
		if( pMsg->m_iMsgType == MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ )
		{
            pCopyMsg = new TUpdateSysParamRuleReq(*(TUpdateSysParamRuleReq*)pMsg);
		}
		else if (pMsg->m_iMsgType == MSGTYPE_RULELOAD_REDIS_ADDR_UPDATE_REQ )
		{
			pCopyMsg = new TRedisUpdateInfo(*((TRedisUpdateInfo *)pMsg));
		}
		m_pRedisThreadNode[idx]->PostMngMsg( pCopyMsg );
	}
    return true;
}

bool CRedisThreadPool::PostMsg(TMsg* pMsg)
{
	TRedisReq* pRedisReq = (TRedisReq*)pMsg;

	UInt32 uIndex = 0;
	UInt64 uNumSum = 0;

	for (UInt32 i = 0; i < pRedisReq->m_strKey.length(); i++)
	{
		UInt32 uTemp = pRedisReq->m_strKey.at(i);
		uTemp = uTemp%m_uThreadNum;

		uNumSum += uTemp;
	}

	uIndex = uNumSum%m_uThreadNum;
    m_pRedisThreadNode[uIndex]->PostMsg(pMsg);
    return true;
}

bool CRedisThreadPool::Init(std::string ip, unsigned int port, int connTimeout, std::string strPwd )
{
	std::string redishost = ip;
	unsigned int redisport = port;
	if(m_uThreadNum == 0)
	{
		return false;
	}

	for(UInt32 i = 0; i< m_uThreadNum; i++)
	{
	    m_pRedisThreadNode[i] = new CRedisThread("RedisThread",m_uRedisIndex);
	    if(NULL == m_pRedisThreadNode[i])
	    {
	        printf("m_pRedisThreadNode[i] new is failed!\n");
	        return false;
	    }
	    if (false == m_pRedisThreadNode[i]->Init(redishost, redisport, connTimeout, strPwd ))
	    {
	        printf("m_pRedisThreadNode[%d] init is failed!\n",i);
	        return false;
	    }
	    if (false == m_pRedisThreadNode[i]->CreateThread())
	    {
	        printf("m_pRedisThreadNode[%d] create is failed!\n",i);
	        return false;
	    }
	}

	LogNotice("[step 4] RedisThreads init is over,sum[%d]!redishost:%s|redisport:%d",  m_uThreadNum, redishost.data(), redisport);
	return true;
}

void CRedisThreadPool::HandleMsg(TMsg* pMsg)
{
	LogDebug("no reason to go here");
}

#if !defined(smsp_charge)
bool redisGet(cs_t strRedisCmd, cs_t strRedisDbKey, CThread* pThread, UInt64 ulSeq, string strSession, CRedisThreadPool *pRedisThreadPool[])
{
    TRedisReq* pRedisReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pRedisReq);

    pRedisReq->m_RedisCmd = strRedisCmd;
    pRedisReq->m_strKey = strRedisDbKey;
    pRedisReq->m_pSender = pThread;
    pRedisReq->m_iSeq = ulSeq;
    pRedisReq->m_strSessionID = strSession;
    pRedisReq->m_iMsgType = MSGTYPE_REDIS_REQ;

    if (NULL == pRedisThreadPool)
    {
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisReq);
    }
    else
    {
        SelectRedisThreadPoolIndex(pRedisThreadPool, pRedisReq);
    }

    LogDebug("redis cmd:%s.", pRedisReq->m_RedisCmd.data());
    return true;
}

bool redisGetList(cs_t strRedisCmd, cs_t strRedisDbKey, CThread* pThread, UInt64 ulSeq, UInt32 uiMaxSize, string strSession)
{
    TRedisReq* pRedisReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pRedisReq);

    pRedisReq->m_RedisCmd = strRedisCmd;
    pRedisReq->m_strKey = strRedisDbKey;
    pRedisReq->m_pSender = pThread;
    pRedisReq->m_strSessionID = strSession;
    pRedisReq->m_uMaxSize = uiMaxSize;
    pRedisReq->m_iSeq = ulSeq;
    pRedisReq->m_iMsgType = MSGTYPE_REDISLIST_REQ;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisReq);

    LogDebug("redis cmd:%s.", pRedisReq->m_RedisCmd.data());
    return true;
}

bool redisSet(cs_t strRedisCmd, cs_t strRedisDbKey, CThread* pThread, CRedisThreadPool *pRedisThreadPool[])
{
    TRedisReq* pRedisReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pRedisReq);

    pRedisReq->m_RedisCmd = strRedisCmd;
    pRedisReq->m_strKey = strRedisDbKey;
    pRedisReq->m_pSender = pThread;
    pRedisReq->m_iMsgType = MSGTYPE_REDIS_REQ;

    if (NULL == pRedisThreadPool)
    {
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisReq);
    }
    else
    {
        SelectRedisThreadPoolIndex(pRedisThreadPool, pRedisReq);
    }

    LogDebug("redis cmd:%s.", pRedisReq->m_RedisCmd.data());
    return true;
}

bool redisSetTTL(cs_t strRedisCmdKey, UInt32 uiTimeOut, cs_t strRedisDbKey, CRedisThreadPool *pRedisThreadPool[])
{
    TRedisReq* pRedisReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pRedisReq);

    pRedisReq->m_RedisCmd.assign("EXPIRE ");
    pRedisReq->m_RedisCmd.append(strRedisCmdKey);
    pRedisReq->m_RedisCmd.append(" ");
    pRedisReq->m_RedisCmd.append(to_string(uiTimeOut));
    pRedisReq->m_strKey = strRedisDbKey;
    pRedisReq->m_iMsgType = MSGTYPE_REDIS_REQ;

    if (NULL == pRedisThreadPool)
    {
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisReq);
    }
    else
    {
        SelectRedisThreadPoolIndex(pRedisThreadPool, pRedisReq);
    }

    LogDebug("redis cmd:%s.", pRedisReq->m_RedisCmd.data());
    return true;
}

bool redisDel(cs_t strRedisCmdKey, cs_t strRedisDbKey)
{
    return redisSet("DEL " + strRedisCmdKey, strRedisDbKey);
}
#endif

//add by liaojialin 2017-07-22
bool SelectRedisThreadPoolIndex(CRedisThreadPool *pCRedisThreadPool[],TMsg* pMsg)
{
    UInt32 uReidsIndex = 0;
    TRedisReq* pRedisReq = (TRedisReq*)pMsg;
    if(NULL == pCRedisThreadPool || NULL == pRedisReq)
    {
        LogError("param error!!");
        return false;
    }

    string strKey = pRedisReq->m_strKey;
    if((string::npos  != strKey.find("accesssms")) ||
        (string::npos != strKey.find("mocsid")) ||
        (string::npos != strKey.find("SASP_OverRate")) ||
        (string::npos != strKey.find("SASP_KeyWord_OverRate")))
    {
        uReidsIndex = REDIS_COMMON_USE_NO_CLEAN_0;
    }
    else if((string::npos != strKey.find("endauditsms")) ||
            (string::npos != strKey.find("needauditsms")) ||
            (string::npos != strKey.find("endgroupsendauditsms")) ||
            (string::npos != strKey.find("needgroupsendauditsms")) ||
            (string::npos != strKey.find("audit_detail")) ||
            (string::npos != strKey.find("cache_detail")) ||
            (string::npos != strKey.find("SASP_Global_OverRate"))||
            (string::npos != strKey.find("send_audit_intercept")))
    {
        uReidsIndex = REDIS_COMMON_USE_CLEAN_1;
    }
    else if((string::npos != strKey.find("moport")) ||
            (string::npos != strKey.find("sendmsgid")) ||
            (string::npos != strKey.find("sendsms"))||
            (string::npos != strKey.find("cluster_sms"))||
            (string::npos != strKey.find("sign_mo_port")))
    {
        uReidsIndex = REDIS_COMMON_USE_CLEAN_2;
    }
    else if((string::npos != strKey.find("active_report_cache")) ||
            (string::npos != strKey.find("active_mo_cache")))
    {
        uReidsIndex = REDIS_UNCOMMON_USE_CLEAN_4;
    }
    else if((string::npos != strKey.find("report_cache")))
    {
        uReidsIndex = REDIS_UNCOMMON_USE_CLEAN_3;
    }
    else if((string::npos != strKey.find("black_list")))
    {
        uReidsIndex = REDIS_COMMON_USE_CLEAN_5;
    }
    else if((string::npos != strKey.find("cluster_data")))
    {
        uReidsIndex = REDIS_COMMON_USE_CLEAN_7 ;
    }
	else if( string::npos != strKey.find( "mergeLong" ))
	{
		uReidsIndex = REDIS_COMMON_USE_CLEAN_8;
	}
	else if((string::npos != strKey.find("SASP_Channel_OverRate")) ||
		(string::npos != strKey.find("SASP_Channel_Day_OverRate")))
	{
		uReidsIndex = REDIS_COMMON_USE_CLEAN_9;
	}
	else
	{
		//uReidsIndex = REDIS_COMMON_USE_CLEAN_2;// ChannelID_Channelsmsid
        LogWarn("reids strkey[%s] is unknow!!",strKey.data());
        return false;
    }

    if(NULL != pCRedisThreadPool[uReidsIndex])
    {
        pCRedisThreadPool[uReidsIndex]->PostMsg(pRedisReq);
        return true;
    }

    LogError("pCRedisThreadPool[%d] is NULL",uReidsIndex);

    return false;
}


void postRedisMngMsg( CRedisThreadPool *pCRedisThreadPool[],TMsg* pMsg )
{
	for( UInt32 i=0; i< REDIS_USE_INDEX_MAX; i++ )
	{
		if( pCRedisThreadPool[i])
		{
			pCRedisThreadPool[i]->PostDBUpdateMsg( pMsg );
		}
	}
	SAFE_DELETE(pMsg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CRedisThreadPoolsInitiator::CRedisThreadPoolsInitiator( const std::string& strRedisIp, int iRedisPort, UInt32 uThreadNum, std::string strPwd )
    : m_RedisIp(strRedisIp),
      m_RedisPort(iRedisPort),
      m_uThreadNum(uThreadNum),
      m_strPwd(strPwd),
      m_RedisDBIdxBitmap(0)
{
}

// Parse redis db index config to a bitmap.
// [Valid Format]: 1,3-7,9  => b0010 1111 1010
bool CRedisThreadPoolsInitiator::ParseRedisDBIdxConfig(const std::string& strRedisDBsConf)
{
    if (strRedisDBsConf.empty()) {
        m_RedisDBIdxBitmap = 0;
        return true;
    }

    std::vector<std::string> vRedisDBIds;
    Comm::split(const_cast<std::string&>(strRedisDBsConf), ",", vRedisDBIds);

    for (size_t i = 0; i < vRedisDBIds.size(); ++i)
    {
        std::vector<std::string> vRedisDBIdRange;
        Comm::split(vRedisDBIds[i], "-", vRedisDBIdRange);

        int range_size = vRedisDBIdRange.size();
        if (range_size > 2) {
            printf("Check RedisDB config: invalid config [%s]. Valid format is like: '1,3,4-7'\n", strRedisDBsConf.c_str());
            return false;
        }

        bool bRange = (range_size == 1) ? false : true;

        const std::string& strStart = vRedisDBIdRange[0];
        const std::string& strEnd = bRange ? vRedisDBIdRange[1] : vRedisDBIdRange[0];

        if (!Comm::isNumber(strStart)
                || !Comm::isNumber(strEnd)) {
            printf("Check RedisDB config: invalid num [%s-%s]", strStart.c_str(), strEnd.c_str());
            return false;
        }

        unsigned int uStart = atoi(strStart.c_str());
        unsigned int uEnd = atoi(strEnd.c_str());

        if (uStart > uEnd
                || uStart >= REDIS_USE_INDEX_MAX
                || uEnd >= REDIS_USE_INDEX_MAX)
        {
            printf("Check RedisDB config: Valid DB-index is [0~%u)\n", REDIS_USE_INDEX_MAX);
            return false;
        }

        for (unsigned int k = uStart; k <= uEnd; ++k)
        {
            m_RedisDBIdxBitmap |= (1 << k);
        }
    }

    return true;
}

bool CRedisThreadPoolsInitiator::CreateThreadPools(CRedisThreadPool* pThreadPools[], size_t size)
{
	char threadName[128] = {0};

    UInt32 uMaxDBIdx = (size < REDIS_USE_INDEX_MAX ? size : REDIS_USE_INDEX_MAX);

	for(UInt32 i = 0; i < uMaxDBIdx; i++)
	{
		if (((m_RedisDBIdxBitmap >> i)&0x01) == 0)
		{
            pThreadPools[i] = NULL;
			continue;
		}

		snprintf(threadName,sizeof(threadName),"RedisThread%u", i);
	    pThreadPools[i] = new CRedisThreadPool(threadName, m_uThreadNum,i);
	    if(NULL == pThreadPools[i])
	    {
	        printf("pRedisThreadPool[%u] new is failed!",i);
	        return false;
	    }
	    if (false == pThreadPools[i]->Init(m_RedisIp, m_RedisPort, 60, m_strPwd ))
	    {
	        printf("pRedisThreadPool[%u] init is failed!", i);
	        return false;
	    }

        printf("init REDIS-DB [%u]\n", i);
	}

    return true;
}


