#include "CMQDBConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include "CEmojiString.h"
#include "Comm.h"
#include "ErrorCode.h"

using namespace std;

const string CONST_STR_RABBIT_MQ_FLAG = "RabbitMQFlag=";
const string CONST_STR_STATE = "state";
const string CONST_STR_SEMICOLON = ";";

const string::size_type CONST_STR_STATE_LEN = CONST_STR_STATE.length();


CMQDBConsumerThread::CMQDBConsumerThread(const char *name, UInt8 uConsumerdatabase):CMQAckConsumerThread(name)
{
    // DB_SWITCH default: 2
    m_uDBSwitch = 2;
	m_uifWriteBigData = 0;
    m_ucConsumerFlag = uConsumerdatabase;
}

CMQDBConsumerThread::~CMQDBConsumerThread()
{
}

bool CMQDBConsumerThread::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
{
    if(false == CMQAckConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
        LogError("CMQNoAckConsumerThread::Init() fail");
        return false;
    }

    if(false == InitChannels())
    {
        return false;
    }

    //m_uDBSwitch
    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    return true;
}

void CMQDBConsumerThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "DB_SWITCH";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

		size_t pos = strTmp.find("|");
		int dbSwitchTemp = 0;
		int iFWriteBigData = 0;
		if (pos != string::npos)
		{
			dbSwitchTemp = atoi(strTmp.substr(0,pos).data());
			iFWriteBigData = atoi(strTmp.substr(pos + 1).data());
		}


        if ((0 != dbSwitchTemp) && (1 != dbSwitchTemp) && (2 != dbSwitchTemp))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), dbSwitchTemp);
            break;
        }
		if ((0 != iFWriteBigData) && (1 != iFWriteBigData))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), iFWriteBigData);
            break;
        }

		m_uifWriteBigData = iFWriteBigData;
        m_uDBSwitch = dbSwitchTemp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u,%u).", strSysPara.c_str(), m_uDBSwitch, m_uifWriteBigData);
}

bool CMQDBConsumerThread::AllChannelConsumerPause()
{
    if(true == m_bConsumerSwitch)
    {
        LogWarn("m_bConsumerSwitch is ON. Can't pause.");
        return false;
    }

    map<string,ComponentRefMq>::iterator itr = m_componentRefMqInfo.begin();
    // cancel consume
    bool bRet = true;

    for(; itr != m_componentRefMqInfo.end(); ++itr)
    {
        std::string strChannelId = Comm::int2str(itr->second.m_uId);
        amqp_basic_cancel(m_mqConnectionState, itr->second.m_uId, amqp_cstring_bytes(strChannelId.c_str()));
        amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
        if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
        {
            LogNotice("ChannelId[%d] Mqid[%lu] pause consume failed. reply_type:%d", itr->second.m_uId, itr->second.m_uMqId, rConsume.reply_type);
            bRet = false;
            continue;
        }
        else
        {
            LogNotice("ChannelId[%d] Mqid[%lu] pause consume success.", itr->second.m_uId, itr->second.m_uMqId);
        }
    }
    return bRet;
}

bool CMQDBConsumerThread::AllChannelConsumerResume()
{
    if(false == m_bConsumerSwitch)
    {
        LogWarn("m_bConsumerSwitch is OFF. Can't resume.");
        return false;
    }

    bool bRet = true;
    map<string,ComponentRefMq>::iterator itr;
    for(itr = m_componentRefMqInfo.begin(); itr != m_componentRefMqInfo.end(); ++itr)
    {
        if (false == amqpBasicConsume(itr->second.m_uId, itr->second.m_uMqId))
        {
            LogError("ChannelId[%d] Mqid[%lu] resume consume failed!!!", itr->second.m_uId, itr->second.m_uMqId);
            bRet = false;
            continue;
        }
        else
        {
            LogNotice("ChannelId[%d] Mqid[%lu] Consume resume succ.", itr->second.m_uId, itr->second.m_uMqId);
        }
    }

    return bRet;
}

bool CMQDBConsumerThread::UpdateMqConsumer(map <string,ComponentRefMq>& mapComponentRefMq)
{
    map<string,ComponentRefMq>::iterator itold = m_componentRefMqInfo.begin();
    for(; itold != m_componentRefMqInfo.end(); )
    {
        map <string,ComponentRefMq>::iterator itdel = mapComponentRefMq.find(itold->first);
        if(itdel == mapComponentRefMq.end())//mq关系解除
        {
            LogNotice("ChannelId[%d] Mqid[%lu] close Consume...", itold->second.m_uId, itold->second.m_uMqId);
            //先取消订阅
            amqp_channel_close(m_mqConnectionState, itold->second.m_uId, AMQP_REPLY_SUCCESS);
            m_componentRefMqInfo.erase(itold++);
            map<UInt32, ChannelSpeedCtrl>::iterator it = m_mapChannelSpeedCtrl.find(itold->second.m_uId);
            if(it != m_mapChannelSpeedCtrl.end())
                m_mapChannelSpeedCtrl.erase(it);
        }
        else
        {
            itold++;
        }
    }

    map <string,ComponentRefMq>::iterator itor = mapComponentRefMq.begin();
    for(; itor != mapComponentRefMq.end(); itor++)
    {
        if((g_uComponentId == itor->second.m_uComponentId) &&
            (MESSAGE_TYPE_DB == itor->second.m_strMessageType) &&
            (CONSUMER_MODE == itor->second.m_uMode))
        {
            map <UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(itor->second.m_uMqId);
            if (itrMq == m_mapMqConfig.end())
            {
                LogError("MqId:%lu is not find in m_mapMqConfig", itor->second.m_uMqId);
                continue;
            }

            //update channel speed
            ChannelSpeedCtrl stuChannelSpeedCtrl;
            stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
            stuChannelSpeedCtrl.uChannelSpeed = itor->second.m_uGetRate;
            stuChannelSpeedCtrl.uMqId = itor->second.m_uMqId;
            SetChannelSpeedCtrl(itor->second.m_uId, stuChannelSpeedCtrl);

            map <string,ComponentRefMq>::iterator itr = m_componentRefMqInfo.find(itor->first);
            if(itr == m_componentRefMqInfo.end())
            {//add new mq consmer
                if((true == m_bConsumerSwitch) && (false == amqpBasicChannelOpen(itor->second.m_uId, itor->second.m_uMqId)))
                {
                    LogError("add newmq[%lu] open failed",itor->second.m_uMqId);
                    return false;
                }
                if((true == m_bConsumerSwitch) && (false == amqpBasicConsume(itor->second.m_uId, itor->second.m_uMqId)))
                {
                    LogError("add newmq[%lu] amqpBasicConsume failed",itor->second.m_uMqId);
                    return false;
                }
                LogNotice("Add new mq[%u] consumer success!!",itor->second.m_uMqId);
                m_componentRefMqInfo[itor->first] = itor->second;
            }
        }
    }
    LogNotice("Init mqqueue Over! m_componentRefMqInfo size[%d], m_mapChannelSpeedCtrl size[%d]",
                m_componentRefMqInfo.size(), m_mapChannelSpeedCtrl.size());
    return true;
}
bool CMQDBConsumerThread::InitChannels()
{
    m_componentRefMqInfo.clear();
    m_mapChannelSpeedCtrl.clear();
    m_iSumChannelSpeed = 0;
    map <string,ComponentRefMq> tempRefMqInfo;
    tempRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(tempRefMqInfo);

    return UpdateMqConsumer(tempRefMqInfo);
}

void CMQDBConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(true == CMQAckConsumerThread::HandleCommonMsg(pMsg))
    {
        return;
    }

    switch(pMsg->m_iMsgType)
    {
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
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            HandleComponentRefMqRep(pMsg);
            break;
        }
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
}

void CMQDBConsumerThread::HandleComponentRefMqRep(TMsg * pMsg)
{
    TUpdateComponentRefMqReq* pMqCon = (TUpdateComponentRefMqReq*)pMsg;
    //m_mapChannelSpeedCtrl.clear();
    m_iSumChannelSpeed = 0;
    map <string,ComponentRefMq>& tempRefMqInfo = pMqCon->m_mapComponentRefMq;

    UpdateMqConsumer(tempRefMqInfo);
}

int CMQDBConsumerThread::GetMemoryMqMessageSize()
{
    int iDBQueueSize    = g_pDBThreadPool->GetMsgSize();
    int iFileQueueSize  = g_pSqlFileThread->GetMsgSize();

    return iDBQueueSize > iFileQueueSize ? iDBQueueSize : iFileQueueSize;
}

void CMQDBConsumerThread::processMessage(string& strData)
{
    LogNotice("==MQDB== strData[%s], DBSwitch[%u].",
        strData.data(), m_uDBSwitch);

    SessionInfo sessionInfo;

    if (!parseMsg(strData, sessionInfo))
    {
        LogError("Call parseMsg failed.");
        return;
    }

    if (!parseMsgForOperFlag(sessionInfo))
    {
        LogError("Call parseMsgForOperFlag failed.");
        return;
    }

    parseMsgForTableName(sessionInfo);
    parseMsgForState(sessionInfo);
    modifySql(sessionInfo);
    fillEmojiString(sessionInfo.strSrcSql_);

    LogDebug("Uid(%s), Sql(%s)."
        "OperFlag(%u), TableFlag(%u), ReQueueTimes(%u), ReQueueTime(%u), "
        "State(%u), bModify(%d), SqlModifyWhere(%s), SqlModifySet(%s).",
        sessionInfo.strId_.data(),
        sessionInfo.strSrcSql_.data(),
        sessionInfo.eOperFlag_,
        sessionInfo.eTableFlag_,
        sessionInfo.usReQueueTimes_,
        sessionInfo.uiReQueueTime_,
        sessionInfo.ucState_,
        sessionInfo.bModify_,
        sessionInfo.strSqlModifyWhere_.data(),
        sessionInfo.strSqlModifySet_.data());

    if ((2 == m_uDBSwitch)
      && (OperFlag_Update == sessionInfo.eOperFlag_)
      && (TableFlag_Invalid != sessionInfo.eTableFlag_))
    {
        forwardMsgEx(sessionInfo);
    }
    else
    {
        forwardMsg(sessionInfo);
    }
}

bool CMQDBConsumerThread::parseMsg(const string& strData, SessionInfo& sessionInfo)
{
    string::size_type pos = strData.find(CONST_STR_RABBIT_MQ_FLAG);
    if (string::npos == pos)
    {
        LogError("Can not find %s in message(%s).",
            CONST_STR_RABBIT_MQ_FLAG.data(),
            strData.data());
        return false;
    }

    sessionInfo.strSrcSql_.assign(strData, 0, pos);

    // RabbitMQFlag=ac6bb2db-c32a-41a5-8cc5-1b4215888818
    static string::size_type lenFlag = CONST_STR_RABBIT_MQ_FLAG.length();
    string::size_type posSemicolon = strData.find(CONST_STR_SEMICOLON, pos);
    if (string::npos == posSemicolon)
    {
        sessionInfo.strId_ = strData.substr(pos + lenFlag);
        return true;
    }

    sessionInfo.strId_.assign(strData, pos + lenFlag, posSemicolon - pos - lenFlag);

    // RabbitMQFlag=ac6bb2db-c32a-41a5-8cc5-1b4215888818;ReQueueTimes=1;ReQueueTime=1513149938
    static string::size_type lenSemicolon = CONST_STR_SEMICOLON.length();
    string strTmp = strData.substr(posSemicolon + lenSemicolon);

    vector<string> vecTmp;
    Comm::split(strTmp, ";", vecTmp);

    vector<string> vecTmp2;
    for (string::size_type i = 0; i < vecTmp.size(); ++i)
    {
        Comm::split(vecTmp[i], "=", vecTmp2);
    }

    if (4 == vecTmp2.size())
    {
        sessionInfo.usReQueueTimes_ = atoi(vecTmp2[1].data());
        sessionInfo.uiReQueueTime_ = atoi(vecTmp2[3].data());
    }
    else
    {
        LogError("Invalid strTmp(%s) in strData(%s).",
            strTmp.data(), strData.data());

        return false;
    }

    return true;
}

bool CMQDBConsumerThread::parseMsgForOperFlag(SessionInfo& sessionInfo)
{
    string strOperFlag = sessionInfo.strSrcSql_.substr(0, 6);

    if (("insert" == strOperFlag) || ("INSERT" == strOperFlag))
    {
        sessionInfo.eOperFlag_ = OperFlag_Insert;
    }
    else if (("UPDATE" == strOperFlag) || ("update" == strOperFlag))
    {
        sessionInfo.eOperFlag_ = OperFlag_Update;
    }
    else
    {
        LogError("The operation flag(%s) is neither insert nor update.",
            strOperFlag.data());

        return false;
    }

    return true;
}

void CMQDBConsumerThread::parseMsgForTableName(SessionInfo& sessionInfo)
{
    // t_sms_access_x_xxxxxxxx
    string strTmp = "t_sms_access_";
    string::size_type pos = sessionInfo.strSrcSql_.find(strTmp);
    if (string::npos != pos)
    {
        string::size_type pos1 = pos + strTmp.length();

        if (isdigit(sessionInfo.strSrcSql_[pos1])
			&& (isdigit(sessionInfo.strSrcSql_[pos1 + 1]) || '_' == sessionInfo.strSrcSql_[pos1 + 1]))
        {
            sessionInfo.eTableFlag_ = TableFlag_Access;
            return;
        }
    }

    // t_sms_record_x_xxxxxxxx
    strTmp = "t_sms_record_";
    pos = sessionInfo.strSrcSql_.find(strTmp);
    if (string::npos != pos)
    {
        string::size_type pos1 = pos + strTmp.length();
        if (isdigit(sessionInfo.strSrcSql_[pos1])
			&& (isdigit(sessionInfo.strSrcSql_[pos1 + 1]) || '_' == sessionInfo.strSrcSql_[pos1 + 1]))
        {
            sessionInfo.eTableFlag_ = TableFlag_Record;
            return;
        }
    }
}

void CMQDBConsumerThread::parseMsgForState(SessionInfo& sessionInfo)
{
    if ((TableFlag_Access != sessionInfo.eTableFlag_)
      || (OperFlag_Update != sessionInfo.eOperFlag_))
    {
        return;
    }

    // [state=1]
    // [state='1']
    // [state = 1]
    // [state = '1']
    // [state = '1']
    // [audit_state='1']
    // [state_xx, state = 1]
    // ...
    const string& strSrcSql = sessionInfo.strSrcSql_;
    string::size_type posStart = 0;

    while(1)
    {
        int result = parseMsgForStateEx(strSrcSql, posStart);

        if (0 == result)
        {
            break;
        }
        else if (-1 == result)
        {
            return;
        }

        // -2 == result
    }

    string strState;

    for (string::size_type i = posStart + CONST_STR_STATE_LEN; i < strSrcSql.length(); ++i)
    {
        if (',' == strSrcSql[i])
        {
            break;
        }

        if (isdigit(static_cast<unsigned char>(strSrcSql[i])))
        {
            strState += strSrcSql[i];
        }
    }

    int iState = atoi(strState.c_str());
    if ((0 > iState) || (10 < iState))
    {
        return;
    }

    sessionInfo.ucState_ = iState;
    sessionInfo.uiStatePos_ = posStart;
}

int CMQDBConsumerThread::parseMsgForStateEx(const string& strSql, string::size_type& posStart)
{
    string::size_type lenSql = strSql.length();
    string::size_type pos1 = strSql.find(CONST_STR_STATE, posStart);

    if (string::npos == pos1)
    {
        LogDebug("Can not find state.");
        return -1;
    }

    string::size_type posBefore = pos1 - 1;
    if (posBefore >= 0 )
    {
        const char c = strSql[posBefore];
        if ((' ' != c) && (',' != c))
        {
            LogDebug("The character before the state is not expected.");
            posStart = pos1 + CONST_STR_STATE_LEN;
            return -2;
        }
    }

    string::size_type posAfter = pos1 + CONST_STR_STATE_LEN;
    if (posAfter < lenSql)
    {
        const char c = strSql[posAfter];
        if ((' ' != c) && ('=' != c))
        {
            LogDebug("The character after the state is not expected.");
            posStart = pos1 + CONST_STR_STATE_LEN;
            return -2;
        }
    }

    posStart = pos1 ;
    LogDebug("Find state");

    return 0;
}

void CMQDBConsumerThread::modifySql(SessionInfo& sessionInfo)
{
    if ((TableFlag_Access != sessionInfo.eTableFlag_)
      || (OperFlag_Update != sessionInfo.eOperFlag_)
      || (SMS_STATUS_SUBMIT_SUCCESS != sessionInfo.ucState_))
    {
        return;
    }

    {
        string::size_type len = sessionInfo.strSrcSql_.length();
        if (';' == sessionInfo.strSrcSql_[len - 1])
        {
            sessionInfo.strSqlModifyWhere_.assign(sessionInfo.strSrcSql_, 0, len - 1);
        }
        else
        {
            sessionInfo.strSqlModifyWhere_.assign(sessionInfo.strSrcSql_);
        }

        sessionInfo.strSqlModifyWhere_.append(" and state NOT IN (3, 6);");
        LogDebug("strSqlModifyWhere(%s).", sessionInfo.strSqlModifyWhere_.c_str());
    }

    {
        sessionInfo.strSqlModifySet_ = sessionInfo.strSrcSql_;
        string::size_type posStart = sessionInfo.uiStatePos_;

        if (0 == posStart)
        {
            return;
        }

        string::size_type posEnd = sessionInfo.strSqlModifySet_.find(",", posStart);
        if (string::npos == posEnd)
        {
            return;
        }

        sessionInfo.strSqlModifySet_.erase(posStart, posEnd - posStart + 1);
        LogDebug("strSqlModifySet(%s).", sessionInfo.strSqlModifySet_.c_str());
    }

    sessionInfo.bModify_ = true;
}

void CMQDBConsumerThread::forwardMsg(const SessionInfo& sessionInfo)
{
    CHK_NULL_RETURN(g_pDBThreadPool);
    CHK_NULL_RETURN(g_pSqlFileThread);
	CHK_NULL_RETURN(g_pBigDataThread);
	//m_uifWriteBigData

    const string& strSql = sessionInfo.strSrcSql_;
    const string& strUid = sessionInfo.strId_;

    if (2 == m_uDBSwitch) /////db switch on
    {
        TDBQueryReq* pDb = new TDBQueryReq();
        pDb->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
        pDb->m_SQL.assign(strSql);
        pDb->m_strKey.assign(strUid);
        g_pDBThreadPool->PostMsg(pDb);
    }
    else if (1 == m_uDBSwitch)
    {
        if (OperFlag_Insert == sessionInfo.eOperFlag_)
        {
            TDBQueryReq* pInsert = new TDBQueryReq();
            pInsert->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
            pInsert->m_SQL.assign(strSql);
            pInsert->m_strKey.assign(strUid);
            g_pDBThreadPool->PostMsg(pInsert);
        }
        else
        {
            TSqlFileReq* pSql = new TSqlFileReq();
            pSql->m_iMsgType = MSGTYPE_LOG_REQ;
            pSql->m_strSqlContent.assign(strSql);
            g_pSqlFileThread->PostMsg(pSql);
        }
    }
    else if (0 == m_uDBSwitch)
    {
        TSqlFileReq* pSql = new TSqlFileReq();
        pSql->m_iMsgType = MSGTYPE_LOG_REQ;
        pSql->m_strSqlContent.assign(strSql);
        g_pSqlFileThread->PostMsg(pSql);
    }
    else
    {
        TSqlFileReq* pSql = new TSqlFileReq();
        pSql->m_iMsgType = MSGTYPE_LOG_REQ;
        pSql->m_strSqlContent.assign(strSql);
        g_pSqlFileThread->PostMsg(pSql);
        LogError("======error======m_uDBSwitch[%d] is invalid.",m_uDBSwitch);
    }
}

void CMQDBConsumerThread::forwardMsgEx(const SessionInfo& sessionInfo)
{
    CHK_NULL_RETURN(g_pDBThreadPool);

    DBInReq* pDb = new DBInReq();
    pDb->m_iMsgType = MSGTYPE_DB_IN_REQ;
    pDb->sessionInfo_ = sessionInfo;
    g_pDBThreadPool->PostMsg(pDb);
}

void CMQDBConsumerThread::fillEmojiString(string& strSql)
{
    char dstSql[4096];
    memset(dstSql, 0, sizeof(dstSql));

    if(CEmojiString::fillEmojiString(strSql.data(), dstSql))
    {
        strSql.assign(dstSql);
    }
    else
    {
        LogError("Call CEmojiString::fillEmojiString failed. srcSql[%s]",
            strSql.data());
    }
}

