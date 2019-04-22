#include "RptMoGetThread.h"
#include "main.h"
#include "HttpParams.h"
#include "base64.h"
#include "Comm.h"

const int RPTMO_GET_TIMER_ID = 12017323;
//const int RPTMO_GET_TIME = 5*60*1000;
const int RPTMO_GET_TIME = 60*1000;

const unsigned int EXE_REDISCMD_AGAIN_TIMER_ID = 22017323;
const int EXE_REDISCMD_AGAIN_TIME = 1*1000;



RptMoGetThread::RptMoGetThread(const char *name) : CThread(name)
{
    // PARAM_REPORT_GET_NUMBER default: 1000;60
    m_uMaxGetSize = 1000;
    m_uTime = 60;

    m_pTimer_CheckLink = NULL;
    m_pSnManager = NULL;
}

RptMoGetThread::~RptMoGetThread()
{

}

bool RptMoGetThread::Init()
{
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }

    m_pSnManager = new SnManager();
    if(NULL == m_pSnManager)
    {
        LogError("new SnManager() is failed.");
        return false;
    }


    //获取系统参数PARAM_REPORT_GET_NUMBER， 每次拉取的最大数量
    std::map<std::string, std::string> mapSysPara;
    g_pRuleLoadThread->getSysParamMap(mapSysPara);
    GetSysPara(mapSysPara);

    //设置定时器，每隔5mins 检查所有链路，然后拉取redis中的状态报告
    m_pTimer_CheckLink = SetTimer(RPTMO_GET_TIMER_ID, "GET_RPTMO", m_uTime*1000);   //5MINS

    return true;
}

void RptMoGetThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "PARAM_REPORT_GET_NUMBER";
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

        int nMaxGetSizeTmp = atoi(strTmp.substr(0, pos).data());
        int nTime = atoi(strTmp.substr(pos + 1).data());

        if ((0 > nMaxGetSizeTmp) || (1000 * 1000 < nMaxGetSizeTmp))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(),
                strTmp.c_str(),
                nMaxGetSizeTmp);

            break;
        }

        if ((0 > nTime) || (10 * 60 < nTime))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTime);

            break;
        }

        m_uMaxGetSize = nMaxGetSizeTmp;
        m_uTime = nTime;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u, %u).",
        strSysPara.c_str(),
        m_uMaxGetSize,
        m_uTime);
}

void RptMoGetThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOutMsg(pMsg);
            break;
        }
        case MSGTYPE_GET_LINKED_CLIENT_RESQ:
        {
            HandleGetLinkedClientResp(pMsg);
            break;
        }
        case MSGTYPE_REDISLIST_RESP:
        {
            HandleRedisListResp(pMsg);
            break;
        }
        case MSGTYPE_SGIP_CLIENT_LOGIN_SUC:
        {
            HandleClientLoginSucInfo(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == pSysParamReq)
            {
                break;
            }

            GetSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        default:
        {
            LogWarn("msgType[%d] is invalid.",pMsg->m_iMsgType);
            break;
        }
    }
}

void RptMoGetThread::HandleTimeOutMsg(TMsg* pMsg)
{
    if(pMsg->m_strSessionID.compare("GET_RPTMO") == 0)
    {
        //CHECK ALL LINK AND GET RPTMO FROM REDIS

        TMsg* pReq = new TMsg();
        pReq->m_pSender = this;
        pReq->m_iMsgType= MSGTYPE_GET_LINKED_CLIENT_REQ;
//      CHK_ALL_THREAD_INIT_OK_RETURN();
        g_pUnitypThread->PostMsg(pReq);

        TMsg* pReq2 = new TMsg();
        pReq2->m_pSender = this;
        pReq2->m_iMsgType= MSGTYPE_GET_LINKED_CLIENT_REQ;
        g_pSgipRtAndMoThread->PostMsg(pReq2);

        //delete old timer and create a new timer
        if(m_pTimer_CheckLink != NULL)
        {
            delete m_pTimer_CheckLink;
            m_pTimer_CheckLink = NULL;
        }
        m_pTimer_CheckLink = SetTimer(RPTMO_GET_TIMER_ID, "GET_RPTMO", m_uTime*1000);   //5MINS
    }
    else if(pMsg->m_iSeq == EXE_REDISCMD_AGAIN_TIMER_ID)
    {
        LogNotice("execluet redis cmd again, cmd[%s]", pMsg->m_strSessionID.c_str());
        TRedisReq* preq = new TRedisReq();
        preq->m_iMsgType = MSGTYPE_REDISLIST_REQ;
        preq->m_RedisCmd = pMsg->m_strSessionID;
        preq->m_uMaxSize = m_uMaxGetSize;
        preq->m_iSeq = m_pSnManager->getSn();
        preq->m_pSender = this;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,preq);
    }

}

UInt32 RptMoGetThread::getRptMoByUser(string strCliendid)
{
    //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*

    //rpop report_cache:clientid

    string strkey = "report_cache:" + strCliendid;

    TRedisReq* preq = new TRedisReq();
    preq->m_iMsgType = MSGTYPE_REDISLIST_REQ;
    preq->m_RedisCmd = "rpop " + strkey;
    preq->m_strKey = strkey;
    /////LogDebug("get redisList req. cmd[%s], maxSize[%d]", preq->m_RedisCmd.data(), m_uMaxGetSize);
    preq->m_uMaxSize = m_uMaxGetSize;
    preq->m_iSeq = m_pSnManager->getSn();
    preq->m_pSender = this;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,preq);
    return 0;
}

void RptMoGetThread::HandleRedisListResp(TMsg* pMsg)
{
    TRedisListResp* pResp = (TRedisListResp*)(pMsg);

    //解析状态报告分发到各个线程
    LogNotice("resp.size[%d],maxsize[%d],iReturn[%d],cmd[%s]", pResp->m_pRespList->size(), pResp->m_uMaxSize, pResp->m_iResult, pResp->m_RedisCmd.c_str());
    for(list<string>::iterator it = pResp->m_pRespList->begin(); it != pResp->m_pRespList->end(); it++)
    {
        string strRptMo = *it;

        web::HttpParams param;
        param.Parse(strRptMo);

        string ismo = param._map["ismo"];
        if(ismo.compare("1") == 0)
        {
            ////lpush report_cache:clientId clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*
            //mo

            TMoRepostMsg* pMo = new TMoRepostMsg();
            pMo->m_uSmsFrom = atoi(param._map["usmsfrom"].c_str());
            pMo->m_strCliendid = param._map["clientid"];
            pMo->m_strPhone = param._map["phone"];
            pMo->m_strDestid = param._map["destid"];
            pMo->m_strContent = Base64::Decode(param._map["content"]);
            pMo->m_iMsgType = MSGTYPE_MO_GET_AGAIN_REQ;

            if (pMo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP)
            {
                g_pCMPPServiceThread->PostMsg(pMo);
            }
            else if (pMo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3)
            {
                g_pCMPP3ServiceThread->PostMsg(pMo);
            }
            else if (pMo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP)
            {
                g_pSMGPServiceThread->PostMsg(pMo);
            }
            else if (pMo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
            {
                g_pSgipRtAndMoThread->PostMsg(pMo);
            }
            else
            {
                LogError("smsfrom[%u] is invalid type.",pMo->m_uSmsFrom);
                delete pMo;
            }

        }
        else
        {
            //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*
            //report
            TReportRepostMsg* preq = new TReportRepostMsg();
            preq->m_uSmsFrom = atoi(param._map["usmsfrom"].c_str());
            preq->m_strCliendid = param._map["clientid"];
            preq->m_strMsgid= param._map["msgid"];
            preq->m_istatus = atoi(param._map["status"].c_str());
            preq->m_strRemark = Base64::Decode(param._map["remark"]);
            preq->m_strPhone = param._map["phone"];
            preq->m_strErr = Base64::Decode(param._map["err"]);     //smgp only
            preq->m_iMsgType = MSGTYPE_REPORT_GET_AGAIN_REQ;


            if (preq->m_uSmsFrom == SMS_FROM_ACCESS_CMPP)
            {
                g_pCMPPServiceThread->PostMsg(preq);
            }
            else if (preq->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3)
            {
                g_pCMPP3ServiceThread->PostMsg(preq);
            }
            else if (preq->m_uSmsFrom == SMS_FROM_ACCESS_SMGP)
            {
                g_pSMGPServiceThread->PostMsg(preq);
            }
            else if (preq->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
            {
                string strSgipSequence = param._map["msgid"];
                vector<string> vecSequence;
                Comm comm;
                comm.splitExVector(strSgipSequence, "|", vecSequence);
                UInt32 uSequenceIdNode = 0;
                UInt32 uSequenceIdTime = 0;
                UInt32 uSequenceId = 0;
                if(vecSequence.size() != 3)
                {
                    LogError("strSgipSequence[%s] is invalid.",strSgipSequence.data());
                }
                else
                {
                    uSequenceIdNode = atoi(vecSequence.at(0).data());
                    uSequenceIdTime= atoi(vecSequence.at(1).data());
                    uSequenceId = atoi(vecSequence.at(2).data());
                }

                CSgipRtReqMsg* pRt = new CSgipRtReqMsg();
                pRt->m_iMsgType = MSGTYPE_SGIP_RT_REQ;
                pRt->m_strSessionID.assign(preq->m_strCliendid);
                pRt->m_uSequenceId = uSequenceId;
                pRt->m_uSequenceIdNode = uSequenceIdNode;
                pRt->m_uSequenceIdTime = uSequenceIdTime;
                pRt->m_strPhone = preq->m_strPhone;
                pRt->m_strClientId = preq->m_strCliendid;


                if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(preq->m_istatus))
                {
                    pRt->m_uState = 0;
                    pRt->m_uErrorCode = 0;
                }
                else
                {
                    pRt->m_uState = 2;

                    if(preq->m_strRemark.length() <= 3 &&
                        preq->m_strRemark[0] >= '0' &&
                        preq->m_strRemark[0] <= '9' &&
                         atoi(preq->m_strRemark.c_str()) < 256 &&
                         atoi(preq->m_strRemark.c_str()) >= 0)
                     {
                         pRt->m_uErrorCode = atoi(preq->m_strRemark.c_str());
                     }
                     else
                     {
                         pRt->m_uErrorCode = 99;
                     }

                }
                LogNotice("HandleRedisListResp return errorCode=%s,status=%d,pRt->m_uErrorCode=%d,pRt->m_uState=%d",
                 preq->m_strRemark.c_str(), preq->m_istatus, pRt->m_uErrorCode, pRt->m_uState);
                g_pSgipRtAndMoThread->PostMsg(pRt);

                delete preq;
                preq = NULL;
            }
            else
            {
                LogError("[%s:%s] smsfrom[%u] is invalid type for repost report to user."
                    ,preq->m_strMsgid.data(),preq->m_strPhone.data(),preq->m_uSmsFrom);
                delete preq;
            }
        }


    }


    //if get size == maxSize, then get redis again.
    if(pResp->m_pRespList->size() >= pResp->m_uMaxSize)
    {
        //m_RedisCmd// again;
        //on timer to get redis again.
        //SetTimer(EXE_REDISCMD_AGAIN_TIMER_ID, pResp->m_RedisCmd, EXE_REDISCMD_AGAIN_TIME);    //1s

    }
}

void RptMoGetThread::HandleGetLinkedClientResp(TMsg* pMsg)
{
    CLinkedClientListRespMsg* presp = (CLinkedClientListRespMsg*)(pMsg);
    ////LogDebug("client sum[%u]", presp->m_list_Client.size());
    for(list<string>::iterator it = presp->m_list_Client.begin(); it != presp->m_list_Client.end(); it++)
    {
        ////LogDebug("get client[%s] report repost from redis.", it->c_str());
        getRptMoByUser(*it);
    }
}

void RptMoGetThread::HandleClientLoginSucInfo(TMsg* pMsg)
{
    LogDebug("client[%u], loginsuc, get redis rptmo to send", pMsg->m_strSessionID.c_str());
    getRptMoByUser(pMsg->m_strSessionID);
}

UInt32 RptMoGetThread::GetSessionMapSize()
{
    //return m_mapSession.size();
    return 0;
}

