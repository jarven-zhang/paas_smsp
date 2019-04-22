#include "CReSendRouterStrategy.h"
#include "CRouterThread.h"
#include "Comm.h"

CReSendRouterStrategy::CReSendRouterStrategy() : CRouterStrategy()
{
    m_strStrategyName = "ReSendRoute";
}

CReSendRouterStrategy::~CReSendRouterStrategy()
{
}

Int32 CReSendRouterStrategy::BaseRoute(models::Channel& smsChannel,string& strReason)
{
    //检查是否为为失败重发消息
    if(m_pSMSSmsInfo->m_iFailedResendTimes <= 0)
    {
        strReason = "No need Failed ReSend route!";
        return CHANNEL_GET_FAILURE_NOT_RESEND_MSG;
    }

    vector<string> vecChannelGroups;
    string strKey;

    vecChannelGroups.clear();
    strKey = m_pSMSSmsInfo->m_strClientId + "_" + m_pSMSSmsInfo->m_strSmsType;

    if(false == m_pRouterThread->GetReSendChannelGroup(strKey, vecChannelGroups))
    {
        strReason = strKey + " find no resend channel group";
        return CHANNEL_GET_FAILURE_NO_CHANNELGROUP;
    }

    return SelectChlFromChlGrp(vecChannelGroups,smsChannel);
}

/*函数作用: 通道路由
 *返回值:  0: 成功; 1: 因流控导致的失败; 2:获取通道组失败 99:其他失败
 */
Int32 CReSendRouterStrategy::GetRoute(models::Channel& smsChannel)
{
    string strReason = "";
    int retval = BaseRoute(smsChannel,strReason);

    if(retval != CHANNEL_GET_SUCCESS)
    {
        LogNotice("[%s:%s] route fail, Reason[%s]",
            m_pSMSSmsInfo->m_strSmsId.data(),
            m_pSMSSmsInfo->m_strPhone.data(),
            strReason.data());
    }

    return retval;
}

Int32 CReSendRouterStrategy::CheckRoute(const models::Channel& smsChannel,string& strReason)
{
//  if (false == CheckChannelSignType(smsChannel))
//  {
//      return CHANNEL_GET_FAILURE_OTHER;
//  }

    return CRouterStrategy::CheckRoute(smsChannel,strReason);
}

bool CReSendRouterStrategy::CheckChannelSignType(const models::Channel& smsChannel)
{
    if (0 != smsChannel.m_uChannelType)
    {
        LogWarn("[%s:%s] channel[%d] channentype[%u] error",
            m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), smsChannel.channelID, smsChannel.m_uChannelType);
        return false;
    }

    return true;
}

