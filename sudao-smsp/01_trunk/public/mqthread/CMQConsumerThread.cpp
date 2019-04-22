#include "CMQConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "base64.h"

using namespace std;

CMQConsumerThread::CMQConsumerThread(const char *name):CThread(name)
{
    m_bConsumerSwitch = false;
    m_mapChannelSpeedCtrl.clear();
    m_mapBlockedChannel.clear();
    m_iSumChannelSpeed = 0;
}

CMQConsumerThread::~CMQConsumerThread()
{
    amqp_connection_close(m_mqConnectionState, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(m_mqConnectionState);
}

bool CMQConsumerThread::close()
{
    amqp_connection_close(m_mqConnectionState, AMQP_REPLY_SUCCESS);

    int ret = amqp_destroy_connection(m_mqConnectionState);
    if(AMQP_STATUS_OK != ret)
    {
        LogWarn("reply_type:%d is amqp_destroy_connection failed.\n",ret);
        return false;
    }
    return true;
}

bool CMQConsumerThread::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
{
    if(false == CThread::Init())
    {
        LogError("CThread::Init fail");
        return false;
    }

    m_strIp.assign(strIp);
    m_uPort = uPort;
    m_strUserName.assign(strUserName);
    m_strPassWord.assign(strPassWord);

    if (false == RabbitMQConnect())
    {
        LogError("RabbitMQConnect is failed.");
        return false;
    }

    #ifdef RULELOAD_GET_MQ_CONFIG
    g_pRuleLoadThread->getMqConfig(m_mapMqConfig);
    #endif

    #ifdef RULELOAD_GET_COMPONENT_SWITCH
    m_bConsumerSwitch = g_pRuleLoadThread->getComponentSwitch(g_uComponentId);
    LogNotice("ComponentId[%d] m_bConsumerSwitch is %s", g_uComponentId, m_bConsumerSwitch ? "ON" : "OFF");
    #endif

    return true;
}

void CMQConsumerThread::SetChannelSpeedCtrl(int iChannelId, ChannelSpeedCtrl stuChannelSpeedCtrl)
{
    if(m_mapChannelSpeedCtrl.empty())
    {
        m_iSumChannelSpeed = 0;
    }

    m_iSumChannelSpeed += stuChannelSpeedCtrl.uChannelSpeed;

    m_mapChannelSpeedCtrl.insert(make_pair(iChannelId, stuChannelSpeedCtrl));

    LogNotice("ChannelId[%d] LimitSpeed[%d] SumChannelSpeed[%d] Queue[%s] MqId[%lu]",
        iChannelId, stuChannelSpeedCtrl.uChannelSpeed, m_iSumChannelSpeed, stuChannelSpeedCtrl.strQueue.data(), stuChannelSpeedCtrl.uMqId);
}

bool CMQConsumerThread::amqpBasicConsume(UInt32 uChannelId, UInt64 uMqId, UInt32 uPreFetchSize)
{
    map<UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(uMqId);
    if (itrMq == m_mapMqConfig.end())
    {
        LogError("mqid[%lu] not found in t_sms_mq_configure", uMqId);
        printf("[WARN] mqid[%lu] not found in t_sms_mq_configure\n", uMqId);
        return false;
    }

    amqp_channel_open(m_mqConnectionState, uChannelId);
    amqp_rpc_reply_t rOpenChannel =  amqp_get_rpc_reply(m_mqConnectionState);
    if (AMQP_RESPONSE_NORMAL != rOpenChannel.reply_type)
    {
        LogError("channelid:%d,reply_type:%d is amqp_channel_open failed.",uChannelId,rOpenChannel.reply_type);
        return false;
    }

    amqp_basic_qos(m_mqConnectionState, uChannelId, 0, uPreFetchSize, 0);

    amqp_rpc_reply_t rSetQos = amqp_get_rpc_reply(m_mqConnectionState);
    if (AMQP_RESPONSE_NORMAL != rSetQos.reply_type)
    {
        LogError("channelid:%d,reply_type:%d is amqp_basic_qos failed.",uChannelId,rSetQos.reply_type);
        return false;
    }

    char strChannelId[32];
    memset(strChannelId, 0, sizeof(strChannelId));
    snprintf(strChannelId, sizeof(strChannelId), "%d", uChannelId);
    amqp_basic_consume(m_mqConnectionState, uChannelId, amqp_cstring_bytes(itrMq->second.m_strQueue.data()), amqp_cstring_bytes(strChannelId),
        0, 0 , 0, amqp_empty_table);
    amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
    if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
    {
        LogError("channelid:%d,reply_type:%d is amqp_basic_consume failed.",uChannelId,rConsume.reply_type);
        return false;
    }

    return true;
}

void CMQConsumerThread::clearBlockedMsgAndResumeSlideWindowSize()
{
    for(map<UInt32, blockedChannelInfo>::iterator itor = m_mapBlockedChannel.begin(); itor != m_mapBlockedChannel.end(); )
    {
        UInt32 uChannelId = itor->first;

        LogWarn("m_bConsumerSwitch is OFF, Channel[%d] Release blocked msg", uChannelId);

        #ifdef SEND_COMPONENT_SLIDER_WINDOW
        // 恢复滑窗
        CChanelTxFlowCtr::channelFlowCtrCountAdd(uChannelId, -1, 1);

        #endif
        m_mapBlockedChannel.erase(itor++);
    }
}

void CMQConsumerThread::amqpCloseAllChannels()
{
    bool bCloseAllChannelOK = true;

    LogDebug("speed size:%u", m_mapChannelSpeedCtrl.size());
    for (map<int, ChannelSpeedCtrl>::iterator iter = m_mapChannelSpeedCtrl.begin(); iter != m_mapChannelSpeedCtrl.end(); iter++)
    {
        UInt32 uChannelId = iter->first;
        amqp_rpc_reply_t ret = amqp_channel_close(m_mqConnectionState, uChannelId, AMQP_REPLY_SUCCESS);
        if(ret.reply_type != AMQP_RESPONSE_NORMAL)
        {
            LogError("amqp_channel_close ChannelId[%u] FAIL, reply_type[%d][%s] ",
                      uChannelId, ret.reply_type, amqp_error_string2(ret.library_error));
            bCloseAllChannelOK = false;
        }
        else
        {
            LogNotice("amqp_channel_close ChannelId[%u] SUCC", uChannelId);
        }
    }

    //清除冻结的消息，防止稍后可能被发送出去造成重发
    clearBlockedMsgAndResumeSlideWindowSize();

    //存在关闭失败的通道
    if(!bCloseAllChannelOK)
    {
        close();
    }
}

bool CMQConsumerThread::RabbitMQConnect()
{
    amqp_socket_t* pSocket = NULL;
    int iStatus = 0;
    m_mqConnectionState = amqp_new_connection();
    pSocket = amqp_tcp_socket_new(m_mqConnectionState);

    if (NULL == pSocket)
    {
        LogWarn("amqp_tcp_socket_new is failed");
        return false;
    }

    iStatus = amqp_socket_open(pSocket, m_strIp.data(), m_uPort);
    if (iStatus < 0)
    {
        LogWarn("amqp_socket_open is failed");
        return false;
    }

    amqp_rpc_reply_t rLogin =  amqp_login(m_mqConnectionState, "/", 0, 131072, 3, AMQP_SASL_METHOD_PLAIN, m_strUserName.data(), m_strPassWord.data());
    if (AMQP_RESPONSE_NORMAL != rLogin.reply_type)
    {
        LogWarn("reply_type:%d is amqp_login failed",rLogin.reply_type);
        return false;
    }

    return true;
}

bool CMQConsumerThread::HandleCommonMsg(TMsg* pMsg)
{
    bool result =false;

    if(NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return true;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
        #ifdef RULELOAD_GET_MQ_CONFIG
        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqConfig = (TUpdateMqConfigReq*)pMsg;
            m_mapMqConfig.clear();
            m_mapMqConfig = pMqConfig->m_mapMqConfig;
            LogNotice("update t_sms_mq_configure size:%d.",m_mapMqConfig.size());
            result = true;
            break;
        }
        #endif

        #ifdef RULELOAD_GET_COMPONENT_SWITCH
        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            bool bConsumerSwitch = g_pRuleLoadThread->getComponentSwitch(g_uComponentId);

            result = true;
            if(m_bConsumerSwitch == bConsumerSwitch)
            {
                break;
            }

            m_bConsumerSwitch = bConsumerSwitch;
            LogWarn("ComponentID[%llu] m_bConsumerSwitch is %s", g_uComponentId, m_bConsumerSwitch ? "ON":"OFF");

            if (m_bConsumerSwitch)
            {
                close();
                RabbitMQConnect();
                InitChannels();
            }
            else
            {
                amqpCloseAllChannels();
            }

            break;
        }
        #endif
    }
    return result;
}

void CMQConsumerThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(1)
    {
        m_pTimerMng->Click();
        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL)
        {
            //do nothing
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
        }

        if(m_bConsumerSwitch == true)
        {
            string  strData = "";
            int     result  = AMQP_CONSUME_MESSAGE_READY;
            result = amqpConsumeMessage(strData);

            if(IS_RABBITMQ_EXCEPTION(result))
            {
                clearBlockedMsgAndResumeSlideWindowSize();
                close();
                RabbitMQConnect();
                InitChannels();
                usleep(1000*1000);
            }
            else
            {
                if(result == AMQP_BASIC_ACK_OK)
                {
                    processMessage( strData );
                }
                else
                {
                    //其他的消息不用处理
                }
            }
        }
        else
        {
            usleep(g_uSecSleep);
        }
    }
}


void CMQConsumerThread::ReleaseBlockedChannels(UInt64 uCurrentTime, int iCurrentMemoryMqQueueSize )
{
    int result = 0;
    for(map<UInt32, blockedChannelInfo>::iterator itor = m_mapBlockedChannel.begin(); itor != m_mapBlockedChannel.end(); )
    {
        map<int, ChannelSpeedCtrl>::iterator itr = m_mapChannelSpeedCtrl.find(itor->first);
        if(itr == m_mapChannelSpeedCtrl.end())
        {
            LogWarn("BlockedChannel[%d] was deleted", itor->first);
            m_mapBlockedChannel.erase(itor++);
        }
        else
        {
            if( iCurrentMemoryMqQueueSize > m_iSumChannelSpeed * MEMORY_MQ_MESSAGE_TIMES_SPEED )
            {
                LogWarn("current mq memory message queue size[%d], limit size[%d]",
                        iCurrentMemoryMqQueueSize,
                        m_iSumChannelSpeed * MEMORY_MQ_MESSAGE_TIMES_SPEED );
                itor++;
            }
            else
            {
                /**   检查通道速度通道配置速度为0
                  **  或者当前可发送速度为0 继续阻塞
                  **/
                if( itr->second.uChannelSpeed <= 0 )
                {
                    itor++;
                }
                else
                {
                    /*超时清除计数数据*/
                    if (( uCurrentTime - itr->second.uLastTime ) > TIME_ONE_SECOND )
					{
						LogNotice("[ block timeout ], amqp_basic_ack[%u:%u:%lu:%d:%d:%d]time[%llu-%llu]", 
									  itor->first, itor->second.uBlockType, 
									  itor->second.uDeliveryTag, result, 
									  itr->second.uCurrentSpeed, itr->second.uChannelSpeed, uCurrentTime, itr->second.uLastTime);
						itr->second.uCurrentSpeed  = 0 ; 
						itr->second.uLastTime = uCurrentTime;
					}

					if( itr->second.uCurrentSpeed >= itr->second.uChannelSpeed )
					{
						itor++;
						continue;
					}

					#ifdef SEND_COMPONENT_SLIDER_WINDOW
					if( CHANNEL_FLOW_STATE_NORMAL !=
							CChanelTxFlowCtr::channelFlowCtrCheckState( itor->first ))
					{
						itor++;
						continue;
					}
					#endif

					result = amqp_basic_ack( m_mqConnectionState, itor->first, itor->second.uDeliveryTag, 0 );
					if( result == 0  )//0 on success
					{
						itr->second.uCurrentSpeed ++;
						processMessage(itor->second.strData);
						m_mapBlockedChannel.erase(itor++);
					}
					else
					{
						LogError("timeout, amqp_basic_ack[%u:%lu:%d]", itor->first, itor->second.uDeliveryTag, result);
						itor++;
					}

                }

            }
        }
    }
}

void CMQConsumerThread::AddBlockedChannel(UInt32 uChannelId, UInt64 uDeliveryTag,
                string strData, UInt64 uBlockedTime, UInt32 uBlockType )
{
    blockedChannelInfo stublockedChannelInfo;

    stublockedChannelInfo.uDeliveryTag = uDeliveryTag;
    stublockedChannelInfo.strData      = strData;
    stublockedChannelInfo.uBlockedTime = uBlockedTime + TIME_ONE_SECOND ;// block one secs
    stublockedChannelInfo.uBlockType   = uBlockType;

    m_mapBlockedChannel.insert(make_pair( uChannelId, stublockedChannelInfo ));

    LogDebug("[chanel block ] Channelid[%d] BlockTIme[%lu], TotalBlockSize:%d, BlockType:%d",
                uChannelId, uBlockedTime, m_mapBlockedChannel.size(), uBlockType );
}

int CMQConsumerThread::amqpQueryMsgCnt(UInt32 uChannelId, const std::string& strQueue)
{
    amqp_queue_declare_ok_t* pResult = amqp_queue_declare(m_mqConnectionState,
                                                          uChannelId,
                                                          amqp_cstring_bytes(strQueue.c_str()),
                                                          1,
                                                          0,
                                                          0,
                                                          1,
                                                          amqp_empty_table);
    amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
    if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
    {
        LogError("channelid:%d,reply_type:%d is amqp_basic_consume failed.",uChannelId,rConsume.reply_type);
        return 1;
    }

    if (pResult == NULL)
    {
        LogError("query get NULL pResult!");
        return 1;
    }

    return pResult->message_count;
}

int CMQConsumerThread::amqpConsumeMessage(string& strData)
{
    int     result;
    UInt32  uChannelId = 0;
    UInt64  uDeliveryTag = 0;
    int     iSliderWindowCount = NO_SLIDER_WINDOW;
    UInt64  uCurrentTime = 0;
    struct timeval tvCurTime;

    gettimeofday(&tvCurTime, NULL);
    uCurrentTime = 1000000*tvCurTime.tv_sec + tvCurTime.tv_usec;

    int iMemMqQueueSize = GetMemoryMqMessageSize();

    /*检查是否有冻结的通道队列已经超时，释放消息*/
    ReleaseBlockedChannels(uCurrentTime, iMemMqQueueSize);

    amqp_rpc_reply_t ret;
    amqp_envelope_t  envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.message.body.bytes = NULL;
    amqp_maybe_release_buffers(m_mqConnectionState);

    /*rabbitmq超时时间*/
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RABBITMQ_TIME_OUT;

    ret = amqp_consume_message(m_mqConnectionState, &envelope, &tv, 0);

    //LogDebug("amqp_consume_message ret.reply_type[%d] ret.reply.id[%x] ret.library_error[-0x%x]",
    //  ret.reply_type, ret.reply.id, 0 - ret.library_error);

    switch(ret.reply_type)
    {
        case AMQP_RESPONSE_NORMAL:
        {
            break;
        }
        case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        {
            if(ret.library_error == AMQP_STATUS_TIMEOUT)
            {
                amqp_destroy_envelope(&envelope);
                // LogError("=====> ret.library_error[-0x%x:%s]", 0 - ret.library_error, amqp_error_string2(ret.library_error));
                return AMQP_CONSUME_MESSAGE_TIMEOUT;
            }
            else
            {
                LogError("ret.library_error[-0x%x:%s]", 0 - ret.library_error, amqp_error_string2(ret.library_error));
                return AMQP_CONSUME_MESSAGE_FAIL;
            }
        }
        default:
        {
            LogError("amqp_consume_message ret.reply_type[%d] ret.reply.id[%x] ret.library_error[-0x%x:%s]",
                ret.reply_type, ret.reply.id, 0 - ret.library_error, amqp_error_string2(ret.library_error));

			return AMQP_CONSUME_MESSAGE_FAIL;
		}
	}

	strData.append((char *)envelope.message.body.bytes, envelope.message.body.len);

	uChannelId = envelope.channel;
	uDeliveryTag = envelope.delivery_tag;

	LogNotice("ChannelID[%d] deliverytag[%lu] exchange[%.*s] routingkey[%.*s],time[%llu]",
		envelope.channel,
		envelope.delivery_tag,
	    envelope.exchange.len, (char *)envelope.exchange.bytes,
	    envelope.routing_key.len, (char *)envelope.routing_key.bytes,
	    uCurrentTime);

    amqp_destroy_envelope(&envelope);

    /* 滑窗控制*/
    #ifdef SEND_COMPONENT_SLIDER_WINDOW
    if ( (iSliderWindowCount = CChanelTxFlowCtr::channelFlowCtrCountSub( FLOW_CTR_CALLER_MQ, uChannelId )) < 0 )
    {
        AddBlockedChannel( uChannelId, uDeliveryTag, strData, uCurrentTime, MQ_BLOCK_TYPE_SLIDER );
        return AMQP_CHANNEL_SLIDER_WINDOW_ABNOMAL;
    }
    #endif

    map<int, ChannelSpeedCtrl>::iterator itor = m_mapChannelSpeedCtrl.find( uChannelId );
    if(itor != m_mapChannelSpeedCtrl.end())
    {
        /* 检查后端处理线程内存是否有堆积*/
        if( iMemMqQueueSize > m_iSumChannelSpeed * MEMORY_MQ_MESSAGE_TIMES_SPEED )
        {
            LogWarn("current mq memory message queue size[%d], limit size[%d]",
                    iMemMqQueueSize, m_iSumChannelSpeed * MEMORY_MQ_MESSAGE_TIMES_SPEED);
            AddBlockedChannel( uChannelId, uDeliveryTag, strData, uCurrentTime , MQ_BLOCK_TYPE_QUEUE_SIZE );
            return AMQP_COMPONENT_MEMORY_QUEUE_FULL;
        }

		UInt64 elaseTime = uCurrentTime - itor->second.uLastTime ;
		if ( itor->second.uChannelSpeed > 0 &&
			 ( itor->second.uCurrentSpeed  < itor->second.uChannelSpeed
			    || elaseTime > TIME_ONE_SECOND ))
		{
			result = amqp_basic_ack(m_mqConnectionState, uChannelId, uDeliveryTag, 0);
			LogDebug("amqp_basic_ack[%d:%lu:%d:%d:%d:%d]time[%llu-%llu]", 
					uChannelId, uDeliveryTag, result, itor->second.uCurrentSpeed, 
					itor->second.uChannelSpeed, iSliderWindowCount, uCurrentTime, itor->second.uLastTime);

			if( result != 0 )
			{
				LogDebug("amqp_basic_ack [ack error][ %d:%lu:%d:%d:%d:%d ]",
						uChannelId, uDeliveryTag, result, itor->second.uCurrentSpeed,
						itor->second.uChannelSpeed, iSliderWindowCount);
				return AMQP_BASIC_ACK_FAIL;
			}
			else
			{
				//重置计数器
				itor->second.uCurrentSpeed++;
				if ( elaseTime >= TIME_ONE_SECOND ){
					itor->second.uLastTime = uCurrentTime;
					itor->second.uCurrentSpeed = 1;
				}
				return AMQP_BASIC_ACK_OK;
			}

		}
		else
		{
			AddBlockedChannel( uChannelId, uDeliveryTag, strData, uCurrentTime, MQ_BLOCK_TYPE_SPEED );
			LogWarn("OverSpeed! ChannelId[%d] Queue[%s] DeliveryTag[%lu] CurrentSpeed[%d] LimitSpeed[%d],time[%llu-%llu]",
					    uChannelId, itor->second.strQueue.data(), uDeliveryTag,
					    itor->second.uCurrentSpeed, itor->second.uChannelSpeed,
					    uCurrentTime, itor->second.uLastTime);
			return AMQP_CHANNEL_OVER_SPEED;
		}

    }
    else
    {
        LogError("rabbitmq send message error, ChannelId[%d]", uChannelId);
        /*诡异的消息，并没有订阅这条通道队列*/
        /*拒绝这样的消息*/
        result = amqp_basic_reject(m_mqConnectionState, uChannelId, uDeliveryTag, 0);

        if(result != 0)
        {
            return AMQP_BASIC_REJECT_FAIL;
        }

        /*关闭这条通道取消订阅*/
        amqp_channel_close(m_mqConnectionState, uChannelId, AMQP_REPLY_SUCCESS);

        return AMQP_BASIC_REJECT_OK;
    }
}
