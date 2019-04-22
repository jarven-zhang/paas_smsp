#include "CSendLimitRouterStrategy.h"
#include "CRouterThread.h"
#include "Comm.h"
#include "global.h"

CSendLimitRouterStrategy::CSendLimitRouterStrategy() : CRouterStrategy()
{
    m_strStrategyName = "SendLimitRoute";
}

CSendLimitRouterStrategy::~CSendLimitRouterStrategy()
{
}

Int32 CSendLimitRouterStrategy::BaseRoute(models::Channel& smsChannel,string& strReason)
{
    models::UserGw userGw;
    vector<string> vecChannelGroups;
    ChannelSelect* chlSelect = m_pRouterThread->GetChannelSelect();

    if (!chlSelect->getChannelGroupVec("send_limit",
                                     m_pSMSSmsInfo->m_strSid,
                                     m_pSMSSmsInfo->m_strSmsType,
                                     m_pSMSSmsInfo->m_uSmsFrom,
                                     m_pSMSSmsInfo->m_uOperater,
                                     vecChannelGroups,
                                     userGw))
    {
        LogError("[%s:%s:%s] get channel groups failed. ",
            m_pSMSSmsInfo->m_strClientId.data(),
            m_pSMSSmsInfo->m_strSmsId.data(),
            m_pSMSSmsInfo->m_strPhone.data());

        strReason = "no channel group";

        return CHANNEL_GET_FAILURE_NO_CHANNELGROUP;
    }

    LogDebug("get channel group[%s] success.", join(vecChannelGroups, ",").data());

    return SelectChlFromChlGrp(vecChannelGroups, smsChannel);
}

Int32 CSendLimitRouterStrategy::GetRoute(models::Channel& smsChannel)
{
    string strReason;
    int retval = BaseRoute(smsChannel, strReason);

    if (retval != CHANNEL_GET_SUCCESS)
    {
        LogNotice("[%s:%s:%s] route fail, Reason[%s]",
            m_pSMSSmsInfo->m_strClientId.data(),
            m_pSMSSmsInfo->m_strSmsId.data(),
            m_pSMSSmsInfo->m_strPhone.data(),
            strReason.data());
    }

    return retval;
}

Int32 CSendLimitRouterStrategy::CheckRoute(const models::Channel& smsChannel,string& strReason)
{
    // 不做任何判断
    return CHANNEL_GET_SUCCESS;
}

