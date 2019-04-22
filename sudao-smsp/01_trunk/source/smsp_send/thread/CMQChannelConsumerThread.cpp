#include "CMQChannelConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "base64.h"
#include "HttpParams.h"

using namespace std;

CMQChannelConsumerThread::CMQChannelConsumerThread(const char *name):CMQConsumerThread(name)
{

}

CMQChannelConsumerThread::~CMQChannelConsumerThread()
{

}

bool CMQChannelConsumerThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if(false == CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
        LogWarn("CMQConsumerThread::Init fail");
        return false;
    }
    
    if (false == InitChannels())
    {
        LogError("InitChannels failed, ComponentId[%d]", g_uComponentId);
        return false;
    }

    return true;
}

bool CMQChannelConsumerThread::InitChannels()
{
    map<int, models::Channel> mapChannel;
    g_pRuleLoadThread->getChannlelMap(mapChannel);
    m_mapChannelInfo.clear();
    m_mapChannelSpeedCtrl.clear();
    for(map<int, models::Channel>::iterator itor = mapChannel.begin(); itor != mapChannel.end(); itor++)
    {
        if(g_uComponentId == itor->second.m_uSendNodeId)
        {
            /*保存当前组件的通道*/
            m_mapChannelInfo.insert(make_pair(itor->first, itor->second));
            LogNotice("Component[%d] Add Channel[%d] Success", g_uComponentId, itor->first);

            /*当前组件计速，保存当前通道队列的最大速度和队列名称*/
            ChannelSpeedCtrl stuChannelSpeedCtrl;
            stuChannelSpeedCtrl.uChannelSpeed = itor->second.m_uSpeed;
            map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(itor->second.m_uMqId);
            if(itrMq != m_mapMqConfig.end())
            {
                stuChannelSpeedCtrl.strQueue        = itrMq->second.m_strQueue;
                stuChannelSpeedCtrl.uMqId           = itor->second.m_uMqId;
                stuChannelSpeedCtrl.uShowsigntype   = itor->second._showsigntype;
                stuChannelSpeedCtrl.uLongsms        = itor->second.longsms;
                stuChannelSpeedCtrl.uChannelType    = itor->second.m_uChannelType;
                stuChannelSpeedCtrl.uExValue        = itor->second.m_uExValue;          
                stuChannelSpeedCtrl.uSplitRule      = itor->second.m_uSplitRule;
                stuChannelSpeedCtrl.uCnLen          = itor->second.m_uCnLen;
                stuChannelSpeedCtrl.uEnLen          = itor->second.m_uEnLen;
                stuChannelSpeedCtrl.uCnSplitLen     = itor->second.m_uCnSplitLen;
                stuChannelSpeedCtrl.uEnSplitLen     = itor->second.m_uEnSplitLen;
                SetChannelSpeedCtrl(itor->first, stuChannelSpeedCtrl);
            }
            else
            {
                LogError("m_mapMqConfig can not find MqId[%lu]", itor->second.m_uMqId);
                return false;
            }

            /*打开通道*/
            if(false == amqpBasicConsume(itor->first, itor->second.m_uMqId))
            {
                LogError("ComponentId[%d] amqp_channel_open Channel[%u] Failed", g_uComponentId, itor->first);
                return false;
            }
            else
            {
                LogNotice("ComponentId[%d] amqp_channel_open Channel[%u] Success", g_uComponentId, itor->first);
            }
        }
    }

    LogNotice("Init Channel Over! m_mapChannelInfo size[%d], m_mapChannelSpeedCtrl size[%d]", 
        m_mapChannelInfo.size(), m_mapChannelSpeedCtrl.size());
    
    return true;
}

void CMQChannelConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(true == CMQConsumerThread::HandleCommonMsg(pMsg))
    {
        return;
    }

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            HandleChannelUpdateReq(pMsg);
            break;
        }
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
}

void CMQChannelConsumerThread::HandleChannelUpdateReq(TMsg * pMsg)
{
    TUpdateChannelReq* pChannelMap = (TUpdateChannelReq*)pMsg;
    
    map<int, models::Channel> mapChannel = pChannelMap->m_ChannelMap;  

    //过滤出当前send组件的所有通道 ---> m_mapChannelInfo
    m_mapChannelInfo.clear();
    for(map<int, models::Channel>::iterator itor = mapChannel.begin(); itor != mapChannel.end(); itor++)
    {
        if(g_uComponentId == itor->second.m_uSendNodeId)
        {
            m_mapChannelInfo.insert(make_pair(itor->first, itor->second));
        }
    }

    //发命令给rabbitmq关闭web页面关闭了的通道
    for(map<int, ChannelSpeedCtrl>::iterator it1 = m_mapChannelSpeedCtrl.begin(); it1 != m_mapChannelSpeedCtrl.end(); it1++)
    {
        map<int, models::Channel>::iterator it2 = m_mapChannelInfo.find(it1->first);
        if(it2 == m_mapChannelInfo.end()
                || it2->second.m_uMqId != it1->second.uMqId)
        {
            amqp_rpc_reply_t result = amqp_channel_close(m_mqConnectionState, it1->first, AMQP_REPLY_SUCCESS);

            if(result.reply_type != AMQP_RESPONSE_NORMAL)
            {
                LogWarn("Close Channel[%u] failed", it1->first);
            }
            else
            {
                LogNotice("amqp_channel_close[%u]", it1->first);
            }
        }
    }

    //打开页面新增的通道
    for(map<int, models::Channel>::iterator it1 = m_mapChannelInfo.begin(); it1 != m_mapChannelInfo.end(); it1++)
    {
        map<int, ChannelSpeedCtrl>::iterator it2 = m_mapChannelSpeedCtrl.find(it1->first);
        if(it2 == m_mapChannelSpeedCtrl.end()
                || it1->second.m_uMqId != it2->second.uMqId)
        {
            if(false == amqpBasicConsume(it1->first, it1->second.m_uMqId))
            {
                LogWarn("Add Channel[%u] failed!", it1->first);
            }
            else
            {
                LogNotice("amqp_channel_open[%u]", it1->first);
            }
        }
    }

    //更新m_mapChannelSpeedCtrl
    m_mapChannelSpeedCtrl.clear();
    for(map<int, models::Channel>::iterator it1 = m_mapChannelInfo.begin(); it1 != m_mapChannelInfo.end(); it1++)
    {
        ChannelSpeedCtrl stuChannelSpeed;
        stuChannelSpeed.uChannelSpeed = it1->second.m_uSpeed;

        map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(it1->second.m_uMqId);
            
        if(itrMq != m_mapMqConfig.end())
        {
            stuChannelSpeed.strQueue = itrMq->second.m_strQueue;
            stuChannelSpeed.uMqId = it1->second.m_uMqId;
            stuChannelSpeed.uShowsigntype   = it1->second._showsigntype;
            stuChannelSpeed.uLongsms        = it1->second.longsms;
            stuChannelSpeed.uChannelType    = it1->second.m_uChannelType;
            stuChannelSpeed.uExValue        = it1->second.m_uExValue;           
            stuChannelSpeed.uSplitRule      = it1->second.m_uSplitRule;
            stuChannelSpeed.uCnLen          = it1->second.m_uCnLen;
            stuChannelSpeed.uEnLen          = it1->second.m_uEnLen;
            stuChannelSpeed.uCnSplitLen     = it1->second.m_uCnSplitLen;
            stuChannelSpeed.uEnSplitLen     = it1->second.m_uEnSplitLen;
            SetChannelSpeedCtrl(it1->first, stuChannelSpeed);
        }
        else
        {
            LogError("m_mapMqConfig can not find MqId[%lu]", it1->second.m_uMqId);
        }
    }
    
    LogNotice("Component[%d] Update m_mapChannelSpeedCtrl,m_mapChannelInfo size[%u, %u]", g_uComponentId
        , m_mapChannelSpeedCtrl.size(), m_mapChannelInfo.size());
}

int CMQChannelConsumerThread::GetMemoryMqMessageSize()
{
    //send组件有滑窗控制
    return 0;
}

void CMQChannelConsumerThread::processMessage(string& strData)
{
    web::HttpParams param;
    param.Parse(strData);

    LogElk("==send== get one channel mq message data:%s.",strData.data());

    TMsg* msg = new TMsg();
    msg->m_iMsgType = MSGTYPE_CHANNEL_SEND_REQ;
    msg->m_strSessionID = strData; //fu yong
    g_CChannelThreadPool->PostMsg(msg);

    string strClientId = param._map["clientId"];
    string strSign = Base64::Decode(param._map["sign"]);
    string strContent = Base64::Decode(param._map["content"]);
    UInt32 uChannelId = atoi(param._map["channelid"].data());
    string strSmsId = param._map["smsId"];
    string strPhone = param._map["phone"];
    map<int, models::Channel>::iterator itrChann = m_mapChannelInfo.find(uChannelId);
    if(itrChann == m_mapChannelInfo.end())
    {
        LogFatal("[%s,%s:%s:%d] Channel not found in m_mapChannelInfo", strClientId.c_str(), strSmsId.c_str()
            , strPhone.c_str(), uChannelId);
        return;
    }
    
    bool isIncludeChinese = Comm::IncludeChinese((char*)(strSign + strContent).data());
    string strSignTemp = "";
    string strContentTemp = strContent;
    string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
    string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );
    if ( false == strSign.empty() )
    {
        strSignTemp = strLeft + strSign + strRight;
    }
    map<int, ChannelSpeedCtrl>::iterator itor = m_mapChannelSpeedCtrl.find( uChannelId );
    int maxWord = 70;
    int splitWord = 67;
    UInt32 msgLength = 0;
    if(itor != m_mapChannelSpeedCtrl.end())
    {
        if (itor->second.uShowsigntype != 3
            || ((itor->second.uExValue & 0x10) && itor->second.uShowsigntype == 3 && itor->second.uSplitRule == 1) )
        {
            strContentTemp.append(strSignTemp);
        }
        if (itor->second.uExValue & 0x10)
        {
            if(!isIncludeChinese)
            {
                maxWord = itor->second.uEnLen;
                splitWord = itor->second.uEnSplitLen;
            }
            else
            {
                maxWord = itor->second.uCnSplitLen;
                splitWord = itor->second.uCnSplitLen;
            }
            
        }
        utils::UTFString utfHelper;
        msgLength = utfHelper.getUtf8Length(strContentTemp);

    }
    else
    {
        LogFatal("[%s:%s:%s:%u]uChannelId is not found", strClientId.c_str(), strSmsId.c_str()
            , strPhone.c_str(), uChannelId);
        return;
    }

    if((itrChann->second.httpmode == YD_CMPP) || (itrChann->second.httpmode == DX_SMGP) || (itrChann->second.httpmode == LT_SGIP) 
        || (itrChann->second.httpmode == GJ_SMPP) || (itrChann->second.httpmode == YD_CMPP3))
    {
        ////direct channel
    
        if (!(itor->second.uExValue & 0x10))
        {
            if(!isIncludeChinese)
            {
                maxWord = 140;
            }
        }

        if(msgLength > (UInt32)maxWord) //70    ||140
        {
            UInt32 cnt = msgLength/splitWord;   //67        
            if(msgLength*1.0/splitWord == cnt)  //67    
            {
                itor->second.uCurrentSpeed = itor->second.uCurrentSpeed + cnt - 1;
            }
            else
            {
                itor->second.uCurrentSpeed += cnt;
            }
        }
    
    
    }
    else
    {
        ////http channel
                    
        if (!itor->second.uLongsms)
        {
            if(msgLength > (UInt32)maxWord)
            {
                UInt32 cnt = msgLength/splitWord;
                if(msgLength*1.0/splitWord == cnt)
                {
                    itor->second.uCurrentSpeed = itor->second.uCurrentSpeed + cnt - 1;
                }
                else
                {
                    itor->second.uCurrentSpeed += cnt;
                }
            }
        }
                    
    }
}
