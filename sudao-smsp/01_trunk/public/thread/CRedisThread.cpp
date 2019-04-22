#include "CRedisThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "base64.h"
#include "Comm.h"
#include "alarm.h"
#include "boost/regex.hpp"

static const int DEFAULT_PIPELINE_MAX_APPEND = 100;
static const bool DEFAULT_USE_PIPELINE_FLAG = false;
static const UInt32 PIPELINE_MAX_APPEND_LIIMT = 10000;
static const UInt64 REDIS_STATE_CHECK_PERIOD_MS = 2000;

CRedisThread::CRedisThread(const char *name,UInt32 uRedisIndex):CThread(name)
{
	m_uRedisIndex = (uRedisIndex < REDIS_USE_INDEX_MAX) ? uRedisIndex : 0;
    m_bUsePipeline = DEFAULT_USE_PIPELINE_FLAG;
	m_uRedisCmdAppendMaxCnt = DEFAULT_PIPELINE_MAX_APPEND;
	m_uLastCheckTime = 0;
	m_uCheckPeriod   = REDIS_STATE_CHECK_PERIOD_MS;
}

CRedisThread::~CRedisThread()
{
    LogDebug("~Redis redisFree!! OK. \n");
}

bool CRedisThread::Init( std::string ip, unsigned int port, int connTimeout, std::string &strPwd )
{
	m_listRedisReq.clear();
    if(false == CThread::Init())
    {
    	printf("CThread::Init() fail!! \n");
        return false;
    }

	m_componentId =  g_uComponentId;
	PropertyUtils::GetValue("db_execult_err_cunts", m_uRedisExecultErrCunts);
	m_redisRunner = &m_redisServ0;
	m_redisFree   = &m_redisServ1;

	m_redisRunner->Init( connTimeout, m_uRedisExecultErrCunts, m_uRedisIndex );

	if(!m_redisRunner->Conn( ip, port, strPwd ))
	{
		printf("redis ip:%s,port:%d is connect is failed.\n",ip.data(),port);
		return false;
	}

    std::map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    return true;
}

void CRedisThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "REDIS_PIPELINE_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        std::size_t pos = strTmp.find_last_of(";");
        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                strSysPara.c_str(), strTmp.c_str());
			break;
        }

        int nTmp1 = atoi(strTmp.substr(0, pos).data());
        int nTmp2 = atoi(strTmp.substr(pos + 1).data());

        if (0 > nTmp1)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp1);
            break;
        }

        if ((0 > nTmp2) || ((int)PIPELINE_MAX_APPEND_LIIMT < nTmp2))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp2);
            break;
        }

		m_bUsePipeline = (nTmp1 == 1);
		m_uRedisCmdAppendMaxCnt = nTmp2;
    }
    while (0);

    LogNotice("System parameter(%s) value(%d, %u).",
        strSysPara.c_str(), m_bUsePipeline, m_uRedisCmdAppendMaxCnt);
}

void CRedisThread::RedisFailedAlarm(UInt32 selectIndex, int iCount, int type)
{
	std::string strChinese = "";

	if (type == REDIS_CONNECT_FAIL)
	{
		strChinese = DAlarm::getRedisFailedAlarmStr(iCount, m_componentId, REDIS_CONNECT_FAIL);
		Alarm(REDIS_CONN_FAILED_ALARM, "redis", strChinese.data());
	}
	else if (type == REDIS_EXE_COMMAND_FAIL)
	{
		char redisIndex = selectIndex + '0';
		strChinese = DAlarm::getRedisFailedAlarmStr(iCount, m_componentId, REDIS_EXE_COMMAND_FAIL);
		Alarm(REDIS_CONN_FAILED_ALARM, &redisIndex, strChinese.data());
	}
	else
	{
		LogError("RedisFailedAlarm type is error.");
	}
}

bool CRedisThread::PostMngMsg(TMsg* pMsg)
{
    pthread_mutex_lock(&m_mutex);
    m_msgMngQueue.AddMsg(pMsg);
    pthread_mutex_unlock(&m_mutex);
    return true;
}

void CRedisThread::MainLoop(void)
{
    WAIT_MAIN_INIT_OVER

    while( true )
    {
        m_pTimerMng->Click();
		/*获取管理消息*/
        pthread_mutex_lock(&m_mutex);
		TMsg* pMsg = m_msgMngQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);
		if( pMsg != NULL )
		{
            HandleMsg(pMsg);
			delete(pMsg);
			pMsg = NULL;
		}

		/*链路不可用时,将消息缓存在内存*/
		if( !checkRedisServState())
		{
			usleep(g_uSecSleep);
			continue;
		}

		/*获取一般请求消息*/
        pthread_mutex_lock(&m_mutex);
        pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

		if( pMsg == NULL )
		{
			if( 0 == RedisPipelineGetResult())
			{
				usleep(g_uSecSleep);
			}
		}
		else if ( NULL != pMsg )
		{
		    HandleMsg( pMsg );
		    delete pMsg;
			pMsg = NULL;
		}
	}

    return;
}

void CRedisThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
	m_iCount++;
	pthread_mutex_unlock(&m_mutex);

    switch ( pMsg->m_iMsgType )
    {
        case MSGTYPE_REDIS_REQ:
        {
            RedisReqProc( pMsg );
            break;
        }
		case  MSGTYPE_REDISLIST_REPORT_REQ:
        {
            RedisListReportReq(pMsg);
            break;
        }
		case  MSGTYPE_REDISLIST_REQ:
        {
            RedisLisReq(pMsg);
            break;
        }
		case  MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* msg = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == msg)
            {
                break;
            }

            GetSysPara(msg->m_SysParamMap);
            break;
        }
		case MSGTYPE_TIMEOUT:
		{

			break;
		}
		case MSGTYPE_RULELOAD_REDIS_ADDR_UPDATE_REQ:
		{
			UpdateRedisAddrInfo( pMsg );
			break;
		}

        default:
        {
            break;
        }
    }

}

/*处理多值请求*/
void CRedisThread::RedisReqProcMuti( TMsg *pMsg )
{
	TRedisReq pRedisReq = *( TRedisReq* )pMsg;
	if( ! m_redisRunner->checkConnState()
		||  REDIS_OK != m_redisRunner->appendCmd( pRedisReq.m_RedisCmd.data()))
	{
		TRedisResp* pRedisResp =  new TRedisResp();
		pRedisResp->m_iMsgType= MSGTYPE_REDIS_RESP;
		pRedisResp->m_iSeq = pRedisReq.m_iSeq;
		pRedisResp->m_uReqTime = pRedisReq.m_uReqTime;
		pRedisResp->m_RedisCmd = pRedisReq.m_RedisCmd;
		pRedisResp->m_strSessionID = pRedisReq.m_strSessionID;
		pRedisResp->m_RedisReply= NULL;
		pRedisResp->m_string = pRedisReq.m_string;
		pRedisReq.m_pSender->PostMsg(pRedisResp);
		return;
	}

	m_listRedisReq.push_back( pRedisReq );
	if( m_listRedisReq.size() >= m_uRedisCmdAppendMaxCnt )
	{
		RedisPipelineGetResult();
	}

}


int CRedisThread::RedisPipelineGetResult()
{
	int  resultSize = 0;
	if (!m_redisRunner->checkConnState())
	{
		LogWarn("Redis PileLine Context Error");
		return 0;
	}

	list < TRedisReq >::iterator itr = m_listRedisReq.begin();
	for( ;itr != m_listRedisReq.end(); itr++ )
	{
		redisReply* rdsReply = NULL;
		TRedisReq & redisReq = *itr;
		if( REDIS_OK != m_redisRunner->getReply((void **)&rdsReply ))
		{
			LogError("[Redis PipeLine] Exec redisGetReply Error ");
		}

		if( redisReq.m_pSender != NULL )
		{
			TRedisResp* pRedisResp =  new TRedisResp();
			pRedisResp->m_iMsgType= MSGTYPE_REDIS_RESP;
			pRedisResp->m_iSeq = redisReq.m_iSeq;
			pRedisResp->m_uReqTime = redisReq.m_uReqTime;
			pRedisResp->m_RedisCmd = redisReq.m_RedisCmd;
			pRedisResp->m_strSessionID = redisReq.m_strSessionID;
			pRedisResp->m_RedisReply= (redisReply*)rdsReply;
			pRedisResp->m_string = redisReq.m_string;
			redisReq.m_pSender->PostMsg(pRedisResp);
		}
		else
		{
			freeReply(rdsReply);
		}
	}

	resultSize =  m_listRedisReq.size();
	m_listRedisReq.clear();

	return resultSize;

}


void CRedisThread::RedisListMultiReq(TMsg* pMsg)
{
    // 先把前面的请求缓冲区执行
    RedisPipelineGetResult();

	int iRet = -2;
    TRedisListResp* pResp = NULL;

    TRedisReq* pRedisReq = (TRedisReq*)pMsg;
    do
    {
        if( NULL == pRedisReq )
        {
            LogError("param NULL error!!");
            break;
        }

        if (!m_redisRunner->checkConnState()
                || pRedisReq->m_uMaxSize <=0
                || pRedisReq->m_uMaxSize > 1000000)	//[1,1000 * 1000]
        {
			string strIp;
			UInt32 uPort;
			m_redisRunner->getRedisServAddr(strIp, uPort );
            LogWarn("redis list req failed, out of range [0, 1000], m_RedisCtx[%s:%u], m_uMaxSize[%d], m_RedisCmd[%s]",
                    strIp.data(), uPort, pRedisReq->m_uMaxSize, pRedisReq->m_RedisCmd.c_str());
            break;
        }

        // 使用局部变量list独立操作
        list < TRedisReq > listRedisReq;
        for( UInt32 i = 0; i < pRedisReq->m_uMaxSize; i++ )
        {
            // 失败的忽略
            if ( REDIS_OK == m_redisRunner->appendCmd(pRedisReq->m_RedisCmd.data()))
            {
                listRedisReq.push_back( *pRedisReq );
            }
        }

        // 获取结果
        pResp = new TRedisListResp();
        if ( !pResp )
        {
            LogError("list-cmd[%s] new TRedisListReq failed.", pRedisReq->m_RedisCmd.data());
            break;
        }

        list < TRedisReq >::iterator itr = listRedisReq.begin();
        for( ; itr != listRedisReq.end(); itr++ )
        {
            redisReply* rdsReply = NULL;
            if( REDIS_OK != m_redisRunner->getReply((void **)&rdsReply ))
            {
                LogError("list-cmd[%s] [Redis PipeLine] Exec redisGetReply Error ", pRedisReq->m_RedisCmd.data());
                iRet = (pResp->m_pRespList->size() > 0) ? 0 : -2;
				freeReply(rdsReply);
                continue;
            }

            // 如果结果集有内容，则部分成功也当作成功
            if ( rdsReply->type != REDIS_REPLY_STRING || rdsReply->str == NULL )
            {
              //  LogWarn("list-cmd[%s] Redis get empty reply, or non-string reply", pRedisReq->m_RedisCmd.data());
                iRet = (pResp->m_pRespList->size() > 0) ? 0 : -1;
			  	freeReply(rdsReply);
                continue;
            }

			string str = rdsReply->str;
			if(!str.empty())
            {
                pResp->m_pRespList->push_back(str);
            }
            iRet = 0;

            freeReply(rdsReply);
        }

    } while (0);

    if( pRedisReq->m_pSender != NULL && pResp )
    {
     	LogDebug("redisMultiList response, m_iResult[%d], resplist.size[%d]", iRet, pResp->m_pRespList->size());
        pResp->m_iMsgType= MSGTYPE_REDISLIST_RESP;
        pResp->m_iSeq = pRedisReq->m_iSeq;
		pResp->m_strKey = pRedisReq->m_strKey;
		pResp->m_RedisCmd = pRedisReq->m_RedisCmd;
		pResp->m_uMaxSize= pRedisReq->m_uMaxSize;
		pResp->m_iResult = iRet;
        pRedisReq->m_pSender->PostMsg(pResp);
		LogDebug("cmd[%s] is SUC.", pRedisReq->m_RedisCmd.data());
    }
    else
    {
    	LogDebug("cmd[%s] psender is NULL, or pResp is NULL.", pRedisReq->m_RedisCmd.data());
        if (pResp)
        {
            delete pResp;
            pResp = NULL;
        }
    }
}

void CRedisThread::RedisListReportMultiReq(TMsg* pMsg)
{
    // 先把前面的请求缓冲执行
    RedisPipelineGetResult();

	int iRet = -2;
	string strResp = "[";

    TRedisReq* pRedisReq = (TRedisReq*)pMsg;
    do
    {
        if(NULL == pRedisReq)
        {
            LogError("param NULL error!!");
            break;
        }

        if (!m_redisRunner->checkConnState()
                || pRedisReq->m_uMaxSize <=0
                || pRedisReq->m_uMaxSize > 1000 )	//[1,1000]
        {
			string strIp;
			UInt32 uPort;
			m_redisRunner->getRedisServAddr(strIp, uPort );
            LogWarn("redis list req failed, out of range [0, 1000], m_RedisCtx[%s:%d], m_uMaxSize[%d], m_RedisCmd[%s]",
                    strIp.data(), uPort, pRedisReq->m_uMaxSize, pRedisReq->m_RedisCmd.c_str());
            break;
        }

        list < TRedisReq > listRedisReq;
        for( UInt32 i = 0; i < pRedisReq->m_uMaxSize; i++ )
        {
            // 失败的忽略
            if (REDIS_OK == m_redisRunner->appendCmd( pRedisReq->m_RedisCmd.data()))
            {
                listRedisReq.push_back( *pRedisReq );
            }
        }

        // 获取结果
        list < TRedisReq >::iterator itr = listRedisReq.begin();
        for( ; itr != listRedisReq.end(); itr++ )
        {
            redisReply* rdsReply = NULL;
            if( REDIS_OK != m_redisRunner->getReply((void **)&rdsReply ))
            {
                LogError("report-list-cmd[%s] [Redis PipeLine] Exec redisGetReply Error", pRedisReq->m_RedisCmd.data());
                iRet = (strResp.length() > 1) ? 0 : -2;
				freeReply(rdsReply);
                continue;
            }

            if (rdsReply->type != REDIS_REPLY_STRING || rdsReply->str == NULL)
            {
                //LogWarn("report-list-cmd[%s] Redis get empty reply, or non-string reply", pRedisReq->m_RedisCmd.data());
                iRet = (strResp.length() > 1) ? 0 : -1;
				freeReply(rdsReply);
                continue;
            }

            std::string str = rdsReply->str;
            iRet = 0;
			if(strResp.length()<=2)
			{
				strResp.append(Base64::Decode(str));
			}
			else
			{
				strResp.append(",");
				strResp.append(Base64::Decode(str));
			}

            freeReply(rdsReply);
        }
    } while(0);

	strResp.append("]");

	if(strResp.length()<=2)
	{
		strResp = "";
	}

    if( pRedisReq->m_pSender != NULL )
    {
    	LogDebug("redisList report response, m_iResult[%d], strReport[%s]", iRet, strResp.data());
        TRedisListReportResp* pRedisResp =  new TRedisListReportResp();
        pRedisResp->m_iMsgType= MSGTYPE_REDISLIST_REPORT_RESP;
        pRedisResp->m_iSeq = pRedisReq->m_iSeq;
		pRedisResp->m_uReqTime = pRedisReq->m_uReqTime;
        pRedisResp->m_strSessionID = pRedisReq->m_strSessionID;
		pRedisResp->m_strReport.assign(strResp);
		pRedisResp->m_iResult = iRet;
		pRedisResp->m_uMaxSize = pRedisReq->m_uMaxSize;
		pRedisResp->m_strKey = pRedisReq->m_strKey;
        pRedisReq->m_pSender->PostMsg(pRedisResp);
		////LogDebug("cmd[%s] is SUC.", pRedisReq->m_RedisCmd.data());
    }
    else
    {
    	LogDebug("cmd[%s] psender is null.", pRedisReq->m_RedisCmd.data());
    }
}

bool CRedisThread::UpdateRedisAddrInfo( TMsg *pMsg )
{
	bool ret = false;
	string strIp = "";
	UInt32 uPort = 0;

	TRedisUpdateInfo *pRedisInfo = ( TRedisUpdateInfo *)pMsg;

	m_redisRunner->getRedisServAddr(strIp, uPort );
	if( pRedisInfo->m_uPort == uPort
		&& pRedisInfo->m_strIP == strIp )
	{
		LogNotice("Redis Addr[%s:%u] Not Change, No Need Proccess..",
			      strIp.data(), uPort);
		return true;
	}

    /*初始化新的连接*/
	m_redisFree->Init( m_uRedisExecultErrCunts, m_uRedisExecultErrCunts, m_uRedisIndex );

	ret = m_redisFree->Conn( pRedisInfo->m_strIP, pRedisInfo->m_uPort, pRedisInfo->m_strPwd );
	if( ret == true )
	{
		CRedisBase *tmp = m_redisRunner;
		if( m_bUsePipeline )
		{
			RedisPipelineGetResult();
		}
		m_redisRunner->Close();
		m_redisRunner = m_redisFree;
		m_redisFree = tmp;
		LogNotice("Redis Serv Changed Succ From[%s:%u] to[%s:%u]",
				  strIp.data(),uPort,
				  pRedisInfo->m_strIP.data(), pRedisInfo->m_uPort);
	}
	else
	{
		LogWarn("Change Redis Addr[ %s:%u->%s:%u] Error, [%s:%u] Can't Connect",
				strIp.data(),uPort,
				pRedisInfo->m_strIP.data(), pRedisInfo->m_uPort,
				pRedisInfo->m_strIP.data(), pRedisInfo->m_uPort);
	}
	return ret;
}

bool CRedisThread::checkRedisServState()
{
	struct timeval timeNow;
	bool ret = true;
	gettimeofday(&timeNow, NULL );

	UInt64 dwTime = 1000* timeNow.tv_sec + timeNow.tv_usec/1000 ;
	if( dwTime - m_uLastCheckTime > m_uCheckPeriod )
	{
		/*心跳到时候处理请求*/
		if( m_bUsePipeline )
		{
			RedisPipelineGetResult();
		}

		if( ! m_redisRunner->checkConnState( true ))
		{
			/*PIPELINE数据可能丢失*/
			ret = m_redisRunner->reConn();
		}

		if ( ret == false )
		{
			RedisFailedAlarm( m_uRedisIndex, m_uRedisExecultErrCunts, REDIS_CONNECT_FAIL );
		}

		m_uLastCheckTime = dwTime;

	}
	else
	{
		ret = m_redisRunner->checkConnState();
	}

	return ret;

}


void CRedisThread::RedisListReportReq(TMsg* pMsg)
{
	TRedisReq* pRedisReq = (TRedisReq*)pMsg;
	if( NULL == pRedisReq )
	{
		LogError("param error!!");
		return;
	}

	if ( m_bUsePipeline )
	{
	    RedisListReportMultiReq( pMsg );
        return;
	}

	int iRet = -2;
	string strResp = "[";
	if( !m_redisRunner->checkConnState()
		|| pRedisReq->m_uMaxSize <=0
		|| pRedisReq->m_uMaxSize > 1000 )	//[1,1000]
	{
		//return failed , redis link error/parma error  -2
		iRet = -2;
	}
	else
   	{
		for(UInt32 i = 0; i < pRedisReq->m_uMaxSize; i++)
		{
			void* Reply = NULL;
			Reply = m_redisRunner->execSyncCmd( pRedisReq->m_RedisCmd.data());
			if( Reply == NULL )
			{
				if( strResp.length() > 1 )
				{
					//has already got report before
					iRet= 0;
				}
				else
				{
					iRet = -2;		//redis command execlute fail
				}
				freeReply(Reply);
				break;
			}

			redisReply *rdsReply = (redisReply*)Reply;
			//redis replyed
			if((rdsReply->type != REDIS_REPLY_STRING) || rdsReply->str == NULL)
			{
				if(strResp.length() > 1)
				{
					//has already got report before
					iRet= 0;
				}
				else
				{
					iRet = -1;		//return failed , redis has no key/no elment
				}
				freeReply(rdsReply);
				break;
			}

			//redis replyed suc
			string str = rdsReply->str;
			iRet = 0;		//get value suc
			if(strResp.length()<=2)
			{
				strResp.append(Base64::Decode(str));
			}
			else
			{
				strResp.append(",");
				strResp.append(Base64::Decode(str));
			}

			freeReply(rdsReply);
		}
   	}
	strResp.append("]");

	if(strResp.length()<=2)
	{
		strResp = "";
	}

	//response
	if( pRedisReq->m_pSender != NULL )
    {
    	LogDebug("redisList report response, m_iResult[%d], strReport[%s]", iRet, strResp.data());
        TRedisListReportResp* pRedisResp =  new TRedisListReportResp();
        pRedisResp->m_iMsgType= MSGTYPE_REDISLIST_REPORT_RESP;
        pRedisResp->m_iSeq = pRedisReq->m_iSeq;
		pRedisResp->m_uReqTime = pRedisReq->m_uReqTime;
        pRedisResp->m_strSessionID = pRedisReq->m_strSessionID;
		pRedisResp->m_strReport.assign(strResp);
		pRedisResp->m_iResult = iRet;
		pRedisResp->m_uMaxSize = pRedisReq->m_uMaxSize;
		pRedisResp->m_strKey = pRedisReq->m_strKey;
        pRedisReq->m_pSender->PostMsg(pRedisResp);
    }
    else
    {
    	LogDebug("cmd[%s] psender is null.", pRedisReq->m_RedisCmd.data());
    }

}

void CRedisThread::RedisLisReq(TMsg* pMsg)
{
	TRedisReq* pRedisReq = (TRedisReq*)pMsg;

	//check redislink, check param,
	int iRet = -2;
	if(NULL == pRedisReq)
	{
		LogError("param error!!");
		return;
	}

	if ( m_bUsePipeline )
	{
	    RedisListMultiReq(pMsg);
        return;
	}

	//new response
	TRedisListResp* pResp = new TRedisListResp();

	if( !m_redisRunner->checkConnState()
		|| (pRedisReq->m_uMaxSize <=0)
		|| (pRedisReq->m_uMaxSize >1000000))	//[1,1000*1000]
	{
		string strIp;
		UInt32 uPort;
		m_redisRunner->getRedisServAddr(strIp, uPort );
		//return failed , redis link error/parma error  -2
		LogWarn("redis list req failed, m_RedisCtx[%s:%u], m_uMaxSize[%u], m_RedisCmd[%s]",
				strIp.data(), uPort, pRedisReq->m_uMaxSize, pRedisReq->m_RedisCmd.c_str());
		iRet = -2;
	}
	else
	{
		for( UInt32 i = 0; i < pRedisReq->m_uMaxSize; i++ )
		{
			void* Reply = NULL;
			Reply = m_redisRunner->execSyncCmd(pRedisReq->m_RedisCmd.data());
			if(Reply == NULL)
			{
				if(pResp->m_pRespList->size() > 0)
				{
					//has already got resp before
					iRet= 0;
				}
				else
				{
					iRet = -2;		//redis command execlute fail
				}
				freeReply(Reply);
				break;
			}

			redisReply *rdsReply = (redisReply*)Reply;
			//redis replyed
			if((rdsReply->type != REDIS_REPLY_STRING) || rdsReply->str == NULL)
			{
				//redis replyed failed
				if(pResp->m_pRespList->size() > 0)
				{
					//has already got report before
					iRet= 0;
				}
				else
				{
					iRet = -1;		//return failed , redis has no key/no elment
				}
				freeReply(rdsReply);
				break;
			}

			//redis replyed suc
			string str = rdsReply->str;
			if(!str.empty())
			{
				pResp->m_pRespList->push_back(str);
			}

			iRet = 0;		//get value suc

			freeReply(rdsReply);
		}
	}
	//response
	if(pRedisReq->m_pSender != NULL)
    {
    	LogDebug("redisList response, m_iResult[%d], resplist.size[%d]",
				iRet, pResp->m_pRespList->size());
        pResp->m_iMsgType= MSGTYPE_REDISLIST_RESP;
        pResp->m_iSeq = pRedisReq->m_iSeq;
		pResp->m_strKey = pRedisReq->m_strKey;
		pResp->m_RedisCmd = pRedisReq->m_RedisCmd;
		pResp->m_uMaxSize= pRedisReq->m_uMaxSize;
		pResp->m_iResult = iRet;
        pRedisReq->m_pSender->PostMsg(pResp);
		LogDebug("cmd[%s] is SUC.", pRedisReq->m_RedisCmd.data());
    }
    else
    {
    	LogDebug("cmd[%s] psender is null.", pRedisReq->m_RedisCmd.data());
		delete pResp;
		pResp = NULL;
    }

}

void CRedisThread::RedisReqProc( TMsg* pMsg )
{
    TRedisReq* pRedisReq = (TRedisReq*)pMsg;
    void* rdsReply = NULL;

	if( NULL == pRedisReq )
	{
		LogError("param error!!");
		return;
	}

	if ( m_bUsePipeline )
	{
		RedisReqProcMuti( pMsg );
		return;
	}

	struct timeval dwStart;
	gettimeofday(&dwStart,NULL);

	UInt32 uReExcult =0;
	do
	{
		if( !m_redisRunner->checkConnState())
		{
			if( !m_redisRunner->reConn())
			{
				LogWarn("==redis== index[%u] cmd[%s] is failed.",
						m_uRedisIndex, pRedisReq->m_RedisCmd.data());
				break;
			}
		}

	    rdsReply = m_redisRunner->execSyncCmd( pRedisReq->m_RedisCmd.data());
	    if ( NULL == rdsReply )
	    {
	    	m_redisRunner->reConn();
			if( ++uReExcult >= m_uRedisExecultErrCunts )
			{
				LogWarn("==redis== cmd excule failed 3 times");
				RedisFailedAlarm(m_uRedisIndex, m_uRedisExecultErrCunts, REDIS_EXE_COMMAND_FAIL);
			}
	    }
		else
		{
	    	/*Success*/
			break;
		}

	}while( uReExcult < m_uRedisExecultErrCunts );

    if ( pRedisReq->m_pSender != NULL )
    {
		struct timeval dwEnd;
		gettimeofday(&dwEnd,NULL);
		unsigned long dwTime=0;
		dwTime = 1000000*(dwEnd.tv_sec-dwStart.tv_sec)+(dwEnd.tv_usec-dwStart.tv_usec);

    	LogNotice("==end sequence:%lu== time:%lu index:%u,redis proc cmd:%s.",
					m_redisRunner->getSequence(), dwTime,
					m_uRedisIndex,pRedisReq->m_RedisCmd.data());
        TRedisResp* pRedisResp =  new TRedisResp();
        pRedisResp->m_iMsgType= MSGTYPE_REDIS_RESP;
        pRedisResp->m_iSeq = pRedisReq->m_iSeq;
		pRedisResp->m_uReqTime = pRedisReq->m_uReqTime;
		pRedisResp->m_RedisCmd = pRedisReq->m_RedisCmd;
        pRedisResp->m_strSessionID = pRedisReq->m_strSessionID;
        pRedisResp->m_RedisReply= (redisReply*)rdsReply;
		pRedisResp->m_string = pRedisReq->m_string;
        pRedisReq->m_pSender->PostMsg(pRedisResp);
    }
    else
    {
		struct timeval dwEnd;
		gettimeofday(&dwEnd,NULL);
		unsigned long dwTime=0;
		dwTime = 1000000*(dwEnd.tv_sec-dwStart.tv_sec)+(dwEnd.tv_usec-dwStart.tv_usec);
		if(REDIS_COMMON_USE_CLEAN_5 != m_uRedisIndex)//黑名单加载打印太多关闭
    		LogNotice("==end index:%u sequence:%lu== time:%lu,redis proc cmd:%s.",
    				  m_uRedisIndex,m_redisRunner->getSequence(),
    				dwTime,pRedisReq->m_RedisCmd.data());
		freeReply(rdsReply);
    }
}
