#include "CRouterStrategy.h"
#include "CRouterThread.h"

CRouterStrategy::CRouterStrategy()
 : m_pRouter(NULL), m_pRouterThread(NULL), m_pSMSSmsInfo(NULL)
{

}

CRouterStrategy::~CRouterStrategy()
{

}

bool CRouterStrategy::Init(CBaseRouter* pRouter)
{
    m_pRouter = pRouter;
    if (NULL == m_pRouter)
    {
        LogWarn("m_pRouter is NULL");
        return false;
    }

    m_pRouterThread = pRouter->m_pRouterThread;
    m_pSMSSmsInfo = pRouter->m_pSMSSmsInfo;

    if (NULL == m_pRouterThread || NULL == m_pSMSSmsInfo)
    {
        LogWarn("m_pRouterThread or m_pSMSSmsInfo is NULL");
        return false;
    }

    return true;
}

Int32 CRouterStrategy::CheckRoute(const models::Channel& smsChannel,string& strReason)
{
    if (false == m_pRouter->CheckRoute(smsChannel,strReason))
    {
        return CHANNEL_GET_FAILURE_OTHER;
    }

    if(false == m_pRouter->CheckChanneOverrate(smsChannel))
    {
        return CHANNEL_GET_FAILURE_OVERRATE;
    }

    if (false == m_pRouter->CheckOverFlow(smsChannel))
    {
        return CHANNEL_GET_FAILURE_OVERFLOW;
    }

    if(false == m_pRouter->CheckChannelLoginStatus(smsChannel))
    {
        return CHANNEL_GET_FAILURE_LOGINSTATUS;
    }


    return CHANNEL_GET_SUCCESS;
}

weight_calculator_ptr_t CRouterStrategy::GetWeightCalculator(CRouterThread* pRouterThread, TSmsRouterReq* pSMSSmsInfo, ChannelGroupWeight* pChlGrpWeight)
{
    return weight_calculator_ptr_t(new CDefaultChannelWeightCalculator(pRouterThread, pSMSSmsInfo, pChlGrpWeight));
}

/* @功能:    从给定的通道组集合中，遍历通道组，并在通道组内按权重优先找到可用的通道.
 *
 * @返回值:  0:  成功;
 *           1:  因流控导致的失败;
 *           2:  获取通道组失败;
 *           99: 其他失败;
 */
Int32 CRouterStrategy::SelectChlFromChlGrp(const std::vector<std::string>& vecChannelGroups, models::Channel& smsChannel)
{
    bool bGetChannlelSuc = false;
    bool bOverFlower = false;
    bool bLoginFail = false;
    bool bChannelOverrate = false;

    // 获取所有通道组的权重信息
    channelgroups_weight_t& channelGroupsWeightMap = m_pRouterThread->GetChannelGroupsWeightMap();

    ChannelScheduler* channelSchedule = m_pRouterThread->GetChannelScheduler();

    char szMsgTip[128] = {0};
    snprintf(szMsgTip, sizeof(szMsgTip), "[%s:%s]", m_pSMSSmsInfo->m_strSmsId.data(),m_pSMSSmsInfo->m_strPhone.data());

    // 遍历所有通道组
    for(vector<std::string>::const_iterator itrGroups = vecChannelGroups.begin(); itrGroups != vecChannelGroups.end(); itrGroups++)
    {
        UInt32 uChlGrpID = atoi(itrGroups->data());

        LogNotice("== %s select channel ==> search in channelgroup[%u].", szMsgTip, uChlGrpID);

        channelgroups_weight_t::iterator itCGW = channelGroupsWeightMap.find(uChlGrpID);
        if(itCGW == channelGroupsWeightMap.end())
        {
            LogWarn("%s channelGroup[%u] NOT found", szMsgTip, uChlGrpID);
            continue;
        }

        // 取出当前通道组的权重信息
        ChannelGroupWeight& channelGroupWeight = itCGW->second;

        // 取实际的计算类
        weight_calculator_ptr_t pCalculator = this->GetWeightCalculator(m_pRouterThread, m_pSMSSmsInfo, &channelGroupWeight);
        if (pCalculator->init() == true)
        {
            channelGroupWeight.weightSort(*pCalculator);
        }
        else
        {
            // 权重计算初始化失败，则忽略权重排序和选择
            LogWarn("%s calculator of channelgroup[%u] init FAILED. WeightSort IGNORED!", szMsgTip, uChlGrpID);
        }

        // 权重排序完成后，遍历通道组内的通道，取权重优先通道
        for (ChannelGroupWeight::iterator itChannel = channelGroupWeight.begin(); itChannel != channelGroupWeight.end(); ++itChannel)
        {
            UInt32 uChannelId = (*itChannel)->getData();

            LogNotice("%s try channel[%u:%u]", szMsgTip, uChannelId, uChlGrpID);

            if (channelSchedule->getChannel(uChannelId, smsChannel))
            {
                string strReason;
                int iResult = this->CheckRoute(smsChannel, strReason);

                if (CHANNEL_GET_SUCCESS != iResult)
                {
                    if (iResult == CHANNEL_GET_FAILURE_OVERFLOW)
                    {
                        LogWarn("%s channel[%u:%u] OVERFLOW!", szMsgTip, uChannelId, uChlGrpID);
                        bOverFlower = true;
                    }
                    else if(iResult == CHANNEL_GET_FAILURE_LOGINSTATUS)
                    {
                        LogWarn("%s channel[%u:%u] Login Fail.", szMsgTip, uChannelId, uChlGrpID);
                        bLoginFail = true;
                    }
                    else if (iResult == CHANNEL_GET_FAILURE_OVERRATE)
                    {
                        LogWarn("%s channel[%u:%u] channel overrate.", szMsgTip, uChannelId, uChlGrpID);
                        bChannelOverrate = true;
                    }

                    LogWarn("%s channel[%u:%u] check failed: [%s].", szMsgTip, uChannelId, uChlGrpID, strReason.c_str());
                    continue;
                }

                LogNotice("== %s do '%s' channel select SUCCESS ==> channel[%u:%u].", szMsgTip, m_strStrategyName.c_str(), uChannelId, uChlGrpID);

                // 选中可用通道后，要更新选中元素的有效权重!!!!!
                channelGroupWeight.updateSelectedChannel(itChannel);

                bGetChannlelSuc = true;
                break;

            }
            else
            {
                LogWarn("===%s channel select === failed. channel[%u] NOT open.", szMsgTip, uChannelId);
            }
        }
        if (bGetChannlelSuc)
        {
            break;
        }
    }

    if (bGetChannlelSuc)
        return CHANNEL_GET_SUCCESS;

    if (bChannelOverrate)
        return CHANNEL_GET_FAILURE_OVERRATE;

    if (bOverFlower)
        return CHANNEL_GET_FAILURE_OVERFLOW;

    if (bLoginFail)
        return CHANNEL_GET_FAILURE_LOGINSTATUS;

    return CHANNEL_GET_FAILURE_OTHER;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CDefaultChannelWeightCalculator::CDefaultChannelWeightCalculator(CRouterThread* pRouterThread,
                                                             TSmsRouterReq* pSMSSmsInfo,
                                                             ChannelGroupWeight* pChlGrpWeight)
    : m_pRouterThread(pRouterThread),
      m_pSMSSmsInfo(pSMSSmsInfo),
      m_pChlGrpWeight(pChlGrpWeight)
{
}

bool CDefaultChannelWeightCalculator::init()
{
    // 短信信息是基本的
    if (m_pSMSSmsInfo == NULL)
    {
        return false;
    }

    memset(m_szLogPrefix, 0, sizeof(m_szLogPrefix));
    snprintf(m_szLogPrefix, sizeof(m_szLogPrefix), "[%s:%s]",
                     m_pSMSSmsInfo->m_strSmsId.data(),m_pSMSSmsInfo->m_strPhone.data());

    return true;
}

bool CDefaultChannelWeightCalculator::loadChannelRealtimeWeightInfo(UInt32 uChannelId, ChannelRealtimeWeightInfo& channelRealtimeWeightInfo)
{
    // 加载所有通道实时权重信息
    channels_realtime_weightinfo_t channelsRealtimeWeightInfo = m_pRouterThread->GetChannelsRealtimeWeightInfo();

    // 找到给定通道的实时权重信息
    channels_realtime_weightinfo_t::iterator crtwiIter = channelsRealtimeWeightInfo.find(uChannelId);
    if (crtwiIter == channelsRealtimeWeightInfo.end())
    {
        LogWarn("%s channel-realtime-weightinfo of channel[%u] NOT FOUND!", m_szLogPrefix, uChannelId);
        return false;
    }

    channelRealtimeWeightInfo = crtwiIter->second;

    return true;
}

