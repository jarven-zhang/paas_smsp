#include <main.h>
#include "ChannelPriorityHelper.h"
#include "ClientPriority.h"

enum SearchPhase {
    PHASE_BEGIN = 0,
    PHASE_ACNT_SIGN_SMSTYPE = 0,
    PHASE_AST_SIGN_SMSTYPE = 1,
    PHASE_ACNT_AST_SMSTYPE = 2,
    PHASE_AST_AST_SMSTYPE = 3,
    PHASE_END = 4
};

int ChannelPriorityHelper::GetClientChannelPriority(smsSessionInfo* pInfo, const client_priorities_t& clientPriorities )
{
    int iPriority = DEFAULT_CLIENT_PRIORITY;

    do 
    {
        if (pInfo == NULL)
        {
            LogWarn("Null pInfo!");
            break;
        }

        if ((pInfo->m_uChannelExValue & CHANNEL_PRIORITY_FLAG) != CHANNEL_PRIORITY_FLAG)
        {
            LogDebug("Channel[%u] priority-flag [OFF]", pInfo->m_uChannleId);
            break;
        }
        for (SearchPhase phase = PHASE_BEGIN; phase < PHASE_END; phase = SearchPhase(1+(int)phase))
        {
            std::string strKey;
            switch(phase)
            {
                case PHASE_ACNT_SIGN_SMSTYPE:
                    strKey = pInfo->m_strClientId + "_" + pInfo->m_strSign + "_" + pInfo->m_strSmsType;
                    break;
                case PHASE_AST_SIGN_SMSTYPE:
                    strKey = std::string("*_") + pInfo->m_strSign + "_" + pInfo->m_strSmsType;
                    break;
                case PHASE_ACNT_AST_SMSTYPE:
                    strKey = pInfo->m_strClientId + "_*_" + pInfo->m_strSmsType;
                    break;
                case PHASE_AST_AST_SMSTYPE:
                    strKey = std::string("*_*_") + pInfo->m_strSmsType;
                    break;
                default:
                    strKey = "not exist";
                    break;
            }

            client_priorities_t::const_iterator cpIter = clientPriorities.find(strKey);
            if (cpIter != clientPriorities.end())
            {
                int iMQPri = MAX_CLIENT_PRIORITY - cpIter->second.m_iPriority;
                iMQPri = (iMQPri < 0) ? 0 : iMQPri;
                iMQPri = (iMQPri > MAX_CLIENT_PRIORITY) ? MAX_CLIENT_PRIORITY : iMQPri;
                LogNotice("[%s:%s] clientId[%s] smstype[%s] sign[%s] find priority key [%s] => value=[%d(max)-%d(cfg)]->[%d(mq_pri)]",
                        pInfo->m_strSmsId.data(), 
                        pInfo->m_strPhone.data(),
                        pInfo->m_strClientId.data(), 
                        pInfo->m_strSmsType.data(),
                        pInfo->m_strSign.data(),
                        strKey.c_str(),
                        MAX_CLIENT_PRIORITY,
                        cpIter->second.m_iPriority,
                        iMQPri);

                // covert user-config priority to MQ priority
                return iMQPri;
            }
        }
    } while(0);

    LogNotice("[%s:%s] clientId[%s] smstype[%s] sign[%s] use default channel priority value [%d]",
            pInfo->m_strSmsId.data(), 
            pInfo->m_strPhone.data(),
            pInfo->m_strClientId.data(), 
            pInfo->m_strSmsType.data(),
            pInfo->m_strSign.data(),
            iPriority);

    return iPriority;
}
