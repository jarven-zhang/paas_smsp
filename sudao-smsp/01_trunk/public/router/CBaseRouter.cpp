#include "CBaseRouter.h"
#include "CForceRouterStrategy.h"
#include "CIntelligenceRouterStrategy.h"
#include "CTestRouterStrategy.h"
#include "CWhiteKeywordRouterStrategy.h"
#include "CAutoRouterStrategy.h"
#include "CReSendRouterStrategy.h"
#include "CSendLimitRouterStrategy.h"
#include "Comm.h"
#include "UrlCode.h"

using namespace BusType;

CBaseRouter::CBaseRouter()
    : m_pRouterThread(NULL), m_pSMSSmsInfo(NULL), m_uRouterType(0), m_pRouterStrategy(NULL)
{

}

CBaseRouter::~CBaseRouter()
{

}

void CBaseRouter::ResetSelectNum()
{
    //do nothing
}


bool CBaseRouter::Init(CRouterThread *pRouterThread, TSmsRouterReq *pInfo)
{
    m_pRouterThread = pRouterThread;
    m_pSMSSmsInfo = pInfo;

    if (NULL == pRouterThread || NULL == m_pSMSSmsInfo)
        return false;

    return true;
}

/*函数作用: 通道路由
 *返回值:  0: 成功; 1: 因流控导致的失败; 2:获取通道组失败 99:其他失败
 */
Int32 CBaseRouter::GetRoute(models::Channel &smsChannel, ROUTE_STRATEGY strategy)
{
    switch (strategy)
    {
        case STRATEGY_SEND_LIMIT:
            m_pRouterStrategy = new CSendLimitRouterStrategy();
            break;
        case STRATEGY_RESEND:
            m_pRouterStrategy = new CReSendRouterStrategy();
            break;
        case STRATEGY_FORCE:
            m_pRouterStrategy = new CForceRouterStrategy();
            break;
        case STRATEGY_INTELLIGENCE:
            m_pRouterStrategy = new CIntelligenceRouterStrategy();
            break;
        case STRATEGY_TEST:
            m_pRouterStrategy = new CTestRouterStrategy();
            break;
        case STRATEGY_WHITE_KEYWORD:
            m_pRouterStrategy = new CWhiteKeywordRouterStrategy();
            break;
        case STRATEGY_AUTO:
            m_pRouterStrategy = new CAutoRouterStrategy();
            break;
        default:
            return CHANNEL_GET_FAILURE_OTHER;
    }

    if (NULL == m_pRouterStrategy)
    {
        LogWarn("m_pRouterStrategy is NULL");
        return CHANNEL_GET_FAILURE_OTHER;
    }

    if (!m_pRouterStrategy->Init(this))
    {
        LogWarn("Init m_pRouterStrategy failure");

        delete m_pRouterStrategy;
        m_pRouterStrategy = NULL;

        return CHANNEL_GET_FAILURE_OTHER;
    }

    Int32 ret = m_pRouterStrategy->GetRoute(smsChannel);

    if (ret != CHANNEL_GET_SUCCESS)
    {
        string strRouteStrategy = RouteStrategy2String(strategy);

        LogNotice("[%s:%s:%s] Do %s RouteStrategy fail, code[%d].",
            m_pSMSSmsInfo->m_strClientId.data(),
            m_pSMSSmsInfo->m_strSmsId.data(),
            m_pSMSSmsInfo->m_strPhone.data(),
            strRouteStrategy.data(),
            ret);
    }

    return ret;
}

string CBaseRouter::RouteStrategy2String(ROUTE_STRATEGY strategy)
{
    string retval;

    switch (strategy)
    {
    case STRATEGY_RESEND:
        retval = "STRATEGY_RESEND";
        break;
    case STRATEGY_FORCE:
        retval = "STRATEGY_FORCE";
        break;
    case STRATEGY_INTELLIGENCE:
        retval = "STRATEGY_INTELLIGENCE";
        break;
    case STRATEGY_TEST:
        retval = "STRATEGY_TEST";
        break;
    case STRATEGY_WHITE_KEYWORD:
        retval = "STRATEGY_WHITE_KEYWORD";
        break;
    case STRATEGY_AUTO:
        retval = "STRATEGY_AUTO";
        break;
    default:
        retval = "STRATEGY_UNDEFINED";
    }

    return retval;
}

bool CBaseRouter::CheckRoute(const models::Channel &smsChannel, string &strReason)
{
    if (false == CheckChannelAttribute(smsChannel, strReason) 				||
            false == CheckFailedReSendChannels(smsChannel, strReason)			||
            false == CheckChannelOverrateReSendChannels(smsChannel, strReason)	||
            false == CheckChannelExValue(smsChannel, strReason)					||
            false == CheckChannelOperater(smsChannel, strReason)        		||
            false == CheckChannelSendTime(smsChannel, strReason) 				||
            false == CheckChannelKeyWord(smsChannel, strReason) 					||
            false == CheckChannelBlackWhiteList(smsChannel, strReason) 			||
            false == CheckChannelTemplateId(smsChannel, strReason) 				||
            false == CheckProvinceRoute(smsChannel, strReason) 					||
            false == CheckAndGetExtendPort(smsChannel, strReason) 				||
            false == CheckChannelQueueSize(smsChannel, strReason))
    {
        return false;
    }

    return true;
}

bool CBaseRouter::CheckFailedReSendChannels(const models::Channel &smsChannel, string &strReason)
{
    if(m_pSMSSmsInfo->m_FailedResendChannelsSet.find(smsChannel.channelID) != m_pSMSSmsInfo->m_FailedResendChannelsSet.end())
    {
        strReason = "channel included in failed resend channels:" + m_pSMSSmsInfo->m_strFailedResendChannels;
        return false;
    }
    else
    {
        return true;
    }
}

bool CBaseRouter::CheckChannelOverrateReSendChannels(const models::Channel &smsChannel, string &strReason)
{
    if(m_pSMSSmsInfo->m_channelOverrateResendChannelsSet.find(smsChannel.channelID) != m_pSMSSmsInfo->m_channelOverrateResendChannelsSet.end())
    {
        strReason = "channel included in channel overrate resend channels:" + m_pSMSSmsInfo->m_strFailedResendChannels;
        return false;
    }
    else
    {
        return true;
    }
}

bool CBaseRouter::CheckChannelAttribute(const models::Channel &smsChannel, string &strReason)
{
    if (smsChannel.channelname.empty() || smsChannel.channelID == 0)
    {
        LogWarn("[%s:%s] channelId:%d, channelname:%s",
                m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID, smsChannel.channelname.c_str());

        strReason = "CheckChannelAttribute is failed";
        return false;
    }

    return true;
}

bool CBaseRouter::CheckChannelExValue(const models::Channel &smsChannel, string &strReason)
{
    if(m_pSMSSmsInfo->m_bIncludeChinese)
    {
        if(false == (smsChannel.m_uExValue & 0x1))
        {
            strReason = "CheckChannelExValue is failed";
            return false;
        }
    }
    else
    {
        if(false == (smsChannel.m_uExValue & 0x2))
        {
            strReason = "CheckChannelExValue is failed";
            return false;
        }
    }

    return true;
}

bool CBaseRouter::CheckChannelOperater(const models::Channel &smsChannel, string &strReason)
{
    if((0 != smsChannel.operatorstyle) && (m_pSMSSmsInfo->m_uOperater != smsChannel.operatorstyle))
    {
        strReason = "CheckChannelOperater failed";
        return false;
    }
    return true;
}

bool CBaseRouter::CheckChannelQueueSize(const models::Channel &smsChannel, string &strReason)
{
    map<UInt32, UInt32> &ChannelQueueSizeMap = m_pRouterThread->GetChannelQueueSizeMap();
    map<UInt32, UInt32>::iterator itrSize = ChannelQueueSizeMap.find(smsChannel.channelID);
    if (ChannelQueueSizeMap.end() != itrSize)
    {
        if (smsChannel.m_uChannelMsgQueueMaxSize <= (int)itrSize->second)
        {
            LogNotice("[%s:%s] channelId:%d,channelMqTrueSize:%d,is over channelMqMaxSize:%d.",
                      m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID,
                      itrSize->second, smsChannel.m_uChannelMsgQueueMaxSize);

            strReason = "CheckChannelQueueSize is failed";
            return false;
        }
    }

    return true;
}

bool CBaseRouter::CheckChannelSendTime(const models::Channel &smsChannel, string &strReason)
{
    std::string strCurHourMins = Comm::getCurrHourMins();
    if( !smsChannel.m_strBeginTime.empty() && !smsChannel.m_strEndTime.empty() &&
            (strCurHourMins < smsChannel.m_strBeginTime || strCurHourMins > smsChannel.m_strEndTime))
    {
        LogNotice("[%s:%s] channel passed, not in correct time area .channelid[%d]. beginTime[%s], endTime[%s], currentTime[%s]. ",
                  m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID,
                  smsChannel.m_strBeginTime.data(), smsChannel.m_strEndTime.data(), strCurHourMins.data());

        strReason = "CheckChannelSendTime is failed";
        return false;
    }

    return true;
}

bool CBaseRouter::CheckChannelKeyWord(const models::Channel &smsChannel, string &strReason)
{


    std::string strOut;
    if(1 == m_pSMSSmsInfo->m_uFreeChannelKeyword)
    {
        LogNotice("[%s_%s] is free channelKeyword,channel[%d]", m_pSMSSmsInfo->m_strClientId.data(), m_pSMSSmsInfo->m_strSmsType.data(), smsChannel.channelID);
        return true;
    }

    /* 通道关键字不做处理*/
#if 0
    if(m_pHttpSendThread->CheckRegex(smsChannel.channelID, m_pSMSSmsInfo->m_strContentSimple, strOut, m_pSMSSmsInfo->m_strSignSimple))
#else
    if(m_pRouterThread->CheckRegex(smsChannel.channelID, m_pSMSSmsInfo->m_strContent, strOut, m_pSMSSmsInfo->m_strSign))
#endif
    {
        LogNotice("[%s:%s] channel passed, content has keyword[%s]! channel[%d]",
                  m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), strOut.data(), smsChannel.channelID);

        strReason = "channel keyword:";
        strReason.append(strOut.data());
        return false;
    }

    return true;
}

bool CBaseRouter::CheckChannelBlackWhiteList(const models::Channel &smsChannel, string &strReason)
{
    if(smsChannel.m_uWhiteList)
    {
        map<UInt32, UInt64Set> &ChannelWhiteListMap = m_pRouterThread->GetChannelWhiteListMap();
        map<UInt32, UInt64Set>::iterator itrr2 = ChannelWhiteListMap.find(smsChannel.channelID);
        if(itrr2 != ChannelWhiteListMap.end())
        {
            UInt64Set::iterator itSet = itrr2->second.find(atol(m_pSMSSmsInfo->m_strPhone.data()));
            if(itSet == itrr2->second.end())
            {
                LogNotice("[%s:%s] =====channel_selecte=====channel passed, phone is not in white list! channel[%d].",
                          m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID);

                strReason = "CheckChannelBlackWhiteList is failed";
                return false;
            }
        }
        else
        {
            LogNotice("[%s:%s] =====channel_selecte=====channel passed, channel has 0 white list! channel[%d].",
                      m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID);

            strReason = "CheckChannelBlackWhiteList is failed";
            return false;
        }
    }
    else
    {
        set<string>::iterator itrr = m_pSMSSmsInfo->m_phoneBlackListInfo.find(Comm::int2str(smsChannel.channelID));
        if(itrr != m_pSMSSmsInfo->m_phoneBlackListInfo.end())
        {

            LogNotice("[%s,%s]=====channel_selecte=====channel passed, phone in blacklist! channel[%d].",
                      m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID);

            strReason = "CheckChannelBlackWhiteList is failed";
            return false;

        }
    }

    return true;
}


bool CBaseRouter::CheckChannelTemplateId(const models::Channel &smsChannel, string &strReason)
{
    //smstype为USSD的需要选择对应的通道模板id,并判断消息内容长度
    int iSmsTyep = atoi(m_pSMSSmsInfo->m_strSmsType.c_str());
    if (iSmsTyep == SMS_TYPE_USSD || iSmsTyep == SMS_TYPE_FLUSH_SMS)
    {
        std::string strkey;
        strkey.assign(m_pSMSSmsInfo->m_strTemplateId).append("_").append(Comm::int2str(smsChannel.channelID));
        STL_MAP_STR &ChannelTemplateMap = m_pRouterThread->GetChannelTemplateMap();
        STL_MAP_STR::iterator itor = ChannelTemplateMap.find(strkey);
        if (itor == ChannelTemplateMap.end())
        {
            LogWarn("[%s:%s] the smsType is [%s], select channel[%d], but not find channel templateid.",
                    m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), m_pSMSSmsInfo->m_strSmsType.c_str(), smsChannel.channelID);

            strReason = "CheckChannelTemplateId is failed---not find channel templateid";
            return false;
        }

        std::string strTempLength = "";    //计算长度用
        std::string strLeft = "";
        std::string strRight = "";
        if (true == Comm::IncludeChinese((char *)m_pSMSSmsInfo->m_strContent.c_str()) ||
                true == Comm::IncludeChinese((char *)m_pSMSSmsInfo->m_strSign.c_str()))
        {
            strLeft = "%e3%80%90";
            strRight = "%e3%80%91";
            strLeft = http::UrlCode::UrlDecode(strLeft);
            strRight = http::UrlCode::UrlDecode(strRight);
        }
        else
        {
            strLeft = "[";
            strRight = "]";
        }

        strTempLength = strLeft + m_pSMSSmsInfo->m_strSign + strRight + m_pSMSSmsInfo->m_strContent;

        utils::UTFString utfHelper;
        UInt32 msgLength = utfHelper.getUtf8Length(strTempLength);
        LogDebug("get length[%u], for content[%s]", msgLength, strTempLength.c_str());

        if (msgLength > smsChannel.m_uContentLength)
        {
            LogWarn("[%s:%s] select channel[%d], but content_length[%u] is long than channel_max_length[%lu].",
                    m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), smsChannel.channelID,
                    msgLength, smsChannel.m_uContentLength);

            strReason = "CheckChannelTemplateId is failed---content is too long";
            return false;
        }

        m_pSMSSmsInfo->m_strChannelTemplateId = itor->second;    //获取通道模板id
    }

    return true;
}


bool CBaseRouter::CheckAndGetExtendPort(const models::Channel &smsChannel, string &strReason)
{
    stl_map_str_SmsAccount &AccountMap = m_pRouterThread->GetAccountMap();
    stl_map_str_SmsAccount::iterator itrAccount = AccountMap.find(m_pSMSSmsInfo->m_strClientId);
    if (itrAccount == AccountMap.end())
    {
        LogError("[%s:%s] account[%s] not found in AccountMap",
                 m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), m_pSMSSmsInfo->m_strClientId.data());

        strReason = "CheckAndGetExtendPort is failed---not found in AccountMap";
        return false;
    }

    ChannelSelect *chlSelect = m_pRouterThread->GetChannelSelect();

    if (0 == smsChannel.m_uChannelType)
    {
        LogDebug("[%s:%s] ======this is  auto sign and port channel.", m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data());
        m_pSMSSmsInfo->m_strUcpaasPort.assign(itrAccount->second.m_strExtNo);
    }
    else if (3 == smsChannel.m_uChannelType)
    {
        LogDebug("[%s:%s] ======this is  nature port channel.", m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data());
        string strExtendPort = "";

        if (false == chlSelect->getChannelExtendPort(smsChannel.channelID, m_pSMSSmsInfo->m_strClientId, strExtendPort))
        {
            LogWarn("[%s:%s] channelId[%u],clientId[%s] is not find in table t_sms_channel_extendport.",
                    m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID, m_pSMSSmsInfo->m_strClientId.data());

            strReason = "CheckAndGetExtendPort is failed---not find in table t_sms_channel_extendport";
            return false;
        }

        m_pSMSSmsInfo->m_strUcpaasPort.assign(strExtendPort);
    }
    else
    {
        LogDebug("[%s:%s] ======this is not support auto sign channel. ChannelType(%u).",
                 m_pSMSSmsInfo->m_strSmsId.data(),
                 m_pSMSSmsInfo->m_strPhone.data(),
                 smsChannel.m_uChannelType);

        string strExtPort = "";
        string strClientIds = "";

        if (false == chlSelect->getExtendPort(smsChannel.channelID, m_pSMSSmsInfo->m_strSign, m_pSMSSmsInfo->m_strClientId, strExtPort))
        {
            LogWarn("[%s:%s] access sign[%s] channelid[%d],clientid[%s] is not find in table t_signextno_gw.",
                    m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), m_pSMSSmsInfo->m_strSign.data(), smsChannel.channelID, m_pSMSSmsInfo->m_strClientId.data());

            strReason = "CheckAndGetExtendPort is failed----not find in table t_signextno_gw";
            return false;
        }
        m_pSMSSmsInfo->m_strUcpaasPort.assign(strExtPort);
        if (1 == smsChannel.m_uChannelType)
        {
            m_pSMSSmsInfo->m_strExtpendPort.assign("");
            m_pSMSSmsInfo->m_strSignPort.assign("");
        }
        else if (2 == smsChannel.m_uChannelType)
        {
            m_pSMSSmsInfo->m_strSignPort.assign("");
        }
        else
        {
            LogWarn("[%s:%s] channelType[%d] is invalid type.", m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.m_uChannelType);
            m_pSMSSmsInfo->m_strExtpendPort.assign("");
            m_pSMSSmsInfo->m_strSignPort.assign("");
        }
    }

    if (false == m_pSMSSmsInfo->m_strExtpendPort.empty())
    {
        if (false == itrAccount->second.m_strSpNum.empty())
        {
            std::string strTmep = "";
            if (m_pSMSSmsInfo->m_strExtpendPort.length() >= itrAccount->second.m_strSpNum.length())
            {
                strTmep = m_pSMSSmsInfo->m_strExtpendPort.substr(itrAccount->second.m_strSpNum.length());
                LogDebug("==&&==strExtpendPort[%s],strSpNum[%s],strTmep[%s].",
                         m_pSMSSmsInfo->m_strExtpendPort.data(), itrAccount->second.m_strSpNum.data(), strTmep.data());
                m_pSMSSmsInfo->m_strExtpendPort.assign(strTmep);
            }
        }
    }
    return true;
}

bool CBaseRouter::CheckProvinceRoute(const models::Channel &smsChannel, string &strReason)
{
    LogDebug("[%s:%s] channel[%d] segcode_type[%u]",
             m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), smsChannel.channelID, smsChannel.m_uSegcodeType);

    if (smsChannel.m_uSegcodeType == 0)
    {
        return true;
    }

    ChannelSelect *chlSelect =  m_pRouterThread->GetChannelSelect();
    map<UInt32, stl_list_int32> &ChannelSegCode = m_pRouterThread->GetChannelSegCodeMap();

    int iPhoneCode = chlSelect->getPhoneCode(m_pSMSSmsInfo->m_strPhone);
    map<UInt32, stl_list_int32>::iterator it = ChannelSegCode.find(smsChannel.channelID);
    if(it == ChannelSegCode.end())
    {
        LogError("[%s:%s] channel[%d] is for province, but not found channel segcode",
                 m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), smsChannel.channelID);

        strReason = "CheckProvinceRoute is failed---not found channel segcode";
        return false;
    }

    stl_list_int32 &PhoneCodeList = it->second;
    stl_list_int32::iterator itor = std::find(PhoneCodeList.begin(), PhoneCodeList.end(), iPhoneCode);
    if (itor == PhoneCodeList.end())
    {
        LogError("[%s:%s]  channel[%d] is for province, but not support phonecode[%d]",
                 m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), smsChannel.channelID, iPhoneCode);

        strReason = "CheckProvinceRoute is failed---not support phonecode";
        return false;
    }

    return true;
}

bool CBaseRouter::CheckOverFlow(const models::Channel &smsChannel)
{
    if(m_pRouterThread->CheckOverFlow(m_pSMSSmsInfo, smsChannel))
    {
        LogDebug("channel[%d] this is overflower.", smsChannel.channelID);
        return false;
    }

    return true;
}

bool CBaseRouter::CheckChanneOverrate(const models::Channel &smsChannel)
{
    if(m_pRouterThread->checkChannelOverrate(m_pSMSSmsInfo, smsChannel))
    {
        LogDebug("channel[%d] this is channel overrate.", smsChannel.channelID);
        return false;
    }

    return true;
}

bool CBaseRouter::CheckChannelLoginStatus(const models::Channel &smsChannel)
{
    //http不判断登录状态
    return((smsChannel.httpmode <= HTTP_POST)
           || m_pRouterThread->GetChannelLoginStatus(smsChannel.channelID));
}



