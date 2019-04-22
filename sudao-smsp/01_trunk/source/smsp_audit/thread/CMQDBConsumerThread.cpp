#include "CMQDBConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "HttpParams.h"
#include "Channellib.h"
#include "boost/foreach.hpp"
#include "Comm.h"

using namespace std;

#define foreach BOOST_FOREACH


CMQDBConsumerThread::CMQDBConsumerThread(const char *name):CMQConsumerThread(name)
{
}

CMQDBConsumerThread::~CMQDBConsumerThread()
{
}

bool CMQDBConsumerThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if (!CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
        LogError("Call CMQConsumerThread::Init failed.");
        return false;
    }

    if (!InitChannels())
    {
        LogError("Call InitChannels failed.");
        return false;
    }

    return true;
}

bool CMQDBConsumerThread::InitChannels()
{
    typedef map<string, ComponentRefMq> map_type;
    typedef const map_type::value_type map_value;

    map_type mapComponentRefMq;
    g_pRuleLoadThread->getComponentRefMq(mapComponentRefMq);

    m_mapChannelSpeedCtrl.clear();

    foreach(map_value& node, mapComponentRefMq)
    {
        if ((g_uComponentId != node.second.m_uComponentId)
         || (MESSAGE_AUDIT_CONTENT_REQ != node.second.m_strMessageType)
         || (CONSUMER_MODE != node.second.m_uMode))
        {
            continue;
        }

        LogDebug("id(%lu), mqId(%lu), rate(%u), weight(%u).",
            node.second.m_uId,
            node.second.m_uMqId,
            node.second.m_uGetRate,
            node.second.m_uWeight);

//        m_componentRefMqInfo =  itr->second;
//        m_uMqId = node.second.m_uMqId;


        m_mapComponentRefMq[node.first] = node.second;

        map<UInt64, MqConfig>::iterator itrMq;
        CHK_MAP_FIND_UINT_RET_FALSE(m_mapMqConfig, itrMq, node.second.m_uMqId);

        ChannelSpeedCtrl stuChannelSpeedCtrl;
        stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
        stuChannelSpeedCtrl.uChannelSpeed = node.second.m_uGetRate;
        stuChannelSpeedCtrl.uMqId = node.second.m_uMqId;

        SetChannelSpeedCtrl(node.second.m_uId, stuChannelSpeedCtrl);

        if (!amqpBasicConsume(node.second.m_uId, node.second.m_uMqId))
        {
            LogWarn("Call amqpBasicConsume failed.");
            return false;
        }
    }

    if (m_mapChannelSpeedCtrl.empty())
    {
        LogError("m_mapChannelSpeedCtrl is empty.");
        return false;
    }

    LogNotice("Init Channel Over! m_mapChannelSpeedCtrl size[%d]", m_mapChannelSpeedCtrl.size());
    return true;

}

//bool CMQDBConsumerThread::UpdateChannelSpeedCtrl(UInt64 uMqId)
//{
//    map<std::string,ComponentRefMq> mapComponentRefMqInfo;
//    mapComponentRefMqInfo.clear();
//    g_pRuleLoadThread->getComponentRefMq(mapComponentRefMqInfo);
//
//    char Key[32] = {0};
//    snprintf(Key, sizeof(Key), "%lu_%s_%u", g_uComponentId, MESSAGE_AUDIT_CONTENT_REQ.data(), CONSUMER_MODE);
//    string strKey;
//    strKey.append(Key);
//    map<string,ComponentRefMq>::iterator itor = mapComponentRefMqInfo.find(strKey);
//
//    if(itor == mapComponentRefMqInfo.end())
//    {
//        LogError("Can not find [component_id[%lu]__message_type[%s]__mode[%d]] in t_sms_component_ref_mq",
//            g_uComponentId, MESSAGE_AUDIT_CONTENT_REQ.data(), CONSUMER_MODE);
//        return false;
//    }
//    else
//    {
//        if(itor->second.m_uMqId != uMqId)
//        {
//            LogError("t_sms_component_configure MqId[%lu], t_sms_component_ref_mq MqId[%lu], not the same!",
//                uMqId, itor->second.m_uMqId);
//
//            return false;
//        }
//        else
//        {
//            m_componentRefMqInfo = itor->second;
//
//            m_mapChannelSpeedCtrl.clear();
//
//            ChannelSpeedCtrl stuChannelSpeedCtrl;
//            stuChannelSpeedCtrl.uChannelSpeed = m_componentRefMqInfo.m_uGetRate;
//
//            map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(uMqId);
//            if(itrMq != m_mapMqConfig.end())
//            {
//                stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
//                stuChannelSpeedCtrl.uMqId = uMqId;
//                SetChannelSpeedCtrl(itor->second.m_uId, stuChannelSpeedCtrl);
//                return true;
//            }
//            else
//            {
//                LogError("Can not find MqId[%lu] in m_mapMqConfig", uMqId);
//                return false;
//            }
//        }
//    }
//}

void CMQDBConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(CMQConsumerThread::HandleCommonMsg(pMsg))
    {
        return;
    }

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            handleComponentRefMqRep(pMsg);
            break;
        }
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
}

void CMQDBConsumerThread::handleComponentRefMqRep(TMsg * pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TUpdateComponentRefMqReq* pRefMq = (TUpdateComponentRefMqReq *)pMsg;

    ComponentRefMqMap mapComponentRefMq;
    typedef const ComponentRefMqMap::value_type map_value;

    foreach(map_value& node, pRefMq->m_componentRefMqInfo)
    {
        if ((g_uComponentId != node.second.m_uComponentId)
         || (MESSAGE_AUDIT_CONTENT_REQ != node.second.m_strMessageType)
         || (CONSUMER_MODE != node.second.m_uMode))
        {
            continue;
        }

        LogDebug("id(%lu), mqId(%lu), rate(%u), weight(%u).",
            node.second.m_uMqId,
            node.second.m_uGetRate,
            node.second.m_uId,
            node.second.m_uWeight);

        mapComponentRefMq[node.first] = node.second;
    }

    if (mapComponentRefMq.empty())
    {
        LogError("mapComponentRefMq is empty.");
        return;
    }

    LogNotice("update componentr ref mq. "
        "m_mapComponentRefMq.size(%u), m_mapChannelSpeedCtrl.size(%u), mapComponentRefMq.size(%u).",
        m_mapComponentRefMq.size(),
        m_mapChannelSpeedCtrl.size(),
        mapComponentRefMq.size());

    foreach(map_value& newObj, mapComponentRefMq)
    {
        map<int, ChannelSpeedCtrl>::iterator it2 = m_mapChannelSpeedCtrl.find(newObj.second.m_uId);

        // add
        if (it2 == m_mapChannelSpeedCtrl.end())
        {
            if (!amqpBasicConsume(newObj.second.m_uId, newObj.second.m_uMqId))
            {
                LogError("call amqpBasicConsume failed. id(%lu), mqid(%lu).",
                    newObj.second.m_uId,
                    newObj.second.m_uMqId);
            }
            else
            {
                LogError("amqpBasicConsume success. id(%lu), mqid(%lu).",
                    newObj.second.m_uId,
                    newObj.second.m_uMqId);
            }
        }
        else // delete
        {
            if (it2->second.uMqId != newObj.second.m_uMqId)
            {
                LogNotice("id(%lu), old mqid(%lu) change to new mqid(%lu).",
                    newObj.second.m_uId,
                    newObj.second.m_uMqId,
                    it2->second.uMqId);

                amqp_rpc_reply_t ret = amqp_channel_close(m_mqConnectionState, it2->second.uMqId, AMQP_REPLY_SUCCESS);

                if(ret.reply_type != AMQP_RESPONSE_NORMAL)
                {
                    LogError("Call amqp_channel_close failed. reply_type(%d), id(%lu), queue(%s).",
                        ret.reply_type,
                        it2->second.uMqId,
                        it2->second.strQueue.data());
                }
                else
                {
                    LogError("Call amqp_channel_close success. reply_type(%d), id(%lu), queue(%s).",
                        ret.reply_type,
                        it2->second.uMqId,
                        it2->second.strQueue.data());
                }

                if(!amqpBasicConsume(newObj.second.m_uId, newObj.second.m_uMqId))
                {
                    LogError("call amqpBasicConsume failed. id(%lu), mqid(%lu).",
                        newObj.second.m_uId,
                        newObj.second.m_uMqId);
                }
            }
        }
    }

    // delete
    foreach(map_value& old, m_mapComponentRefMq)
    {
        ComponentRefMqMap::iterator it2 = mapComponentRefMq.find(old.first);

        if (it2 == mapComponentRefMq.end())
        {
            amqp_rpc_reply_t ret = amqp_channel_close(m_mqConnectionState, old.second.m_uId, AMQP_REPLY_SUCCESS);

            if(ret.reply_type != AMQP_RESPONSE_NORMAL)
            {
                LogError("Call amqp_channel_close failed. reply_type(%d), id(%lu), mqid(%lu).",
                    ret.reply_type,
                    old.second.m_uId,
                    old.second.m_uMqId);
            }
            else
            {
                LogError("amqp_channel_close success. reply_type(%d), id(%lu), mqid(%lu).",
                    ret.reply_type,
                    old.second.m_uId,
                    old.second.m_uMqId);
            }
        }
    }

    m_mapChannelSpeedCtrl.clear();
    m_mapComponentRefMq.clear();
    m_mapComponentRefMq = mapComponentRefMq;

    foreach(map_value& newObj, m_mapComponentRefMq)
    {
        map<UInt64, MqConfig>::iterator itrMq;
        CHK_MAP_FIND_UINT_CONTINUE(m_mapMqConfig, itrMq, newObj.second.m_uMqId);

        ChannelSpeedCtrl stuChannelSpeedCtrl;
        stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
        stuChannelSpeedCtrl.uChannelSpeed = newObj.second.m_uGetRate;
        stuChannelSpeedCtrl.uMqId = newObj.second.m_uMqId;

        SetChannelSpeedCtrl(newObj.second.m_uId, stuChannelSpeedCtrl);
    }
}

int CMQDBConsumerThread::GetMemoryMqMessageSize()
{
    if (NULL == g_pAuditServiceThread)
    {
        LogError("g_pAuditServiceThread is NULL.");
        return 0;
    }

    return g_pAuditServiceThread->GetMsgSize();
}

void CMQDBConsumerThread::processMessage(string& strData)
{
    LogNotice("=== get one audit content req msg === %s.", strData.data());

    web::HttpParams param;
    param.Parse(strData);

    MqC2sDbAuditContentReqQueMsg* pMsg = new MqC2sDbAuditContentReqQueMsg();
    CHK_NULL_RETURN(pMsg);
    CHK_NULL_RETURN(g_pAuditServiceThread);

    // mq message field
    pMsg->m_iMsgType = MSGTYPE_AUDIT_SERVICE_REQ;
    pMsg->strClientId_ = param._map["clientid"];
    pMsg->strUserName_ = smsp::Channellib::decodeBase64(param._map["username"]);
    pMsg->strSmsId_ = param._map["smsid"];
    pMsg->strPhone_ = param._map["phone"];
    pMsg->strSign_ = smsp::Channellib::decodeBase64(param._map["sign"]);
    pMsg->strContent_ = smsp::Channellib::decodeBase64(param._map["content"]);
    pMsg->strAuditContent_ = param._map["auditcontent"];
    pMsg->ucSmsType_ = atoi(param._map["smstype"].data());
    pMsg->ucPayType_ = atoi(param._map["paytype"].data());
    pMsg->ucOperatorType_ = atoi(param._map["operatortype"].data());
    pMsg->strCsDate_ = param._map["csdate"];
    pMsg->uiGroupSendLimUserFlag_ = atoi(param._map["groupsendlim_userflag"].data());
    pMsg->uiGroupSendLimTime_ = atoi(param._map["groupsendlim_time"].data());
    pMsg->uiGroupSendLimNum_ = atoi(param._map["groupsendlim_num"].data());

    // other field
    pMsg->eMsgType_ = ((pMsg->uiGroupSendLimNum_ >= 1) && (pMsg->uiGroupSendLimNum_ <= 100000))
                        ? MqC2sDbAuditContentReqQueMsg::GroupSendLimMsg : MqC2sDbAuditContentReqQueMsg::CommonMsg;

    pMsg->converCsDate2Timesatmp();
    pMsg->getPhoneCount();
    pMsg->print();

    g_pAuditServiceThread->PostMsg(pMsg);
}




