#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "hiredis.h"
#include "ErrorCode.h"
#include "main.h"
#include "Uuid.h"
#include "UrlCode.h"
#include "Comm.h"
#include "base64.h"

CDirectMoThread::CDirectMoThread(const char *name):CThread(name)
{

}

CDirectMoThread::~CDirectMoThread()
{
    
}

bool CDirectMoThread::Init()
{   
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }
    m_pVerify = new VerifySMS();
    if(m_pVerify == NULL)
    {
        printf("m_pVerify is NULL.");
        return false;
    }

    g_pRuleLoadThread->getAccountMap(m_accountMap);
    g_pRuleLoadThread->getClientIdAndAppend(m_mapSignExtGw);
    g_pRuleLoadThread->getChannelBlackListMap(m_ChannelBlackListMap);
    g_pRuleLoadThread->getChannelExtendPortTableMap(m_mapChannelExt);

    g_pRuleLoadThread->getMqConfig(m_mapMqConfig);
    g_pRuleLoadThread->getComponentConfig(m_mapComponentConfig);
    
    time_t now = time(NULL);
    struct tm today = {0};
    localtime_r(&now,&today);
    int seconds;
    today.tm_hour = 0;
    today.tm_min = 0;
    today.tm_sec = 0;
    seconds = 24*60*60 - difftime(now,mktime(&today));

    m_pTimer = SetTimer(0, "MOINFO_CLEAN",seconds*1000);

    return true;
}

void CDirectMoThread::HandleMsg(TMsg* pMsg)
{   
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }
    
    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_TO_DIRECTMO_REQ:
        {
            ////mo
            HandleToDirectMoReqMsg(pMsg);
            break;
        }
        case MSGTYPE_DIRECT_SEND_TO_DIRECT_MO_REQ:
        {
            HandleDirectSendReqMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:    
        {   
            TUpdateAccountReq* msg = (TUpdateAccountReq*)pMsg;
            LogNotice("RuleUpdate Account update. map.size[%d]", msg->m_accountMap.size());
            m_accountMap.clear();
            m_accountMap = msg->m_accountMap;   
            break;
        }
        case MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ:
        {
            TUpdateSignextnoGwReq* pSign = (TUpdateSignextnoGwReq*)pMsg;
            LogNotice("RuleUpdate signextno_gw update,map size[%d].",pSign->m_SignextnoGwMap.size());
            m_mapSignExtGw.clear();
            m_mapSignExtGw = pSign->m_SignextnoGwMap;
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_EXTENDPORT_UPDATE_REQ:
        {
            TUpdateChannelExtendPortReq* pExt = (TUpdateChannelExtendPortReq*)pMsg;
            LogNotice("RuleUpdate channel_extendport update,map size[%d].",pExt->m_ChannelExtendPortTable.size());
            m_mapChannelExt.clear();
            m_mapChannelExt = pExt->m_ChannelExtendPortTable;
            break;
        }
        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqCon = (TUpdateMqConfigReq*)pMsg;
            m_mapMqConfig.clear();

            m_mapMqConfig = pMqCon->m_mapMqConfig;

            LogNotice("update t_sms_mq_configure size:%d.",m_mapMqConfig.size());
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            TUpdateComponentConfigReq* pMqCom = (TUpdateComponentConfigReq*)pMsg;
            m_mapComponentConfig.clear();

            m_mapComponentConfig = pMqCom->m_mapComponentConfigInfo;

            LogNotice("update t_sms_component_configure size:%d.",m_mapComponentConfig.size());
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisResp(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOutMsg(pMsg);
            break;
        }
        default:
        {
            LogWarn("msgType[%d] is invalid.", pMsg->m_iMsgType);
            break;
        }
    }
}


/////this is directsendthread to mourl info
void CDirectMoThread::HandleDirectSendReqMsg(TMsg* pMsg)
{
    TDirectSendToDriectMoUrlReqMsg* pSend = (TDirectSendToDriectMoUrlReqMsg*)pMsg;
    map<string,CMoParamInfo*>::iterator itr = m_mapMoInfo.find(pSend->m_MoParmInfo.m_strShowPhone);
    if (itr == m_mapMoInfo.end())
    {
        CMoParamInfo* pMoInfo = new CMoParamInfo(pSend->m_MoParmInfo);
        m_mapMoInfo.insert(make_pair(pMoInfo->m_strShowPhone,pMoInfo));
    }
    else
    {
        itr->second->m_strClientId = pSend->m_MoParmInfo.m_strClientId;
        itr->second->m_strSign = pSend->m_MoParmInfo.m_strSign;
        itr->second->m_strSignPort = pSend->m_MoParmInfo.m_strSignPort;
        itr->second->m_strExtendPort = pSend->m_MoParmInfo.m_strExtendPort;
        itr->second->m_strShowPhone = pSend->m_MoParmInfo.m_strShowPhone; 
        itr->second->m_uSmsFrom = pSend->m_MoParmInfo.m_uSmsFrom;
        itr->second->m_uC2sId = pSend->m_MoParmInfo.m_uC2sId;
    }
}


bool CDirectMoThread::getUserBySp(UInt64 uSeq)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {
        LogWarn("iSeq[%lu] is not find in m_mapSession.", uSeq);
        return false;
    }

    if (itr->second->m_strSp.length() >=  itr->second->m_strDstSp.length())
    {
        LogError("[ :%s] sp[%s],DstSp[%s] length is invalid.",
            itr->second->m_strPhone.data(),itr->second->m_strSp.data(),itr->second->m_strDstSp.data());
        return false;
    }

    string strExtendPort = itr->second->m_strDstSp.substr(itr->second->m_strSp.length());

    if (0 == itr->second->m_uChannelType)
    {
        AccountMap::iterator itGw = m_accountMap.begin();
        AccountMap::iterator itGwLong = m_accountMap.end();
        for (; itGw != m_accountMap.end(); ++itGw)
        {
            if (0 == strExtendPort.compare(0,itGw->second.m_strExtNo.length(),itGw->second.m_strExtNo))
            {
                if (itGwLong == m_accountMap.end())
                {
                    itGwLong = itGw;
                }
                else
                {
                    if (itGwLong->second.m_strExtNo.length() < itGw->second.m_strExtNo.length())
                    {
                        itGwLong = itGw;
                    }
                }
            }
        }

        if (itGwLong == m_accountMap.end())
        {
            LogWarn("[ :%s] extendport[%s] is not find match in userGw table.", itr->second->m_strPhone.data(),strExtendPort.data());
            return false;
        }

        UInt32 uSignPortLen = itGwLong->second.m_uSignPortLen;
        itr->second->m_strClientId = itGwLong->second.m_strAccount;
        itr->second->m_strSignPort = strExtendPort.substr(itGwLong->second.m_strExtNo.length(),uSignPortLen);

        if (strExtendPort.length() >= itGwLong->second.m_strExtNo.length()+uSignPortLen)
        {
            itr->second->m_strUserPort = strExtendPort.substr(itGwLong->second.m_strExtNo.length()+uSignPortLen);
        }
        else
        {
            LogWarn("[ :%s] 2ExtendPort[%s],length[%d].",itr->second->m_strPhone.data(),strExtendPort.data(),strExtendPort.length());
            return false;
        }

        return true;

    }
    else if (3 == itr->second->m_uChannelType)
    {
        std::multimap<UInt32,ChannelExtendPort>::iterator itPort = m_mapChannelExt.begin();
        std::multimap<UInt32,ChannelExtendPort>::iterator itPortLong = m_mapChannelExt.end();

        for (; itPort != m_mapChannelExt.end(); itPort++)
        {
            if (itPort->first == itr->second->m_uChannelId)
            {
                if (0 == strExtendPort.compare(0,itPort->second.m_strExtendPort.length(),itPort->second.m_strExtendPort))
                {
                    if (itPortLong == m_mapChannelExt.end())
                    {
                        itPortLong = itPort;
                    }
                    else
                    {
                        if (itPortLong->second.m_strExtendPort.length() < itPort->second.m_strExtendPort.length())
                        {
                            itPortLong = itPort;
                        }
                    }
                }
            }
        }

        if (itPortLong == m_mapChannelExt.end())
        {
            LogWarn("[ :%s:%u] extendport[%s] is not find match in channel_extendport table.",
                itr->second->m_strPhone.data(),itr->second->m_uChannelId,strExtendPort.data());
            return false;
        }

        itr->second->m_strClientId = itPortLong->second.m_strClientId;
        ////UserGWMap::iterator itGwExt = m_mapUserGw.find(itPortLong->second.m_strClientId);
        ////if (itGwExt == m_mapUserGw.end())
        ////{
            ////LogError("clientid[%s] is not find in userGw.",itPortLong->second.m_strClientId.data());
            ////return false;
        ////}

        ////UInt32 uSignPortLen = itGwExt->second.m_uSignPortLen;

        ////itr->second->m_strSignPort = strExtendPort.substr(itPortLong->second.m_strExtendPort.length(),uSignPortLen);

        if (strExtendPort.length() >= itPortLong->second.m_strExtendPort.length())
        {
            itr->second->m_strUserPort = strExtendPort.substr(itPortLong->second.m_strExtendPort.length());
        }
        else
        {
            LogWarn("[ :%s] 2ExtendPort[%s],length[%d].",itr->second->m_strPhone.data(),strExtendPort.data(),strExtendPort.length());
            return false;
        }

        return true;
        
    }
    else
    {
        LogError("channelType[%d] is invalid.",itr->second->m_uChannelType);
        return false;
    }
}

void CDirectMoThread::QuerySignMoPort(MoSession* pSession, UInt64 iSeq)
{
    TRedisReq* pRds = new TRedisReq();
    string strCmd = "";
    strCmd.append("HGETALL  ");
    char tmpKey[250] = {0};
    snprintf(tmpKey,250,"sign_mo_port:%d_%s:%s",pSession->m_uChannelId,pSession->m_strDstSp.data(), pSession->m_strPhone.data());
    strCmd.append(tmpKey);

    LogDebug("[ :%s] temKey[%s].",pSession->m_strPhone.data(),strCmd.data());
    pRds->m_RedisCmd = strCmd;
    pRds->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRds->m_iSeq = iSeq;
    pRds->m_pSender = this;
    pRds->m_strSessionID = "get_sign_mo_port";
    pRds->m_strKey = tmpKey;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRds);
}

/////this is channel take mo
void CDirectMoThread::HandleToDirectMoReqMsg(TMsg* pMsg)
{
    TToDirectMoReqMsg* pMoReq = (TToDirectMoReqMsg*)pMsg;
    MoSession* pSession = new MoSession();
    pSession->m_strContent = pMoReq->m_strContent;
    pSession->m_strDstSp = pMoReq->m_strDstSp;
    //remove 86
    if (m_pVerify->CheckPhoneFormat(pMoReq->m_strPhone) == false)
    {
        LogError("channel return phone[%s] format error", pMoReq->m_strPhone.data())
    }
    pSession->m_strPhone = pMoReq->m_strPhone;
    pSession->m_strSp = pMoReq->m_strSp;
    pSession->m_uChannelId = pMoReq->m_uChannelId;
    pSession->m_uMoType = pMoReq->m_uMoType;
    pSession->m_uChannelType = pMoReq->m_uChannelType;
    pSession->m_uChannelMoId = pMoReq->m_uChannelMoId;
    pSession->m_uPkTotal = pMoReq->m_uPkTotal;
    
    //long mo 
    if(pMoReq->m_bHead && pMoReq->m_uPkTotal > 1)
    {
        LongMoInfo longMoInfo;
        longMoInfo.m_strShortContent = pMoReq->m_strContent;
        longMoInfo.m_uPkNumber = pMoReq->m_uPkNumber;
        string strKey = pMoReq->m_strDstSp + "_";
        strKey.append(Comm::int2str(pMoReq->m_uChannelId) + "_");
        strKey.append(Comm::int2str(pMoReq->m_uRandCode));
        LongMoMap::iterator itLong = m_mapLogMo.find(strKey);
        if(itLong == m_mapLogMo.end())
        {       
            pSession->m_uRevShortMoCount++;
            pSession->m_vecLongMoInfo.push_back(longMoInfo);
            m_mapLogMo[strKey] = pSession;
            pSession->m_pTimer = SetTimer(LONG_MO_WAIT_TIMER_ID,strKey,30*1000);
            return;
        }
        else
        {
            SAFE_DELETE(pSession); 
            pSession = itLong->second;
            pSession->m_uRevShortMoCount++;
            pSession->m_vecLongMoInfo.push_back(longMoInfo);
            if(pSession->m_uRevShortMoCount == pSession->m_uPkTotal)
            {//get all short mo
                SAFE_DELETE(pSession->m_pTimer);
                if(false == mergeLongMo(pSession))
                {
                    LogWarn("mergeLongMo fail ,phone[%s:%u],sp[%s]",
                        pSession->m_strPhone.c_str(), pSession->m_uChannelId,pSession->m_strDstSp.c_str());
                }
                m_mapLogMo.erase(itLong);
            }
            else
            {
                return;
            }
            
        }
    }
    UInt64 iSeq = m_SnMng.getSn();
    m_mapSession[iSeq] = pSession;
    pSession->m_pTimer = SetTimer(iSeq, "SESSION_TIMEOUT", 20*1000);

    ////// channelType is not auto sign
    if ((1 == pSession->m_uChannelType) || (2 == pSession->m_uChannelType))
    {
        string strExtendPort = "";
        if (pSession->m_strSp.length() >= pSession->m_strDstSp.length())
        {
            LogError("[ :%s] channelSp[%s] length >= channelDstSp[%s] length.",
                pSession->m_strPhone.data(),pSession->m_strSp.data(),pSession->m_strDstSp.data());
            InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);
            delete pSession->m_pTimer;
            delete pSession;
            m_mapSession.erase(iSeq); 
            return;
        }

        strExtendPort = pSession->m_strDstSp.substr(pSession->m_strSp.length());
        LogNotice("[ :%s] ExtendPort[%s] ChannelType[%u].",pSession->m_strPhone.data(), strExtendPort.data(), pSession->m_uChannelType);

        /*固签无自扩展，从数据库获取数据*/
        if (1 == pSession->m_uChannelType)//////not support auto sign and not support extend port
        {
            char cChannelId[64] = {0};
            snprintf(cChannelId,64,"%u",pSession->m_uChannelId);
            string strKey = "";
            strKey.append(cChannelId);
            strKey.append("&");
            strKey.append(strExtendPort);
            
            pSession->m_strGwExtend = strExtendPort;
            
            SignExtnoGwMap::iterator iter = m_mapSignExtGw.find(strKey);
            if (iter == m_mapSignExtGw.end())
            {
                LogNotice("[ :%s] key[%s] is not find in memory t_signextno_gw,not support auto extend port",
                    pSession->m_strPhone.data(),strKey.data());
               //need to query redis
               
                QuerySignMoPort(pSession, iSeq);
                return;
            }
            else
            {
                bool isStar = false;
                vector<models::SignExtnoGw>::iterator itVec = iter->second.begin();
                for (; itVec != iter->second.end(); itVec++)
                {
                    if (itVec->m_strClient == "*")
                    {
                        isStar = true;
                        break;
                    }
                    
                }
                if (iter->second.size() > 1 || isStar)
                {
                    LogNotice("[ :%s] key[%s] too many client t_signextno_gw,not support auto extend port",
                    pSession->m_strPhone.data(),strKey.data());
                    QuerySignMoPort(pSession, iSeq);
                    return;
                }
                else
                {
                    pSession->m_strSign = iter->second[0].sign;
                    // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
                    if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(iter->second[0].m_strClient))
                    {
                        pSession->m_strClientId = iter->second[0].m_strClient;
                    }
                }
            }

        }
        /*固签有自扩展, 按照最长匹配原则匹配*/
        else if (2 == pSession->m_uChannelType)//////not support auto sign but support extend port
        {
            SignExtnoGwMap::iterator iter = m_mapSignExtGw.begin();
            SignExtnoGwMap::iterator itrLong = m_mapSignExtGw.end();
            
            for (; iter != m_mapSignExtGw.end(); ++iter)
            {
                UInt32 uChannelId = atoi(iter->second[0].channelid.data());
                if (pSession->m_uChannelId == uChannelId)
                {
                    if (0 == strExtendPort.compare(0,iter->second[0].appendid.length(),iter->second[0].appendid))
                    {
                        // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
                        if (BusType::CLIENT_STATUS_LOGOFF == GetAcctStatus(iter->second[0].m_strClient))
                        {
                            iter->second[0].m_strClient = "";
                        }
                        
                        if (itrLong == m_mapSignExtGw.end())
                        {
                            itrLong = iter;
                        }
                        else
                        {
                            if (itrLong->second[0].appendid.length() >= iter->second[0].appendid.length())
                            {
                                ;
                            }
                            else
                            {
                                itrLong = iter;
                            }
                        }
                    }
                }
            }

            if (itrLong == m_mapSignExtGw.end())
            {
                LogWarn("[ :%s] channelid[%d],extendPort[%s] is not find in memory must long match in t_signextno_gw.",
                    pSession->m_strPhone.data(),pSession->m_uChannelId,strExtendPort.data());
                QuerySignMoPort(pSession, iSeq);
                return;
            }
            else
            {
                bool isStar = false;
                vector<models::SignExtnoGw>::iterator itVec = itrLong->second.begin();
                for (; itVec != itrLong->second.end(); itVec++)
                {
                    if (itVec->m_strClient == "*")
                    {
                        isStar = true;
                        break;
                    }
                    
                }
                if (itrLong->second.size() > 1 || isStar)
                {
                    LogNotice("[ :%s] key[%s] too many client t_signextno_gw,not support auto extend port",
                    pSession->m_strPhone.data(),itrLong->first.data());
                    pSession->m_strGwExtend = itrLong->second[0].appendid;
                    QuerySignMoPort(pSession, iSeq);
                    return;
                }
                else
                {
                    pSession->m_strSign = itrLong->second[0].sign;
                    pSession->m_strUserPort = strExtendPort.substr(itrLong->second[0].appendid.length());
                    pSession->m_strClientId = itrLong->second[0].m_strClient;               
                }
            }
        }
        else
        {
            LogWarn("[ :%s] channelType[%d] is invalid.",pSession->m_strPhone.data(),pSession->m_uChannelType);
            InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);
            delete pSession->m_pTimer;
            delete pSession;
            m_mapSession.erase(iSeq); 
            return;
        }

        TRedisReq* pRedReq = new TRedisReq();
        string strCmd = "";
        strCmd.append("HGETALL  ");
        char tmpKey[250] = {0};
        snprintf(tmpKey,250,"mocsid:%s",pSession->m_strClientId.data());
        strCmd.append(tmpKey);

        LogDebug("[ :%s] temKey[%s].",pSession->m_strPhone.data(),strCmd.data());
        pRedReq->m_RedisCmd = strCmd;
        pRedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedReq->m_iSeq = iSeq;
        pRedReq->m_pSender = this;
        pRedReq->m_strSessionID = "get_SMSP_CB";
        pRedReq->m_strKey = tmpKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedReq);
        return;
    }
    
    if (LT_SGIP == pSession->m_uMoType)
    {
        TRedisReq* pRds = new TRedisReq();
        string strCmd = "";
        strCmd.append("HGETALL  ");
        char tmpKey[250] = {0};
        snprintf(tmpKey,250,"moport:%d_%s",pSession->m_uChannelId,pSession->m_strDstSp.data());
        strCmd.append(tmpKey);

        LogDebug("[ :%s] temKey[%s].",pSession->m_strPhone.data(),strCmd.data());
        pRds->m_RedisCmd = strCmd;
        pRds->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRds->m_iSeq = iSeq;
        pRds->m_pSender = this;
        pRds->m_strSessionID = "get_moport";
        pRds->m_strKey = tmpKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRds);
    }
    else if ((DX_SMGP == pSession->m_uMoType) || (YD_CMPP == pSession->m_uMoType) || (YD_CMPP3 == pSession->m_uMoType))
    {
        map<string,CMoParamInfo*>::iterator itr = m_mapMoInfo.find(pMoReq->m_strDstSp);
        if (itr == m_mapMoInfo.end())
        {
            LogWarn("[ :%s] this showphone[%s] length[%d] is not find in m_mapMoInfo size[%d].",
                pSession->m_strPhone.data(),pMoReq->m_strDstSp.data(),pMoReq->m_strDstSp.length(),m_mapMoInfo.size());
            if (false == getUserBySp(iSeq))
            {
                LogWarn("[ :%s] iSeq[%lu] sp[%s] is not find in t_user_gw",pSession->m_strPhone.data(),iSeq,pSession->m_strDstSp.data());
                InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);
                delete pSession->m_pTimer;
                delete pSession;
                m_mapSession.erase(iSeq); 
                return;
            }

            TRedisReq* pRed = new TRedisReq();
            string strCmd = "";
            strCmd.append("HGETALL  ");
            char tmpKey[250] = {0};
            snprintf(tmpKey,250,"mocsid:%s",pSession->m_strClientId.data());
            strCmd.append(tmpKey);

            LogDebug("[ :%s] temKey[%s].",pSession->m_strPhone.data(),strCmd.data());
            pRed->m_RedisCmd = strCmd;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = iSeq;
            pRed->m_pSender = this;
            pRed->m_strSessionID = "get_SMSP_CB";
            pRed->m_strKey = tmpKey;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);
        }
        else
        {   
            pSession->m_uC2sId = itr->second->m_uC2sId;
            pSession->m_strSignPort = itr->second->m_strSignPort;
            pSession->m_strUserPort = itr->second->m_strExtendPort;
            pSession->m_strSign = itr->second->m_strSign;
            pSession->m_strClientId = itr->second->m_strClientId;
            pSession->m_uSmsFrom = itr->second->m_uSmsFrom;
            processMoMessage(iSeq);
        }
    }
    else
    {   
        delete pSession->m_pTimer;
        delete pSession;
        m_mapSession.erase(iSeq);   
        LogWarn("[ :%s] MoType[%u] is invalid.",pSession->m_strPhone.data(),pSession->m_uMoType);
        return;
    }
    
}

void CDirectMoThread::processMoMessage(UInt64 uSeq)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", uSeq);
        return;
    }

    InsertMoRecord(uSeq, CHANAUTOORCLIENTFOUNDORERROR);
    
    map<UInt64,ComponentConfig>::iterator itrCom = m_mapComponentConfig.find(itr->second->m_uC2sId);
    if (itrCom == m_mapComponentConfig.end())
    {
        LogError("==direct mo== phone:%s,c2sid:%lu is not find in m_mapComponentConfig.",itr->second->m_strPhone.data(),itr->second->m_uC2sId);
        delete itr->second->m_pTimer;
        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    map<UInt64,MqConfig>::iterator itrMq =  m_mapMqConfig.find(itrCom->second.m_uMqId);
    if (itrMq == m_mapMqConfig.end())
    {
        LogError("==direct mo== phone:%s,mqid:%lu is not find in m_mapMqConfig.",itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
        delete itr->second->m_pTimer;
        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    string strExchange = itrMq->second.m_strExchange;
    string strRoutingKey = itrMq->second.m_strRoutingKey;

    string strData = "";

    string strClientId = "";
    // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
    if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(itr->second->m_strClientId))
    {
        strClientId = itr->second->m_strClientId;
    }

    ////type
    strData.append("type=");
    strData.append("2");
    ////clientid
    strData.append("&clientid=");
    strData.append(strClientId);
    ////channelid
    strData.append("&channelid=");
    strData.append(Comm::int2str(itr->second->m_uChannelId));
    ////phone
    strData.append("&phone=");
    strData.append(itr->second->m_strPhone);
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(itr->second->m_strContent));
    ////moid
    strData.append("&moid=");
    strData.append(itr->second->m_strMoId);
    ////time
    UInt64 uTime = time(NULL);
    strData.append("&time=");
    strData.append(Comm::int2str(uTime));
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(itr->second->m_strSign));
    ////signport
    strData.append("&signport=");
    strData.append(itr->second->m_strSignPort);
    ////userport
    strData.append("&userport=");
    strData.append(itr->second->m_strUserPort);
    ////smsfrom
    strData.append("&smsfrom=");
    strData.append(Comm::int2str(itr->second->m_uSmsFrom));
    ////showphone
    strData.append("&showphone=");
    strData.append(itr->second->m_strDstSp);
    LogDebug("==push mo==exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = strExchange;       
    pMQ->m_strRoutingKey = strRoutingKey;   
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMqAbIoProductThread->PostMsg(pMQ);

    delete itr->second->m_pTimer;
    delete itr->second;
    m_mapSession.erase(itr);

    return;
}

void CDirectMoThread::HandleRedisResp(TMsg* pMsg)
{
    TRedisResp* pResp = (TRedisResp*)pMsg;
    if ((NULL == pResp->m_RedisReply) 
        || (pResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pResp->m_RedisReply->elements == 0)))
    {
        LogError("iSeq[%lu] redisSessionId[%s],query redis is failed",pMsg->m_iSeq,pMsg->m_strSessionID.data());

        SessionMap::iterator itr = m_mapSession.find(pResp->m_iSeq);
        if (itr == m_mapSession.end())
        {
            LogError("iSeq[%lu] is not find m_mapSession.", pResp->m_iSeq);
            return;
        }
        if("get_SMSP_CB" == pMsg->m_strSessionID)
        {
            PushMoToRandC2s(pResp->m_iSeq); 
        }
        if("get_moport" == pMsg->m_strSessionID) ///only for auto channel , SGIP protocal
        {
            if (false == getUserBySp(pResp->m_iSeq))
            {
                LogWarn("[ :%s] iSeq[%lu] sp[%s] is not find in t_user_gw",itr->second->m_strPhone.data(),pResp->m_iSeq,itr->second->m_strDstSp.data());
                InsertMoRecord(pResp->m_iSeq, CHANAUTOORCLIENTFOUNDORERROR);
                delete itr->second->m_pTimer;
                delete itr->second;
                m_mapSession.erase(pResp->m_iSeq); 
                return;
            }

            TRedisReq* pRed = new TRedisReq();
            string strCmd = "";
            strCmd.append("HGETALL  ");
            char tmpKey[250] = {0};
            snprintf(tmpKey,250,"mocsid:%s",itr->second->m_strClientId.data());
            strCmd.append(tmpKey);

            LogDebug("[ :%s] temKey[%s].",itr->second->m_strPhone.data(),strCmd.data());
            pRed->m_RedisCmd = strCmd;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = pResp->m_iSeq;
            pRed->m_pSender = this;
            pRed->m_strSessionID = "get_SMSP_CB";
            pRed->m_strKey = tmpKey;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);
            return;
        }
        if ("get_sign_mo_port" == pMsg->m_strSessionID)
        {
            InsertMoRecord(pResp->m_iSeq, CHANGWCLIENTNOTFOUND);
            SAFE_DELETE(itr->second->m_pTimer);
            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            freeReply(pResp->m_RedisReply);
           
            return;
        }
        if (NULL != pResp->m_RedisReply)
        {
            freeReply(pResp->m_RedisReply);
        }
        
        
        InsertMoRecord(pResp->m_iSeq, CHANAUTOORCLIENTFOUNDORERROR);
        delete itr->second->m_pTimer;
        delete itr->second;
        m_mapSession.erase(itr);
    
        return;
    }

    if ("get_SMSP_CB" == pMsg->m_strSessionID)
    {
        HandleSMSPCBRedisResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else if ("get_moport" == pMsg->m_strSessionID)
    {
        HandleMoPortRedisResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else if ("get_sign_mo_port" == pMsg->m_strSessionID)
    {
        HandleGWMORedisResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else
    {
        LogError("this Redis type[%s] is invalid.",pMsg->m_strSessionID.data());
        freeReply(pResp->m_RedisReply);
        return;
    }

    freeReply(pResp->m_RedisReply);
    return;
}

void CDirectMoThread::HandleSMSPCBRedisResp(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    ////HMSET mocsid:clientid c2sid* smsfrom*
    string strTmp = "";
    UInt32 uSmsFrom = 0;
    UInt64 uC2sId = 0;

    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());

    strTmp = "";
    paserReply("c2sid", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());

    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uSmsFrom = uSmsFrom;

    processMoMessage(iSeq);
   
    return;
}

void CDirectMoThread::HandleMoPortRedisResp(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    ////redis HMSET moport:channelid_showphone clientid* sign* signport* userport* mourl* smsfrom*
    string strClientId = "";
    string strSign = "";
    string strSignPort = "";
    string strUserPort = "";
    UInt64 uC2sId = 0;
    UInt32 uSmsFrom = 0;
    string strTmp = "";

    paserReply("clientid", pRedisReply, strClientId);
    paserReply("sign", pRedisReply, strSign);
    paserReply("signport", pRedisReply, strSignPort);
    paserReply("userport", pRedisReply, strUserPort);
    
    paserReply("mourl", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());

    strTmp = "";
    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());

    if (0 == strSign.compare("null"))
    {
        strSign = "";
    }
    if (0 == strSignPort.compare("null"))
    {
        strSignPort = "";
    }    
    if (0 == strUserPort.compare("null"))
    {
        strUserPort = "";
    }

    itr->second->m_strClientId = strClientId;
    itr->second->m_strSign = strSign;
    itr->second->m_strSignPort = strSignPort;
    itr->second->m_strUserPort = strUserPort;
    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uSmsFrom = uSmsFrom;

    processMoMessage(iSeq);

    return;
}

void CDirectMoThread::HandleGWMORedisResp(redisReply* pRedisReply,UInt64 uSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", uSeq);
        return;
    }

    string strClientId = "";
    string strSign = "";
    string strSignPort = "";
    string strUserPort = "";
    UInt64 uC2sId = 0;
    UInt32 uSmsFrom = 0;
    string strTmp = "";

    paserReply("clientid", pRedisReply, strClientId);
    paserReply("sign", pRedisReply, strSign);
    paserReply("signport", pRedisReply, strSignPort);
    paserReply("userport", pRedisReply, strUserPort);
    
    paserReply("mourl", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());

    strTmp = "";
    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());
    
    std::map<string, SmsAccount>::iterator itrAccount = m_accountMap.find(strClientId);
    if(itrAccount == m_accountMap.end())
    {
        LogError("not have the clientid[%s]", strClientId.data());
        return;
    }
    
    UInt32 realSmsFrom = itrAccount->second.m_uSmsFrom;
    LogDebug("smsfrom=%u,redisSmsfrom=%u,strSign=%s,strUserPort=%s,uC2sId=%u"
        , realSmsFrom, uSmsFrom,strSign.data(),strUserPort.data(),uC2sId);

    if (0 == strSign.compare("null"))
    {
        strSign = "";
    }
    if (0 == strSignPort.compare("null"))
    {
        strSignPort = "";
    }    
    if (0 == strUserPort.compare("null"))
    {
        strUserPort = "";
    }

    itr->second->m_strClientId = strClientId;
    itr->second->m_strSign = strSign;
    itr->second->m_strSignPort = strSignPort;
    itr->second->m_strUserPort = strUserPort;
    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uSmsFrom = uSmsFrom;

    map<UInt64,ComponentConfig>::iterator itCom = m_mapComponentConfig.find(itr->second->m_uC2sId);
    if ((realSmsFrom == 6 || realSmsFrom == 7) && (uSmsFrom == realSmsFrom) &&
        (itCom != m_mapComponentConfig.end())&&(itCom->second.m_uComponentSwitch == 1))//https
    {

        processMoMessage(uSeq);
    }
    else
    {
        TRedisReq* pRed = new TRedisReq();
        string strCmd = "";
        strCmd.append("HGETALL  ");
        char tmpKey[250] = {0};
        snprintf(tmpKey,250,"mocsid:%s",strClientId.data());
        strCmd.append(tmpKey);

        LogDebug("[ :%s] temKey[%s].",itr->second->m_strPhone.data(),strCmd.data());
        pRed->m_RedisCmd = strCmd;
        pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRed->m_iSeq = uSeq;
        pRed->m_pSender = this;
        pRed->m_strSessionID = "get_SMSP_CB";
        pRed->m_strKey = tmpKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);        
    }
    
}

void CDirectMoThread::PushMoToRandC2s(UInt64 uSeq)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {   
        LogError("uSeq[%ld] is not find in mapSession.",uSeq);
        return;
    }

    UInt32 realSmsFrom = 0;
    std::map<string, SmsAccount>::iterator itrAccount = m_accountMap.find(itr->second->m_strClientId);
    if(itrAccount == m_accountMap.end())
    {
        LogError("push mo to random http,cant not find smsfrom by account");
        return;
    }
    realSmsFrom = itrAccount->second.m_uSmsFrom ;
    
    map<UInt64,MqConfig>::iterator itrMq;
    map<UInt64,ComponentConfig>::iterator itrCom = m_mapComponentConfig.begin();
    if(2 == realSmsFrom || 3 == realSmsFrom ||4 == realSmsFrom ||5 == realSmsFrom) //cmpp,smgp,sgip,smpp
    {
        for(;itrCom != m_mapComponentConfig.end();itrCom ++)
        {
            if(itrCom->second.m_strComponentType.compare("00") == 0 && itrCom->second.m_uComponentSwitch == 1)  //find c2s
            {
                LogDebug("*********weilu_test: c2s id is %ld mq id is %ld",itrCom ->first,itrCom->second.m_uMqId);
                 itrMq  =   m_mapMqConfig.find(itrCom->second.m_uMqId);
                if (itrMq == m_mapMqConfig.end())
                {
                    LogWarn("push mo ==phone:%s,mqid:%lu is not find in m_mapMqConfig.",
                        itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
                    return;
                }
                break;
            }
        }
    }
    else if(6 == realSmsFrom || 7 == realSmsFrom) //http ,http2
    {
        for(;itrCom != m_mapComponentConfig.end();itrCom ++)
        {
            if(itrCom->second.m_strComponentType.compare("08") == 0 && itrCom->second.m_uComponentSwitch == 1)  //find http 
            {
                LogDebug("*********weilu_test: http id is %ld mq id is %ld",itrCom ->first,itrCom->second.m_uMqId);
                 itrMq  =   m_mapMqConfig.find(itrCom->second.m_uMqId);
                if (itrMq == m_mapMqConfig.end())
                {
                    LogWarn("push mo ==phone:%s,mqid:%lu is not find in m_mapMqConfig.",
                        itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
                    return;
                }
                break;
            }
        }
    }
    else
    {
        LogWarn("account %s smsfrom  is %d ",itrAccount->second.m_strAccount.data(),realSmsFrom);
        return;
    }

    if(itrCom == m_mapComponentConfig.end())
    {
        LogError("push randon mo ==component type 00 is not find in component config ");
        return;

    }
    
    string strExchange = itrMq->second.m_strExchange;
    string strRoutingKey = itrMq->second.m_strRoutingKey;

    string strData = "";
    string strClientId = "";
    
    // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
    if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(itr->second->m_strClientId))
    {
        strClientId = itr->second->m_strClientId;
    }
    ////type
    strData.append("type=");
    strData.append("2");
    ////clientid
    strData.append("&clientid=");
    strData.append(strClientId);
    ////channelid
    strData.append("&channelid=");
    strData.append(Comm::int2str(itr->second->m_uChannelId));
    ////phone
    strData.append("&phone=");
    strData.append(itr->second->m_strPhone);
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(itr->second->m_strContent));
    ////moid
    if(itr->second->m_strMoId.empty())
    {
        string strUUID = getUUID();
        itr->second->m_strMoId = strUUID;
    }
    strData.append("&moid=");
    strData.append(itr->second->m_strMoId);
    ////time
    UInt64 uTime = time(NULL);
    strData.append("&time=");
    strData.append(Comm::int2str(uTime));
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(itr->second->m_strSign));
    ////signport
    strData.append("&signport=");
    strData.append(itr->second->m_strSignPort);
    ////userport
    strData.append("&userport=");
    strData.append(itr->second->m_strUserPort);
    ////smsfrom
    //weilu_test
    strData.append("&smsfrom=");
    strData.append(Comm::int2str(realSmsFrom)); ///http mo itr->second->m_uSmsFrom  
    ////showphone
    strData.append("&showphone=");
    strData.append(itr->second->m_strDstSp);
    LogDebug("==push mo==exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = strExchange;       
    pMQ->m_strRoutingKey = strRoutingKey;   
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMqAbIoProductThread->PostMsg(pMQ);

    return;     
}

void CDirectMoThread::InsertMoRecord(UInt64 uSeq, int type)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {   
        LogError("uSeq[%ld] is not find in mapSession.",uSeq);
        return;
    }
    
    char sql[4096]  = {0};
    time_t now = time(NULL);
    struct tm timeinfo = {0};
    char sztime[64] = {0};
    char szreachtime[64] = { 0 };
    if(now != 0 )
    {
        localtime_r((time_t*)&now,&timeinfo);
        strftime(szreachtime, sizeof(sztime), "%Y%m%d%H%M%S", &timeinfo);
    }

    string strUUID = getUUID();

    itr->second->m_strMoId = strUUID;
    char contentSQL[2684] = { 0 };
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
    if(MysqlConn != NULL)
    {
        mysql_real_escape_string(MysqlConn, contentSQL, itr->second->m_strContent.data(), itr->second->m_strContent.length());
    }
    
    char table[32];
    memset(table, 0, sizeof(table));
    if (type == CHANAUTOORCLIENTFOUNDORERROR)
    {
        string strClientId = "";
        // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
        if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(itr->second->m_strClientId))
        {
            strClientId = itr->second->m_strClientId;
        }
            
        snprintf(table, sizeof(table) - 1, "t_sms_record_molog_%04d%02d",timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
        snprintf(sql,4096,"insert into %s(moid,channelid,receivedate,phone,tophone,content,channelsmsid,"
            "clientid,mofee,chargetotal,paytime) values('%s','%d','%s','%s','%s','%s','%lu','%s','%d','%d','%s');",
            table,
            itr->second->m_strMoId.data(),
            itr->second->m_uChannelId,
            szreachtime,
            itr->second->m_strPhone.substr(0, 20).data(),
            itr->second->m_strDstSp.substr(0, 20).data(),
            contentSQL,
            itr->second->m_uChannelMoId,
            strClientId.data(),
            0,
            1,
            szreachtime);
    }
    else if (type == CHANGWCLIENTNOTFOUND)
    {
        snprintf(table, sizeof(table) - 1, "t_sms_record_sign_molog");
        snprintf(sql,8092,"insert into %s(mo_id,channel_id,receive_time,phone,to_phone,content,sign_port,sp_port,channel_smsid,state,"
        "update_time) values('%s','%d','%s','%s','%s','%s','%s','%s','%lu','%d','%s');",
        table,
        itr->second->m_strMoId.data(),
        itr->second->m_uChannelId,
        szreachtime,
        itr->second->m_strPhone.substr(0, 20).data(),
        itr->second->m_strDstSp.substr(0, 20).data(),
        contentSQL,
        ( true == itr->second->m_strGwExtend.empty()) ? "NULL" : itr->second->m_strGwExtend.data(),
        itr->second->m_strSp.data(),
        itr->second->m_uChannelMoId,
        0,
        szreachtime);
    }
    else
    {
        LogError("channel type error[%d]", type);
    }

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(g_strMQDBExchange);
    pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append(strUUID);
    g_pRecordMQProducerThread->PostMsg(pMQ);

    return;
    
}

void CDirectMoThread::HandleTimeOutMsg(TMsg* pMsg)
{
    LogWarn("iSeq[%lu,sessionId[%s] is timeout]",pMsg->m_iSeq, pMsg->m_strSessionID.data());
    if (0 == pMsg->m_strSessionID.compare("SESSION_TIMEOUT"))
    {
        InsertMoRecord(pMsg->m_iSeq, CHANAUTOORCLIENTFOUNDORERROR);
        SessionMap::iterator itr = m_mapSession.find(pMsg->m_iSeq);
        if (itr == m_mapSession.end())
        {
            LogWarn("iSeq[%lu] is not find in mapSession.",pMsg->m_iSeq);
            return;
        }

        delete itr->second->m_pTimer;
        delete itr->second;
        m_mapSession.erase(itr);
    }
    else if (0 == pMsg->m_strSessionID.compare("MOINFO_CLEAN"))
    {
        map<string,CMoParamInfo*>::iterator itr = m_mapMoInfo.begin();

        for (; itr != m_mapMoInfo.end();)
        {
            delete itr->second;
            m_mapMoInfo.erase(itr++);
        }

        delete m_pTimer;
        m_pTimer = NULL;
        m_pTimer = SetTimer(0, "MOINFO_CLEAN",24*60*60*1000);
    }
    else if(LONG_MO_WAIT_TIMER_ID == pMsg->m_iSeq)
    {
        LogWarn("Long mo wait time out!!,strKey[%s]",pMsg->m_strSessionID.c_str());
        LongMoMap::iterator itLong = m_mapLogMo.find(pMsg->m_strSessionID);
        if(itLong != m_mapLogMo.end())
        {
            delete itLong->second->m_pTimer;
            delete itLong->second;
            m_mapLogMo.erase(itLong);
        }
    }
    else
    {
        LogError("m_strSessionID[%s] is invalid.",pMsg->m_strSessionID.data());
    }
}

bool CDirectMoThread::mergeLongMo(MoSession* pSession)
{
    if(NULL == pSession)
    {
        LogError("param is null!!");
        return false;
    }
    bool iSuccess = false;
    string strLongContent = "";
    for(UInt32 iPkNumber = 1; iPkNumber <= pSession->m_uPkTotal; iPkNumber++)
    {
        iSuccess = false;
        UInt32 uInfoSize = pSession->m_vecLongMoInfo.size();
        for (UInt32 i = 0; i < uInfoSize;i++)
        {
            if (iPkNumber == pSession->m_vecLongMoInfo.at(i).m_uPkNumber)
            {
                strLongContent.append(pSession->m_vecLongMoInfo.at(i).m_strShortContent);
                iSuccess = true;
                break;
            }
            
        }

        if (false == iSuccess)
        {
            break;
        }

    }
    
    pSession->m_strContent.assign(strLongContent);
    return iSuccess;
}

// 获取账户状态 1：注册完成，5：冻结，6：注销，7：锁定
int CDirectMoThread::GetAcctStatus(const string& strClientId)
{
    int nRet = -1;
    std::map<string, SmsAccount>::iterator itAcct = m_accountMap.find(strClientId);
    if (itAcct != m_accountMap.end())
    {
        nRet = itAcct->second.m_uStatus;
    }   
    return nRet;
}
