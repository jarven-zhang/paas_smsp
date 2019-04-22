#include "TpsAndHoldCount.h"
#include "main.h"
#include "UrlCode.h"
#include "base64.h"
#include "Uuid.h"
#include "ssl/md5.h"
#include "boost/foreach.hpp"
#include "MqManagerThread.h"
#include "commonmessage.h"
#include "BusTypes.h"
#include "global.h"
#include "Fmt.h"

string HTTPS_RESPONSE_MSG_0 = "发送成功";                     ////0
string HTTPS_RESPONSE_MSG_1 = "账号不存在";                   ////-1
string HTTPS_RESPONSE_MSG_2 = "账号余额不足";                 ////-2
string HTTPS_RESPONSE_MSG_3 = "账号被禁用";                   ////-3
string HTTPS_RESPONSE_MSG_4 = "账号被锁定";                   ////-4
string HTTPS_RESPONSE_MSG_5 = "ip鉴权失败";                   ////-5
string HTTPS_RESPONSE_MSG_6 = "缺少请求参数或参数不正确";       ////-6
string HTTPS_RESPONSE_MSG_7 = "手机号码格式不正确";            ////-7
string HTTPS_RESPONSE_MSG_8 = "短信内容超长或签名超长";         ////-8
string HTTPS_RESPONSE_MSG_9 = "签名未报备";                   ////-9

string HTTPS_RESPONSE_MSG_15 = "模板ID不存在";    //-15
string HTTPS_RESPONSE_MSG_16 = "模板参数个数不匹配或参数超长或含有非法字符";    //-16

int ACCESS_HTTPSERVER_RETURNCODE_ACCOUNTNotEXIST = -1;
int ACCESS_HTTPSERVER_RETURNCODE_NOFEE = -2;
int ACCESS_HTTPSERVER_RETURNCODE_ACCOUNTFORBIDDEN = -3;
int ACCESS_HTTPSERVER_RETURNCODE_ACCOUNTLOCKED = -4;
int ACCESS_HTTPSERVER_RETURNCODE_IPERR = -5;
int ACCESS_HTTPSERVER_RETURNCODE_PARAMERR = -6;
int ACCESS_HTTPSERVER_RETURNCODE_PHONEERR = -7;
int ACCESS_HTTPSERVER_RETURNCODE_ERRORLENGTH = -8;
int ACCESS_HTTPSERVER_RETURNCODE_SIGNNOTREGISTED = -9;

int ACCESS_HTTPSERVER_RETURNCODE_ERRORTEMPLATEID = -15;
int ACCESS_HTTPSERVER_RETURNCODE_ERRORTEMPLATEPARAM = -16;


#define foreach BOOST_FOREACH

#define CHK_INT_BRK(var, min, max)                                          \
    do{                                                                     \
        if (((var) < (min)) || ((var) > (max)))                             \
        {                                                                   \
            LogError("Invalid system parameter(%s) value(%s), %s(%d).",     \
                strSysPara.c_str(),                                         \
                iter->second.c_str(),                                       \
                VNAME(var),                                                 \
                var);                                                       \
            break;                                                          \
        }                                                                   \
    }while(0);


#define MERGE_LONG_GET_COUNT                        0xF001
#define MERGE_LONG_GET_SHORTS                       0xF002
#define MERGE_LONG_GET_FIRST_RSP_CODE_AND_SMSID     0XF003
#define MERGE_LONG_TIMEOUT_GET_SHORTS               0xF004

#define MERGE_LONG_REDIS_TIMER_ID                   0xE004
#define MERGE_LONG_REDIS_GETALL_TIMER_ID            0xE005


CUnityThread::CUnityThread(const char* name):CThread(name)
{
    // IOMQERR_MAXCHCHESIZE default: 3000000
    m_uIOProduceMaxChcheSize = 3000*1000;

    m_uiLongSmsMergeTimeOut = 10;
}

CUnityThread::~CUnityThread()
{

}

bool CUnityThread::Init(UInt64 SwitchNumber)
{
    if (false == CThread::Init())
    {
        printf("CDispatchThread::init -> CThread::Init is failed.\n");
        return false;
    }

    m_honeDao.Init();

    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    m_mapClientSignPort.clear();
    g_pRuleLoadThread->getSmsClientAndSignMap(m_mapClientSignPort);

    m_userGwMap.clear();
    g_pRuleLoadThread->getUserGwMap(m_userGwMap);

    m_mqConfigInfo.clear();
    g_pRuleLoadThread->getMqConfig(m_mqConfigInfo);

    m_componentRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);
    // Init MQ Group WeightInfo
    updateMQWeightGroupInfo(m_componentRefMqInfo);

    m_mapSystemErrorCode.clear();
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);

    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    getSysPara(sysParamMap);

    m_mqId.clear();
    for (map<string,ComponentRefMq>::iterator itr = m_componentRefMqInfo.begin();itr !=m_componentRefMqInfo.end(); ++itr)
    {
        if (g_uComponentId == itr->second.m_uComponentId)
        {
            m_mqId.insert(itr->second.m_uMqId);
        }
    }

    m_uSwitchNumber = SwitchNumber;

    time_t now = time(NULL);
    struct tm today =  {0};
    localtime_r(&now,&today);
    int seconds;
    today.tm_hour = 0;
    today.tm_min = 0;
    today.tm_sec = 0;
    seconds = 24*60*60 - difftime(now,mktime(&today));
    m_UnlockTimer = SetTimer(ACCOUNT_UNLOCK_TIMER_ID, "", seconds*1000);

    PropertyUtils::GetValue("is_need_statistic", m_uIsSendForStat);
    LogDebug("Get m_uIsSendForStat[%d].", m_uIsSendForStat);

    for (int i = 0; i < SMS_FROM_ACCESS_MAX; ++i)
    {
        for (int j = 0; j < TCP_RESULT_ERROR_MAX; ++j)
        {
            m_arrErrorCodeMap[i][j] = 0;
            m_arrLoginErrorCodeMap[i][j] = 0;
        }
    }

    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_OK]               = CMPP_SUCCESS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_MSG_FORMAT_ERROR] = CMPP_ERROR_MSG_FORMAT;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_INVALID_SRC]      = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_INVALID_DEST]     = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_AUTH_ERROR]       = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_VERSION]          = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_PARAM_ERROR]      = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_MSG_LENGTH_ERROR] = CMPP_ERROR_MSG_LENGTH;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_OTHER_ERROR]      = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_FLOWCTRL]         = CMPP_ERROR_FLOW_CONCTROL;

    //submitresp status
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_OK]               = CMPP_SUCCESS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_MSG_FORMAT_ERROR] = CMPP_ERROR_MSG_FORMAT;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_INVALID_SRC]      = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_INVALID_DEST]     = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_AUTH_ERROR]       = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_VERSION]          = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_PARAM_ERROR]      = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_MSG_LENGTH_ERROR] = CMPP_ERROR_MSG_LENGTH;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_OTHER_ERROR]      = CMPP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_FLOWCTRL]         = CMPP_ERROR_FLOW_CONCTROL;

    //loginresp status
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_OK]               = CMPP_LOGIN_RESULT_OK;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_MSG_FORMAT_ERROR] = CMPP_LOGIN_RESULT_MSG_FORMAT_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_INVALID_SRC]      = CMPP_LOGIN_RESULT_INVALID_SRC;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_INVALID_DEST]     = CMPP_LOGIN_RESULT_OTHER_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_AUTH_ERROR]       = CMPP_LOGIN_RESULT_AUTH_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_VERSION]          = CMPP_LOGIN_RESULT_VERSION;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_PARAM_ERROR]      = CMPP_LOGIN_RESULT_OTHER_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_MSG_LENGTH_ERROR] = CMPP_LOGIN_RESULT_OTHER_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][TCP_RESULT_OTHER_ERROR]      = CMPP_LOGIN_RESULT_OTHER_ERROR;

    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_OK]               = CMPP_LOGIN_RESULT_OK;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_MSG_FORMAT_ERROR] = CMPP_LOGIN_RESULT_MSG_FORMAT_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_INVALID_SRC]      = CMPP_LOGIN_RESULT_INVALID_SRC;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_INVALID_DEST]     = CMPP_LOGIN_RESULT_OTHER_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_AUTH_ERROR]       = CMPP_LOGIN_RESULT_AUTH_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_VERSION]          = CMPP_LOGIN_RESULT_VERSION;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_PARAM_ERROR]      = CMPP_LOGIN_RESULT_OTHER_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_MSG_LENGTH_ERROR] = CMPP_LOGIN_RESULT_OTHER_ERROR;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][TCP_RESULT_OTHER_ERROR]      = CMPP_LOGIN_RESULT_OTHER_ERROR;


    /*SGIP 协议内部错误码与标准错误码对应关系*/
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_OK]               = SGIP_SUCCESS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_MSG_FORMAT_ERROR] = SGIP_ERROR_PARAM_FORMAT;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_INVALID_SRC]      = SGIP_ERROR_ILLEGAL_TERMID;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_INVALID_DEST]     = SGIP_ERROR_ILLEGAL_TERMID;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_AUTH_ERROR]       = SGIP_ERROR_ILLEGAL_LOGIN;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_VERSION]          = SGIP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_PARAM_ERROR]      = SGIP_ERROR_PARAM_FORMAT;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_MSG_LENGTH_ERROR] = SGIP_ERROR_MSG_LENGTH;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_OTHER_ERROR]      = SGIP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_FLOWCTRL]         = SGIP_ERROR_NODE_BUSY;

    //loginresp status
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_OK]               = SGIP_SUCCESS;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_MSG_FORMAT_ERROR] = SGIP_ERROR_PARAM_FORMAT;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_INVALID_SRC]      = SGIP_ERROR_ILLEGAL_LOGIN;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_INVALID_DEST]     = SGIP_ERROR_ILLEGAL_LOGIN;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_AUTH_ERROR]       = SGIP_ERROR_ILLEGAL_LOGIN;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_VERSION]          = SGIP_ERROR_ILLEGAL_LOGIN;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_PARAM_ERROR]      = SGIP_ERROR_PARAM_FORMAT;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_MSG_LENGTH_ERROR] = SGIP_ERROR_MSG_LENGTH;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][TCP_RESULT_OTHER_ERROR]      = SGIP_ERROR_SYS;

    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_OK]               = SMGP_SUCCESS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_MSG_FORMAT_ERROR] = SMGP_ERROR_ILLEGAL_MSG_FORMAT;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_INVALID_SRC]      = SMGP_ERROR_ILLEGAL_SRC_TERMID;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_INVALID_DEST]     = SMGP_ERROR_ILLEGAL_DEST_TERMID;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_AUTH_ERROR]       = SMGP_ERROR_AUTH;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_VERSION]          = SMGP_ERROR_VERSION;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_PARAM_ERROR]      = SMGP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_MSG_LENGTH_ERROR] = SMGP_ERROR_ILLEGAL_MSG_LENGTH;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_OTHER_ERROR]      = SMGP_ERROR_SYS;
    m_arrErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_FLOWCTRL]         = SMGP_ERROR_SYS_BUSY;

    //loginresp status
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_OK]               = SMGP_SUCCESS;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_MSG_FORMAT_ERROR] = SMGP_ERROR_MSG_FORMAT;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_INVALID_SRC]      = SMGP_ERROR_AUTH;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_INVALID_DEST]     = SMGP_ERROR_AUTH;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_AUTH_ERROR]       = SMGP_ERROR_AUTH;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_VERSION]          = SMGP_ERROR_VERSION;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_PARAM_ERROR]      = SMGP_ERROR_MSG_FORMAT;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_MSG_LENGTH_ERROR] = SMGP_ERROR_ILLEGAL_MSG_LENGTH;
    m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][TCP_RESULT_OTHER_ERROR]      = SMGP_ERROR_SYS;

    // SMGP
    m_arrErrorCodeMap[BusType::SMS_FROM_ACCESS_SMPP][TCP_RESULT_FLOWCTRL]   = SMPP_MSGQFUL;

    return true;
}

void CUnityThread::getSysPara(const map<string, string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "IOMQERR_MAXCHCHESIZE";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        Int32 iIOProduceMaxChcheSize = atoi(iter->second.c_str());

        CHK_INT_BRK(iIOProduceMaxChcheSize, 0, 1000 * 1000 * 1000);

        m_uIOProduceMaxChcheSize = iIOProduceMaxChcheSize;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uIOProduceMaxChcheSize);

    do
    {
        strSysPara = "LONGSMS_MERGE_TIMEOUT";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        Int32 iLongSmsMergeTimeOut = atoi(iter->second.c_str());

        CHK_INT_BRK(iLongSmsMergeTimeOut, 1, 90);

        m_uiLongSmsMergeTimeOut = iLongSmsMergeTimeOut; // second
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uiLongSmsMergeTimeOut);

}


void CUnityThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        TMsg* pMsg = NULL;
        pthread_mutex_lock(&m_mutex);
        pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL)
        {
            usleep(10);
        }
        else if(pMsg != NULL)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }
}


void CUnityThread::HandleMsg(TMsg* pMsg)
{
    if(NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_TCP_LOGIN_REQ:
        {
            HandleTcpLoginReq(pMsg);
            break;
        }
        case MSGTYPE_TCP_SUBMIT_REQ:
        {
            HandleSubmitReq(pMsg);
            break;
        }
        case MSGTYPE_TCP_DISCONNECT_REQ:
        {
            HandleDisConnectLinkReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
        {
            TUpdateSystemErrorCodeReq* pHttp = (TUpdateSystemErrorCodeReq*)pMsg;
            m_mapSystemErrorCode.clear();
            m_mapSystemErrorCode = pHttp->m_mapSystemErrorCode;
            LogNotice("update t_sms_system_error_desc size:%d.",m_mapSystemErrorCode.size());
            break;
        }
        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
        {
            TUpdateUserGwReq* msg = (TUpdateUserGwReq*)pMsg;
            LogNotice("RuleUpdate userGwMap update. map.size[%d]", msg->m_UserGwMap.size());
            m_userGwMap.clear();
            m_userGwMap = msg->m_UserGwMap;
            break;
        }
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            LogNotice("RuleUpdate account update!");
            HandleAccountUpdateReq(pMsg);
            break;
        }

        case MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ:
        {
            TUpdateClientIdAndSignReq* pReq= (TUpdateClientIdAndSignReq*)pMsg;
            LogNotice("RuleUpdate ClientAndSign update. size[%d]", pReq->m_ClientIdAndSignMap.size());
            m_mapClientSignPort.clear();
            m_mapClientSignPort = pReq->m_ClientIdAndSignMap;
            break;
        }
        case MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ:
        {
            TUpdateOperaterSegmentReq* pOperater = (TUpdateOperaterSegmentReq*)pMsg;
            LogNotice("RuleUpdate t_sms_operater_segment update. size[%d]", pOperater->m_OperaterSegmentMap.size());
            m_honeDao.m_OperaterSegmentMap.clear();
            m_honeDao.m_OperaterSegmentMap = pOperater->m_OperaterSegmentMap;
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* msg = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == msg)
            {
                break;
            }

            getSysPara(msg->m_SysParamMap);
            break;
        }
        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqConfig = (TUpdateMqConfigReq*)pMsg;
            LogNotice("RuleUpdate Mq config update. size[%d]", pMqConfig->m_mapMqConfig.size());
            m_mqConfigInfo.clear();
            m_mqConfigInfo = pMqConfig->m_mapMqConfig;
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            TUpdateComponentRefMqReq* pMqConfig =(TUpdateComponentRefMqReq*)pMsg;
            LogNotice("RuleUpdate component ref Mq  update. size[%d]", pMqConfig->m_componentRefMqInfo.size());
            m_componentRefMqInfo.clear();
            m_componentRefMqInfo = pMqConfig->m_componentRefMqInfo;
            updateMQWeightGroupInfo(m_componentRefMqInfo);
            break;
        }
        case MSGTYPE_GET_LINKED_CLIENT_REQ:
        {
            HandleGetLinkedClientReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_CLIENTFORCEEXIT_UPDATE_REQ:
        {
            HandleClientForceExit(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOut(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            handleRedisRsp(pMsg);
            break;
        }
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
    return;
}

void CUnityThread::proSetInfoToRedis(SMSSmsInfo* pSmsInfo)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return;
    }
    string strSubmits = "";
    string strSgipRtAndMoSubmits = "";

    UInt32 uShortInfoSize = pSmsInfo->m_vecShortInfo.size();
    if (SMS_FROM_ACCESS_SGIP == pSmsInfo->m_uSmsFrom)
    {
        for (UInt32 i = 0; i < uShortInfoSize; i++)
        {
            if (i == 0)
            {
                strSgipRtAndMoSubmits.assign(pSmsInfo->m_vecShortInfo.at(i).m_strSgipNodeAndTimeAndId);
            }
            else
            {
                strSgipRtAndMoSubmits.append("&");
                strSgipRtAndMoSubmits.append(pSmsInfo->m_vecShortInfo.at(i).m_strSgipNodeAndTimeAndId);
            }
        }
    }
    else
    {
        for (UInt32 i = 0; i < uShortInfoSize; i++)
        {
            char charSubmitIds[64] = {0};
            if(i == 0)
            {
                snprintf(charSubmitIds, 64,"%lu", pSmsInfo->m_vecShortInfo.at(i).m_uSubmitId);
                strSubmits = charSubmitIds;
            }
            else
            {
                snprintf(charSubmitIds, 64,"%lu", pSmsInfo->m_vecShortInfo.at(i).m_uSubmitId);
                strSubmits.append("&");
                strSubmits.append(charSubmitIds);
            }
        }
    }


    UInt32 uPhoneListSize = pSmsInfo->m_Phonelist.size();
    for (UInt32 i = 0; i < uPhoneListSize; i++)
    {
        string strRdsKey = "";
        strRdsKey.append("accesssms:");
        strRdsKey.append(pSmsInfo->m_strSmsId);
        strRdsKey.append("_");
        strRdsKey.append(pSmsInfo->m_Phonelist.at(i));

        string strCmd = "";
        strCmd.append("HMSET  ");
        strCmd.append(strRdsKey);

        pSmsInfo->m_vecLongMsgContainIDs.clear();

        string strIDs = "";
        for (UInt32 j = 0; j < uShortInfoSize; j++)
        {
            if(strIDs.empty())
            {
                string stridtmp = pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[pSmsInfo->m_Phonelist.at(i)];
                strIDs = stridtmp;
                pSmsInfo->m_vecLongMsgContainIDs.push_back(stridtmp);
            }
            else
            {
                string stridtmp = pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[pSmsInfo->m_Phonelist.at(i)];
                strIDs.append(",");
                strIDs.append(stridtmp);
                pSmsInfo->m_vecLongMsgContainIDs.push_back(stridtmp);
            }
        }



        if (SMS_FROM_ACCESS_HTTP == pSmsInfo->m_uSmsFrom)
        {
            strCmd.append(" submitid ");
            strCmd.append("0");
            strCmd.append(" clientid ");
            strCmd.append(pSmsInfo->m_strClientId);
            strCmd.append(" uid ");
            ////strCmd.append(pSmsInfo->m_strUid);
            string strUid = "";
            if (pSmsInfo->m_strUid.empty())
            {
                strUid.assign("NULL");
            }
            else
            {
                strUid.assign(pSmsInfo->m_strUid);
            }
            strCmd.append(strUid);
            strCmd.append(" id ");
            strCmd.append(strIDs);

            if(pSmsInfo->m_strDeliveryUrl.empty())
            {
                pSmsInfo->m_strDeliveryUrl = "NULL";
            }
            strCmd.append(" deliveryurl ");
            strCmd.append(pSmsInfo->m_strDeliveryUrl);

            strCmd.append(" date ");
            strCmd.append(pSmsInfo->m_strDate);
        }
        else if (SMS_FROM_ACCESS_SGIP == pSmsInfo->m_uSmsFrom)
        {
            strCmd.append(" submitid ");
            strCmd.append(strSgipRtAndMoSubmits);
            strCmd.append(" clientid ");
            strCmd.append(pSmsInfo->m_strClientId);
            strCmd.append(" id ");
            strCmd.append(strIDs);
            strCmd.append(" date ");
            strCmd.append(pSmsInfo->m_strDate);
        }
        else
        {
            strCmd.append(" submitid ");
            strCmd.append(strSubmits);
            strCmd.append(" clientid ");
            strCmd.append(pSmsInfo->m_strClientId);
            strCmd.append(" id ");
            strCmd.append(strIDs);
            strCmd.append(" date ");
            strCmd.append(pSmsInfo->m_strDate);

            if(SMS_FROM_ACCESS_SMPP == pSmsInfo->m_uSmsFrom)//smpp协议专用
            {
                strCmd.append(" destaddrton ");
                strCmd.append(Comm::int2str(pSmsInfo->m_uDestAddrTon));

                strCmd.append(" destaddrnpi ");
                strCmd.append(Comm::int2str(pSmsInfo->m_uDestAddrNpi));

                strCmd.append(" sourceaddrton ");
                strCmd.append(Comm::int2str(pSmsInfo->m_uSourceAddrTon));

                strCmd.append(" sourceaddrnpi ");
                strCmd.append(Comm::int2str(pSmsInfo->m_uSourceAddrNpi));
            }
            strCmd.append(" connectLink ");// for report push to origin link
            strCmd.append(pSmsInfo->m_vecShortInfo.at(0).m_strLinkId);
        }

        LogDebug("[%s: ] redis cmd[%s], length[%d].",pSmsInfo->m_strSmsId.data(),strCmd.data(),strCmd.length());
        TRedisReq* pRed = new TRedisReq();
        pRed->m_RedisCmd = strCmd;
        pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRed->m_strKey= strRdsKey;
        //g_pRedisThreadPool->PostMsg(pRed);
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);

        TRedisReq* pRedisExpire = new TRedisReq();
        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisExpire->m_strKey= strRdsKey;
        pRedisExpire->m_RedisCmd.assign("EXPIRE ");
        pRedisExpire->m_RedisCmd.append(strRdsKey);
        pRedisExpire->m_RedisCmd.append("  259200");
        //g_pRedisThreadPool->PostMsg(pRedisExpire);
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);

        string strMQdate;

        strMQdate.append("clientId=");
        strMQdate.append(pSmsInfo->m_strClientId);

        strMQdate.append("&userName=");
        strMQdate.append(Base64::Encode(pSmsInfo->m_strUserName));

        strMQdate.append("&smsId=");
        strMQdate.append(pSmsInfo->m_strSmsId);

        strMQdate.append("&phone=");
        strMQdate.append(pSmsInfo->m_Phonelist.at(i));

        strMQdate.append("&sign=");
        strMQdate.append(Base64::Encode(pSmsInfo->m_strSign));

        strMQdate.append("&content=");
        strMQdate.append(Base64::Encode(pSmsInfo->m_strContent));

        strMQdate.append("&smsType=");
        strMQdate.append(pSmsInfo->m_strSmsType);

        strMQdate.append("&smsfrom=");
        strMQdate.append(Comm::int2str(pSmsInfo->m_uSmsFrom));

        strMQdate.append("&signport=");
        strMQdate.append(pSmsInfo->m_strSignPort);

        strMQdate.append("&paytype=");
        strMQdate.append(Comm::int2str(pSmsInfo->m_uPayType));

        // if the sign is manual-end-sign, convert showsigntype to front-sign
        UInt32 uiShowSignType = (1 == pSmsInfo->m_uiSignManualFlag) ? 1 : pSmsInfo->m_uShowSignType;
        strMQdate.append("&showsigntype=");
        strMQdate.append(Comm::int2str(uiShowSignType));

        strMQdate.append("&userextpendport=");
        strMQdate.append(pSmsInfo->m_strExtpendPort);

        utils::UTFString utfHelper;
        int iLength = utfHelper.getUtf8Length(pSmsInfo->m_strContentWithSign);
        UInt32 uiLongSms = (iLength > 70) ? 1 : 0;

        strMQdate.append("&longsms=");
        strMQdate.append(Comm::int2str(uiLongSms));

        string strIDs1 = "";
        vector<string> vecIDs = pSmsInfo->m_vecLongMsgContainIDs;
        for(vector<string>::iterator itids = vecIDs.begin(); itids != vecIDs.end(); itids++)
        {
            if(itids == vecIDs.begin())
            {
                strIDs1 = (*itids);
            }
            else
            {
                strIDs1.append(",");
                strIDs1.append(*itids);
            }
        }
        strMQdate.append("&ids=");
        strMQdate.append(strIDs1);

        UInt32 uiClientCount = (iLength > 70) ? ((iLength + 66) / 67) : 1;
        strMQdate.append("&clientcnt=");
        strMQdate.append(Comm::int2str(uiClientCount));

        strMQdate.append("&csid=");
        strMQdate.append(Comm::int2str(g_uComponentId));

        strMQdate.append("&csdate=");
        strMQdate.append(pSmsInfo->m_strDate);

        //模板相关参数
        strMQdate.append("&templateid=").append(pSmsInfo->m_strTemplateId);
        strMQdate.append("&channel_tempid=").append(pSmsInfo->m_strTemplateParam);
        strMQdate.append("&temp_params=").append(pSmsInfo->m_strTemplateType);
        //
        map<string,SmsAccount>::iterator iter = m_AccountMap.find(pSmsInfo->m_strClientId);
        if(iter == m_AccountMap.end())
        {
            LogElk("can not find account in account map! account is %s",pSmsInfo->m_strClientId.data());
            signalErrSmsPushRtAndDb(pSmsInfo,i);
            continue;
        }

        string strMessageType = "";
        UInt64 PhoneType = 0;
        PhoneType = m_honeDao.getPhoneType(pSmsInfo->m_Phonelist.at(i));
        UInt64 uSmsType = atoi(pSmsInfo->m_strSmsType.data());

        if (FOREIGN == PhoneType)
        {
            if (SMS_TYPE_USSD == uSmsType || SMS_TYPE_FLUSH_SMS == uSmsType
                || TEMPLATE_TYPE_HANG_UP_SMS == atoi((pSmsInfo->m_strTemplateType).c_str()))
            {
                LogElk("phone operator[%d] error for smstype[%d]", PhoneType, uSmsType);
                signalErrSmsPushRtAndDb(pSmsInfo,i);
                continue;
            }
        }

        if(iter->second.m_uClientLeavel == 2 || iter->second.m_uClientLeavel == 3)
        {
            if (YIDONG == PhoneType)
            {
                if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
                {
                    strMessageType.assign("02");
                }
                else
                {
                    strMessageType.assign("01");
                }
            }
            else if (DIANXIN == PhoneType)
            {
                if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
                {
                    strMessageType.assign("06");
                }
                else
                {
                    strMessageType.assign("05");
                }
            }
            else if ((LIANTONG == PhoneType) || (FOREIGN == PhoneType))
            {
                if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
                {
                    strMessageType.assign("04");
                }
                else
                {
                    strMessageType.assign("03");
                }
            }
            else
            {
                LogElk("phoneType:%u is invalid.",PhoneType);
                signalErrSmsPushRtAndDb2(pSmsInfo,i);
                continue;
            }
        }
        else if(iter->second.m_uClientLeavel == 1)
        {
            if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
            {
                strMessageType.assign("08");
            }
            else
            {
                strMessageType.assign("07");
            }

        }
        else
        {
            LogElk("client leavel is error[%d] ,client id is %s", iter->second.m_uClientLeavel, pSmsInfo->m_strClientId.data());
            signalErrSmsPushRtAndDb(pSmsInfo,i);
            continue;
        }


        mq_weightgroups_map_t::iterator itReq = m_mapMQWeightGroups.find(strMessageType);
        if (itReq == m_mapMQWeightGroups.end())
        {
            LogElk("MessageType:%s is not find in m_mapMQWeightGroups.",strMessageType.data());
            signalErrSmsPushRtAndDb(pSmsInfo,i);
            continue;
        }

        MQWeightDefaultCalculator calc;
        itReq->second.weightSort(calc);

        UInt64 uMqId = (*itReq->second.begin())->getData();
        // Get first mqId
        map<UInt64,MqConfig>::iterator itrMq = m_mqConfigInfo.find(uMqId);
        if (itrMq == m_mqConfigInfo.end())
        {
            LogElk("mqid:%lu is not find in m_mqConfigInfo.", uMqId);
            signalErrSmsPushRtAndDb(pSmsInfo,i);
            continue;
        }


        string strExchange = itrMq->second.m_strExchange;
        string strRoutingKey = itrMq->second.m_strRoutingKey;

        LogDebug("insert to rabbitMQ, smstype[%s], clientid[%s], phone[%s], strMQdate[%s]",
            pSmsInfo->m_strSmsType.c_str(),
            pSmsInfo->m_strClientId.data(),
            pSmsInfo->m_Phonelist.at(i).data(),
            strMQdate.data());

        //modify by fangjinxiong 20161023, check Mq link

//      UInt32 Queuesize = g_pMQC2sIOProducerThread->GetMsgSize();
//      if(Queuesize > m_uIOProduceMaxChcheSize && (g_pMQC2sIOProducerThread->m_linkState == false))
        UInt32 Queuesize = g_pMqManagerThread->getMqC2sIoPThdTotalQueueSize();
        if (Queuesize > m_uIOProduceMaxChcheSize)
        {
            //save old phone list
            std::vector<std::string> list = pSmsInfo->m_Phonelist;
            string curPhone = pSmsInfo->m_Phonelist.at(i);

            //update db
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = MQ_LINK_ERROR_AND_QUEUESIZE_TOO_LARGE;
            pSmsInfo->m_strYZXErrCode = MQ_LINK_ERROR_AND_QUEUESIZE_TOO_LARGE;
            pSmsInfo->m_Phonelist.clear();
            pSmsInfo->m_Phonelist.push_back(curPhone);
            UpdateRecord(pSmsInfo);

            //report to user, internal error
            if( pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3 ||
                pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP ||
                pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP ||
                pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMPP ||
                pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
            {
                //report tcp
                SendFailedDeliver(pSmsInfo);
            }
            else if(pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_HTTP)
            {
                //report http
                LogError("http messege no reason go there!");
            }

            LogWarn("MQlink error, and g_pMQIOProducerThread.queuesize[%d] > m_uIOProduceMaxChcheSize[%d], phone[%s]",
                Queuesize, m_uIOProduceMaxChcheSize, curPhone.c_str());
            //refill phonelist
            pSmsInfo->m_Phonelist = list;
        }
        else
        {
            publishMqC2sIoMsg(strExchange, strRoutingKey, strMQdate);
        }
    }

    return;
}

bool CUnityThread::signalErrSmsPushRtAndDb(SMSSmsInfo* pSmsInfo,UInt32 i)
{
    std::vector<std::string> list = pSmsInfo->m_Phonelist;
    string curPhone = pSmsInfo->m_Phonelist.at(i);

    //update db
    pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    pSmsInfo->m_strErrorCode = C2S_MQ_CONFIG_ERROR;
    pSmsInfo->m_strYZXErrCode = C2S_MQ_CONFIG_ERROR;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist.push_back(curPhone);
    UpdateRecord(pSmsInfo);

    //report to user, internal error
    if( pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3 ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMPP ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
    {
        //report tcp
        SendFailedDeliver(pSmsInfo);
    }
    else if(pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_HTTP)
    {
        //report http
        LogError("this is http report!");
    }

    LogWarn("sms push to mq is error");
    //refill phonelist
    pSmsInfo->m_Phonelist = list;
    return true;

}

bool CUnityThread::signalErrSmsPushRtAndDb2(SMSSmsInfo* pSmsInfo,UInt32 i)
{
    std::vector<std::string> list = pSmsInfo->m_Phonelist;
    string curPhone = pSmsInfo->m_Phonelist.at(i);

    //update db
    pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
    pSmsInfo->m_strYZXErrCode = PHONE_FORMAT_IS_ERROR;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist.push_back(curPhone);
    UpdateRecord(pSmsInfo);

    //report to user, internal error
    if( pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3 ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMPP ||
        pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
    {
        //report tcp
        SendFailedDeliver(pSmsInfo);
    }
    else if(pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_HTTP)
    {
        //report http
        LogError("this is http report!");
    }

    LogWarn("sms push to mq is error");
    //refill phonelist
    pSmsInfo->m_Phonelist = list;
    return true;

}

void CUnityThread::HandleDisConnectLinkReq(TMsg* pMsg)
{
    CTcpDisConnectLinkReqMsg* pDis = (CTcpDisConnectLinkReqMsg*)pMsg;

    AccountMap::iterator iterAccount = m_AccountMap.find(pDis->m_strAccount);
    if (iterAccount == m_AccountMap.end())
    {
        LogElk("account[%s] is not find in accountMap",pDis->m_strAccount.data());
        return;
    }

    SmsAccount& smsAccount = iterAccount->second;
    string strLinkPre;

    char strpre[6][8] = {"CMPP3","SMPP","CMPP","SGIP","SMGP"};
    string strFirstKey;
    strFirstKey.append(strpre[smsAccount.m_uSmsFrom - 1]);
    strFirstKey.append(pDis->m_strLinkId);
    std::map<string, LinkManager*>::iterator iterLink = m_LinkMap.find(strFirstKey);
    if (iterLink != m_LinkMap.end())
    {
        if (iterLink->second->m_iLinkState == LINK_CONNECTED)
        {
           smsAccount.m_uLinkCount--;
           SAFE_DELETE(iterLink->second);
           LogNotice("Account[%s] disconnect, LinkCount[%u], NodeNum[%u].",
               smsAccount.m_strAccount.data(),
               smsAccount.m_uLinkCount,
               smsAccount.m_uNodeNum);
           set<string>::iterator itr = smsAccount.m_setLinkId.find(pDis->m_strLinkId);
           if (itr != smsAccount.m_setLinkId.end())
           {
                smsAccount.m_setLinkId.erase(itr);
                LogNotice("account[%s] delete link[%s]", smsAccount.m_strAccount.data(), pDis->m_strLinkId.data());
           }
        }
       
    }
    else
    {
        LogError("Account[%s] disconnect, LinkCount[%u], NodeNum[%u], can not find link[%s],maybe have been released",
            smsAccount.m_strAccount.data(),
            smsAccount.m_uLinkCount,
            smsAccount.m_uNodeNum,strFirstKey.data());
    }
 
}

void CUnityThread::HandleTimeOut(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    ////long sms merge timeout
    if (LONG_MSG_WAIT_TIMER_ID == (UInt32)pMsg->m_iSeq)
    {
        LongSmsMap::iterator itrLong = m_mapLongSmsMng.find(pMsg->m_strSessionID);
        if(itrLong == m_mapLongSmsMng.end())
        {
            LogError("sessionId[%s]is not in m_mapLongSmsMng!", pMsg->m_strSessionID.data());
            return;
        }

        if (TCP_RESULT_OK == itrLong->second->m_uFirstShortSmsResp)
        {
            itrLong->second->m_strErrorCode = LONG_SMS_MERGE_TIMEOUT;
            itrLong->second->m_strYZXErrCode = LONG_SMS_MERGE_TIMEOUT;
            itrLong->second->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            UpdateRecord(itrLong->second);
            SendFailedDeliver(itrLong->second);
        }

        delete itrLong->second->m_pLongSmsTimer;
        delete itrLong->second;
        m_mapLongSmsMng.erase(itrLong);
    }
    else if (MERGE_LONG_REDIS_TIMER_ID == (UInt32)pMsg->m_iSeq)
    {
        UInt64 ulSession = Comm::strToUint64(pMsg->m_strSessionID);

        LongSmsRedisMap::iterator iter;
        CHK_MAP_FIND_UINT_RET(m_mapLongSmsMngRedis, iter, ulSession);

        SMSSmsInfo* pSmsInfo = iter->second;
        CHK_NULL_RETURN(pSmsInfo);

        SAFE_DELETE(pSmsInfo->m_pLongSmsRedisTimer);

        LogDebug("clientid(%s), smsid(%s). Redis merge longsms time out function process.",
            pSmsInfo->m_strClientId.data(),
            pSmsInfo->m_strSmsId.data());

        mergeLongGetAllMsg(pSmsInfo, MERGE_LONG_TIMEOUT_GET_SHORTS);
    }
    else if (MERGE_LONG_REDIS_GETALL_TIMER_ID == (UInt32)pMsg->m_iSeq)
    {
        UInt64 ulSession = Comm::strToUint64(pMsg->m_strSessionID);

        LongSmsRedisMap::iterator iter;
        CHK_MAP_FIND_UINT_RET(m_mapLongSmsMngRedis, iter, ulSession);

        SMSSmsInfo* pSmsInfo = iter->second;
        CHK_NULL_RETURN(pSmsInfo);

        SAFE_DELETE(pSmsInfo->m_pLongSmsGetAllTimer);

        LogDebug("clientid(%s), smsid(%s). Get all info again.",
            pSmsInfo->m_strClientId.data(),
            pSmsInfo->m_strSmsId.data());

        mergeLongGetAllMsg(pSmsInfo, MERGE_LONG_GET_SHORTS);
    }
    else if (ACCOUNT_UNLOCK_TIMER_ID == (UInt32)pMsg->m_iSeq)
    {
        if(NULL != m_UnlockTimer)
        {
            delete m_UnlockTimer;
        }
        m_UnlockTimer = SetTimer(ACCOUNT_UNLOCK_TIMER_ID, "", 24*60*60*1000);

        time_t nowTime = time(NULL);
        struct tm timenow = {0};
        localtime_r(&nowTime,&timenow);
        for (std::map<string, SmsAccount>::iterator iter = m_AccountMap.begin(); iter != m_AccountMap.end(); ++iter)
        {
            if(7 == iter->second.m_uStatus)
            {
                iter->second.m_uStatus = 0;
                char tempTime[64] = { 0 };
                /////if (timenow != 0)
                strftime(tempTime, sizeof(tempTime), "%Y%m%d%H%M%S", &timenow);

                string unlockTime;
                unlockTime.assign(tempTime);

                char sql[1024]  = {0};
                snprintf(sql, 1024,"update t_sms_account set status = 1 where clientid = '%s'", iter->second.m_strAccount.data());

                TDBQueryReq* updateAccountTable = new TDBQueryReq();
                updateAccountTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
                updateAccountTable->m_SQL.assign(sql);
                g_pDisPathDBThreadPool->PostMsg(updateAccountTable);

                snprintf(sql, 1024,"update t_sms_account_login_status set status = 1, unlocktime = '%s', unlockby = 'access',updatetime = '%s' where clientid = '%s' and status = 0",
                                unlockTime.data(), unlockTime.data(), iter->second.m_strAccount.data());
                TDBQueryReq* updateLoginStatusTable = new TDBQueryReq();
                updateLoginStatusTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
                updateLoginStatusTable->m_SQL.assign(sql);
                g_pDisPathDBThreadPool->PostMsg(updateLoginStatusTable);

            }
        }
    }

}

void CUnityThread::AccountConnectClose(SmsAccount& oldAccount)
{
    if(oldAccount.m_setLinkId.empty())
    {
        LogNotice("account[%s] do not have been logined", oldAccount.m_strAccount.c_str());
        return;
    }
    set<string>::iterator itr = oldAccount.m_setLinkId.begin();
    for(; itr != oldAccount.m_setLinkId.end(); itr++)
    {
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = oldAccount.m_strAccount;
        req->m_strLinkId = (*itr);
        LogWarn("account[%s] linkid[%s] will be closed,smsfrom[%d] ",req->m_strAccount.c_str(),req->m_strLinkId.c_str(),oldAccount.m_uSmsFrom);

        switch (oldAccount.m_uSmsFrom)
        {
            case SMS_FROM_ACCESS_CMPP3:
            {
                req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
                g_pCMPP3ServiceThread->PostMsg(req);
                break;
            }
            case SMS_FROM_ACCESS_SMPP:
            {
                req->m_iMsgType = MSGTYPE_SMPP_LINK_CLOSE_REQ;
                g_pSMPPServiceThread->PostMsg(req);
                break;
            }
            case SMS_FROM_ACCESS_CMPP:
            {
                req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
                g_pCMPPServiceThread->PostMsg(req);
                break;
            }
            case SMS_FROM_ACCESS_SGIP:
            {
                req->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
                g_pSGIPServiceThread->PostMsg(req);
                //close report link
                CSgipCloseReqMsg* pClose = new CSgipCloseReqMsg();
                pClose->m_strClientId.assign(oldAccount.m_strAccount);
                pClose->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
                g_pSgipRtAndMoThread->PostMsg(pClose);
                break;
            }
            case SMS_FROM_ACCESS_SMGP:
            {
                req->m_iMsgType = MSGTYPE_SMGP_LINK_CLOSE_REQ;
                g_pSMGPServiceThread->PostMsg(req);
                break;
            }
            default:
            {
                LogWarn("account[%s] linkid[%s],unknow smsfrom[%d]",req->m_strAccount.c_str(),req->m_strLinkId.c_str(),oldAccount.m_uSmsFrom);
                delete req;
                break;
            }
        }
    }
    return;
}

void CUnityThread::HandleClientForceExit(TMsg* pMsg)
{
     TUpdateClientForceExitReq* pMqClient =(TUpdateClientForceExitReq*)pMsg;
     LogNotice("RuleUpdate t_sms_client_force_exit update size[%d]", pMqClient->m_clientForceExitMap.size());
     std::map<string, int>::iterator it = pMqClient->m_clientForceExitMap.begin();
     for (; it!=pMqClient->m_clientForceExitMap.end(); it++)
     {
        if (it->second == 1)
        {
            std::map<string, int>::iterator itOld = m_clientForceExitMap.find(it->first);
            if ((itOld == m_clientForceExitMap.end())
                || (itOld != m_clientForceExitMap.end() && itOld->second == 0))
            {
                AccountMap::iterator newIter = m_AccountMap.find(it->first);
                if(newIter != m_AccountMap.end())
                {
                    AccountConnectClose(newIter->second);
                    if (newIter->second.m_setLinkId.empty())
                    {
                        LogDebug("maybe have been processed, not need to handle mysql");
                    }
                    else
                    {
                        newIter->second.m_setLinkId.clear();
                    }
                    
                }
                else
                {
                    LogError("account[%s] not exist, t_sms_client_force_exit config error", (it->first).c_str());
                }
                
                char sql[512] = {0};
                snprintf(sql, sizeof(sql),"update t_sms_client_force_exit set state = 0 where clientid='%s' and component_id=%lu;"
                    ,(it->first).c_str(), g_uComponentId);
                TDBQueryReq* pMsgDB = new TDBQueryReq();
                pMsgDB->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
                pMsgDB->m_SQL.assign(sql);
                g_pDisPathDBThreadPool->PostMsg(pMsgDB);
                LogNotice("sql[%s]", sql);
             
                itOld->second = 0;
                
            }
            else
            {
                LogNotice("account[%s] is processing,maybe mysql process slowly", (it->first).c_str());
            }
        }
     }
    
     m_clientForceExitMap.clear();
     m_clientForceExitMap = pMqClient->m_clientForceExitMap;
}

void CUnityThread::HandleAccountUpdateReq(TMsg* pMsg)
{
    TUpdateSmsAccontReq* pAccountUpdateReq= (TUpdateSmsAccontReq*)pMsg;

    AccountMap::iterator oldIter = m_AccountMap.begin();
    for( ;oldIter != m_AccountMap.end(); oldIter++)
    {
        AccountMap::iterator newIter = pAccountUpdateReq->m_SmsAccountMap.find(oldIter->first);
        if(newIter != pAccountUpdateReq->m_SmsAccountMap.end())
        {
            if (1 != newIter->second.m_uStatus)
            {
                newIter->second.m_uLoginErrorCount = 0;
            }
            else
            {
                newIter->second.m_uLoginErrorCount = oldIter->second.m_uLoginErrorCount;
            }

            newIter->second.m_uLinkCount = oldIter->second.m_uLinkCount;
            newIter->second.m_uLockTime = oldIter->second.m_uLockTime;
            if((oldIter->second.m_strPWD != newIter->second.m_strPWD) ||
                (oldIter->second.m_uSmsFrom != newIter->second.m_uSmsFrom) ||
                (oldIter->second.m_uStatus != newIter->second.m_uStatus))
            {
                LogWarn("old Account[%s] pwd[%s] smsfrom[%u] status[%u],new pwd[%s],smsfrom[%u],status[%u]",
                    oldIter->first.c_str(),oldIter->second.m_strPWD.c_str(),oldIter->second.m_uSmsFrom,oldIter->second.m_uStatus,
                    newIter->second.m_strPWD.c_str(),newIter->second.m_uSmsFrom,newIter->second.m_uStatus);
                AccountConnectClose(oldIter->second);
            }
            else
            {
                newIter->second.m_setLinkId.clear();
                newIter->second.m_setLinkId = oldIter->second.m_setLinkId;
            }
        }
    }
    m_AccountMap.clear();
    m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
}

void CUnityThread::HandleTcpLoginReq(TMsg* pMsg)
{
    CTcpLoginReqMsg* pLoginMsg = (CTcpLoginReqMsg*)pMsg;
    string strLinkPre;
    AccountMap::iterator iterAccount = m_AccountMap.find(pLoginMsg->m_strAccount);
    if (iterAccount == m_AccountMap.end())
    {
        LogElk("account[%s] is not find in accountMap",pLoginMsg->m_strAccount.data());
        SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
        return;
    }

    if (1 != iterAccount->second.m_uStatus)
    {
        LogElk("account[%s],status[%d] is not ok!",pLoginMsg->m_strAccount.data(),iterAccount->second.m_uStatus);
        SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
        return;
    }

    ////check protocol type wheather match
    if (pLoginMsg->m_uSmsFrom != iterAccount->second.m_uSmsFrom)
    {
        LogElk("clientId[%s],smsfrom[%d],logIn smsfrom[%d] is not match.",
            pLoginMsg->m_strAccount.data(),iterAccount->second.m_uSmsFrom,pLoginMsg->m_uSmsFrom);

        SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);

        iterAccount->second.m_uLoginErrorCount++;
        if(iterAccount->second.m_uLoginErrorCount >= 5)
        {
            string reason = http::UrlCode::UrlDecode(string("%e5%8d%8f%e8%ae%ae%e7%b1%bb%e5%9e%8b%e4%b8%8d%e5%8c%b9%e9%85%8d"));
            AccountLock(iterAccount->second, reason.c_str());
        }
        return;
    }

    if (iterAccount->second.m_uLinkCount >= iterAccount->second.m_uNodeNum)
    {
        LogElk("account[%s],linkCount[%u] is already equal than NodeNum[%u],can't increase links!",pLoginMsg->m_strAccount.data(),iterAccount->second.m_uLinkCount,iterAccount->second.m_uNodeNum);
        SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
        return;
    }
    /////check ip whitelist
    if (false == iterAccount->second.m_bAllIpAllow)
    {
            bool IsIPInWhiteList = false;
            for(list<string>::iterator it = iterAccount->second.m_IPWhiteList.begin(); it != iterAccount->second.m_IPWhiteList.end(); it++)
            {
                if(string::npos != pLoginMsg->m_strIp.find((*it), 0))
                {
                    IsIPInWhiteList = true;
                    break;
                }
            }
            if(false == IsIPInWhiteList)
            {
                LogElk("IP[%s] is not in whitelist.", pLoginMsg->m_strIp.data());
                SendLoginResp(pLoginMsg,TCP_RESULT_INVALID_SRC);

                /*   no need
                iterAccount->second.m_uLoginErrorCount++;
                if(iterAccount->second.m_uLoginErrorCount >= 5)
                {
                    AccountLock(iterAccount->second);
                }
                */
                return;
            }
        }
    else
    {
        LogDebug("account[%s] allowed all IP[%s]!",pLoginMsg->m_strAccount.data(),pLoginMsg->m_strIp.data());
    }

    string account = pLoginMsg->m_strAccount;
    string passwd  = iterAccount->second.m_strPWD;

    if (SMS_FROM_ACCESS_CMPP == pLoginMsg->m_uSmsFrom ||
        SMS_FROM_ACCESS_CMPP3 == pLoginMsg->m_uSmsFrom)
    {
        string AuthenSource;
        char strTime[80] = {0};
        UInt32 timeStamp = pLoginMsg->m_uTimeStamp;
        snprintf(strTime, 80,"%010d", timeStamp);

        unsigned char md5[16] = {0};
        char target[1024] = {0};
        char* pos = target;
        memcpy(pos, account.data(), account.length());
        pos += (account.length() + 9);
        memcpy(pos, passwd.data(), passwd.size());
        pos += passwd.size();
        memcpy(pos, strTime, 10);
        UInt32 targetLen = account.length() + 9 + passwd.size() + 10;
        MD5((const unsigned char*) target, targetLen, md5);

        if(0 != memcmp(md5, pLoginMsg->m_strAuthSource.data(), 16))
        {
            LogElk("md5 AuthenSource error: account[%s], passwd[%s], timeStamp[%s]!",account.data(), passwd.data(), strTime);
            SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
            iterAccount->second.m_uLoginErrorCount++;
            if(iterAccount->second.m_uLoginErrorCount >= 5)
            {
                string reason = http::UrlCode::UrlDecode(string("%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5%ef%bc%88%e8%b4%a6%e5%8f%b7%e6%88%96%e5%af%86%e7%a0%81%e9%94%99%e8%af%af%ef%bc%89"));
                AccountLock(iterAccount->second, reason.c_str());
            }
            return;
        }
        strLinkPre.assign("CMPP");
        if (SMS_FROM_ACCESS_CMPP3 == pLoginMsg->m_uSmsFrom)
            strLinkPre.assign("CMPP3");

    }
    else if (SMS_FROM_ACCESS_SMGP == pLoginMsg->m_uSmsFrom)
    {
        string AuthenSource;
        char strTime[80] = {0};
        UInt32 timeStamp = pLoginMsg->m_uTimeStamp;
        snprintf(strTime, 80,"%010d", timeStamp);

        unsigned char md5[16] = {0};
        char target[1024] = {0};
        char* pos = target;
        memcpy(pos, account.data(), account.length());
        pos += (account.length() + 7);
        memcpy(pos, passwd.data(), passwd.size());
        pos += passwd.size();
        memcpy(pos, strTime, 10);
        UInt32 targetLen = account.length() + 7 + passwd.size() + 10;
        MD5((const unsigned char*) target, targetLen, md5);

        if(0 != memcmp(md5, pLoginMsg->m_strAuthSource.data(), 16))
        {
            LogElk("md5 AuthenSource error: account[%s], passwd[%s], timeStamp[%s]!",account.data(), passwd.data(), strTime);
            SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
            iterAccount->second.m_uLoginErrorCount++;
            if(iterAccount->second.m_uLoginErrorCount >= 5)
            {
                string reason = http::UrlCode::UrlDecode(string("%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5%ef%bc%88%e8%b4%a6%e5%8f%b7%e6%88%96%e5%af%86%e7%a0%81%e9%94%99%e8%af%af%ef%bc%89"));
                AccountLock(iterAccount->second, reason.c_str());

            }
            return;
        }
        strLinkPre.assign("SMGP");

    }
    else if (SMS_FROM_ACCESS_SGIP == pLoginMsg->m_uSmsFrom)
    {
        if((0 != account.compare(pLoginMsg->m_strAccount)) ||(0 != passwd.compare(pLoginMsg->m_strSgipPassWd)))
        {
            LogElk("account check error:Account[%s],Tpasswd[%s],Spasswd[%s]",account.data(),pLoginMsg->m_strSgipPassWd.data(),passwd.data());
            SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
            iterAccount->second.m_uLoginErrorCount++;
            if(iterAccount->second.m_uLoginErrorCount >= 5)
            {
                string reason = http::UrlCode::UrlDecode(string("%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5%ef%bc%88%e8%b4%a6%e5%8f%b7%e6%88%96%e5%af%86%e7%a0%81%e9%94%99%e8%af%af%ef%bc%89"));
                AccountLock(iterAccount->second, reason.c_str());
            }

            return;
        }
        strLinkPre.assign("SGIP");
    }
    else /////smpp
    {
        if((0 != account.compare(pLoginMsg->m_strAccount)) ||(0 != passwd.compare(pLoginMsg->m_strPwd)))
        {
            LogElk("account check error:Account[%s],Tpasswd[%s],Spasswd[%s]",account.data(),pLoginMsg->m_strPwd.data(),passwd.data());

            SendLoginResp(pLoginMsg,TCP_RESULT_AUTH_ERROR);
            iterAccount->second.m_uLoginErrorCount++;
            if(iterAccount->second.m_uLoginErrorCount >= 5)
            {
                string reason = http::UrlCode::UrlDecode(string("%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5%ef%bc%88%e8%b4%a6%e5%8f%b7%e6%88%96%e5%af%86%e7%a0%81%e9%94%99%e8%af%af%ef%bc%89"));
                AccountLock(iterAccount->second, reason.c_str());
            }

            return;
        }
         strLinkPre.assign("SMPP");
    }

    LogNotice("==suc== clientid[%s],ip[%s] login success!", pLoginMsg->m_strAccount.data(),pLoginMsg->m_strIp.data());
    SendLoginResp(pLoginMsg,TCP_RESULT_OK);
    iterAccount->second.m_uLoginErrorCount = 0;
    iterAccount->second.m_uLinkCount++;
    iterAccount->second.m_setLinkId.insert(pLoginMsg->m_strLinkId);//login success,save the linkid for close this link
    LogDebug("Account[%s] login, m_uLinkCount[%d]", iterAccount->second.m_strAccount.data(), iterAccount->second.m_uLinkCount);

    string strFirstKey = strLinkPre + pLoginMsg->m_strLinkId;
    std::map<string, LinkManager*>::iterator iterLink = m_LinkMap.find(strFirstKey);
    if (iterLink == m_LinkMap.end())
    {
        LinkManager* pSession = new LinkManager();
        pSession->m_iLinkState = LINK_CONNECTED;
        pSession->m_strAccount = pLoginMsg->m_strAccount;
        m_LinkMap[strFirstKey] = pSession;
    }


    ///set mo ref c2s redis
    string cmdkey1 = "mocsid:";
    cmdkey1.append(account);

    string strCmd1 = "";
    strCmd1.append("HMSET  ");
    strCmd1.append(cmdkey1);
    strCmd1.append(" c2sid ");
    char strC2sId[16] = {0};
    snprintf(strC2sId,16,"%ld",g_uComponentId);
    strCmd1.append(strC2sId);
    ////smsfrom
    strCmd1.append("  smsfrom ");
    strCmd1.append(Comm::int2str(pLoginMsg->m_uSmsFrom));
    TRedisReq* pMoReq = new TRedisReq();
    pMoReq->m_RedisCmd = strCmd1;
    pMoReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pMoReq->m_strKey = cmdkey1;
    //g_pRedisThreadPool->PostMsg(pMoReq);
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pMoReq);


    TRedisReq* pMoExpire = new TRedisReq();
    pMoExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
    pMoExpire->m_strKey = cmdkey1;
    pMoExpire->m_RedisCmd.assign("EXPIRE ");
    pMoExpire->m_RedisCmd.append(cmdkey1);
    pMoExpire->m_RedisCmd.append("  259200");
    //g_pRedisThreadPool->PostMsg(pMoExpire);
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pMoExpire);
    LogDebug("set mo ref c2s redis ");

    return;
}

void CUnityThread::AccountLock(SmsAccount& accountInfo, const char* accountLockReason)
{
    /////LogDebug("account[%s]  login errorNum[%d] more than 5 times!",accountInfo.m_strAccount.data(),accountInfo.m_uLoginErrorCount);
    char sql[1024]  = {0};
    snprintf(sql, 1024,"update t_sms_account set status = 7 where clientid = '%s'", accountInfo.m_strAccount.data());

    TDBQueryReq* updateAccountTable = new TDBQueryReq();
    updateAccountTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    updateAccountTable->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(updateAccountTable);

    time_t now = time(NULL);
    accountInfo.m_uLockTime = now;
    LogElk("lock account[%s], locktime[%ld],login errorNum[%d] more than 5 times.",accountInfo.m_strAccount.data(), accountInfo.m_uLockTime,accountInfo.m_uLoginErrorCount);

    accountInfo.m_uStatus = 7;
    accountInfo.m_uLoginErrorCount = 0;

    struct tm timenow = {0};
    localtime_r(&now,&timenow);
    char tempTime[64] = {0};
    /////if (timenow != 0)
    {
        strftime(tempTime, sizeof(tempTime), "%Y%m%d%H%M%S", &timenow);
    }

    string locktime;
    locktime.assign(tempTime);

    snprintf(sql, 1024,"insert t_sms_account_login_status(clientid, remarks, locktime, status, updatetime) values('%s', '%s', '%s', '0', '%s')",
        accountInfo.m_strAccount.data(), accountLockReason, locktime.data(), locktime.data());
    TDBQueryReq* updateLoginStatusTable = new TDBQueryReq();
    updateLoginStatusTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    updateLoginStatusTable->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(updateLoginStatusTable);
}

void CUnityThread::SendLoginResp(CTcpLoginReqMsg* pLogMsg ,UInt32 uResult)
{
    CTcpLoginRespMsg* pResp = new CTcpLoginRespMsg();
    uResult = (uResult >= TCP_RESULT_ERROR_MAX) ? TCP_RESULT_OTHER_ERROR : uResult;
    pResp->m_strAccount = pLogMsg->m_strAccount;
    pResp->m_strLinkId = pLogMsg->m_strLinkId;
    pResp->m_uResult = uResult;
    pResp->m_uSequenceNum = pLogMsg->m_uSequenceNum;
    pResp->m_iMsgType = MSGTYPE_TCP_LOGIN_RESP;

    if (SMS_FROM_ACCESS_CMPP == pLogMsg->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP][uResult];
        g_pCMPPServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_CMPP3 == pLogMsg->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_CMPP3][uResult];
        g_pCMPP3ServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_SMPP == pLogMsg->m_uSmsFrom)
    {
        g_pSMPPServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_SMGP == pLogMsg->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SMGP][uResult];
        g_pSMGPServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_SGIP == pLogMsg->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_SGIP][uResult];
        pResp->m_uSequenceIdNode = pLogMsg->m_uSequenceIdNode;
        pResp->m_uSequenceIdTime = pLogMsg->m_uSequenceIdTime;
        pResp->m_uSequenceId = pLogMsg->m_uSequenceId;

        g_pSGIPServiceThread->PostMsg(pResp);
    }
    else
    {
        LogWarn("smsfrom[%u] is invalid!",pLogMsg->m_uSmsFrom);
    }

    return;
}

bool CUnityThread::getPortSign( string strClientId, string strSignPort, string &strSignOut )
{
    ClientIdAndSignMap::iterator itr = m_mapClientSignPort.begin();
    for( ;itr != m_mapClientSignPort.end(); itr++ )
    {
        if( strSignPort == itr->second.m_strSignPort
            && strClientId == itr->second.m_strClientId )
        {
            strSignOut = itr->second.m_strSign;
            LogDebug("ClientId:%s, SignPort:%s Match Result[ Sign:%s ] ",
                  strClientId.data(), strSignPort.data(),  strSignOut.data());
            return true;
        }
    }

    LogWarn("clientId[%s], strSignPort[%s] not find in t_sms_clientid_signport.",
            strClientId.data(), strSignPort.data());

    return false;
}

void CUnityThread::HandleSubmitReq(TMsg* pMsg)
{
    CTcpSubmitReqMsg* pReq = (CTcpSubmitReqMsg*)pMsg;

    AccountMap::iterator itrAccount = m_AccountMap.find(pReq->m_strAccount);
    if (itrAccount == m_AccountMap.end())
    {
        LogElk("account[%s] length[%d]is not find in accountMAP!",
            pReq->m_strAccount.data(),pReq->m_strAccount.length());
        return;
    }

    UInt64 iSubmitId = m_SnManager.CreateSubmitId(m_uSwitchNumber);
    string strSgipNodeAndTimeAndId = "";
    if (SMS_FROM_ACCESS_SGIP == pReq->m_uSmsFrom)
    {
        char cTemp[250] = {0};
        snprintf(cTemp,250,"%u|%u|%u",pReq->m_uSequenceIdNode,pReq->m_uSequenceIdTime,pReq->m_uSequenceId);
        strSgipNodeAndTimeAndId.assign(cTemp);
    }

    string strSmsId = getUUID();

    struct tm timenow = {0};
    time_t now = time(NULL);
    char submitTime[64] = { 0 };
    localtime_r(&now,&timenow);
    strftime(submitTime, sizeof(submitTime), "%Y%m%d%H%M%S", &timenow);


    LogElk("submit msg account[%s], submitid[%lu], linkid[%s]",
        pReq->m_strAccount.c_str(),
        iSubmitId,
        pReq->m_strLinkId.c_str());

    string utf8Content;
    ContentCoding(pReq->m_strMsgContent, pReq->m_uMsgFmt, utf8Content);

    ShortSmsInfo shortInfo;
    shortInfo.m_uPkNumber = pReq->m_uPkNumber;
    shortInfo.m_uPkTotal = pReq->m_uPkTotal;
    shortInfo.m_uSubmitId = iSubmitId;
    shortInfo.m_strSgipNodeAndTimeAndId.assign(strSgipNodeAndTimeAndId);
    shortInfo.m_strLinkId.assign(pReq->m_strLinkId);
    shortInfo.m_uSequenceNum = pReq->m_uSequenceNum;
    shortInfo.m_strShortContent = utf8Content;

    SMSSmsInfo* pSmsInfo = new SMSSmsInfo();
    pSmsInfo->m_strSmsId.assign(strSmsId);
    pSmsInfo->m_uSubmitId = iSubmitId;
    pSmsInfo->m_strSgipNodeAndTimeAndId.assign(strSgipNodeAndTimeAndId);
    pSmsInfo->m_strClientId.assign(itrAccount->second.m_strAccount);
    pSmsInfo->m_strSid.assign(itrAccount->second.m_strSid);
    pSmsInfo->m_strUserName.assign(itrAccount->second.m_strname);
    pSmsInfo->m_uNeedExtend = itrAccount->second.m_uNeedExtend;
    pSmsInfo->m_uAgentId = itrAccount->second.m_uAgentId;
    pSmsInfo->m_strSmsType = itrAccount->second.m_strSmsType;
    pSmsInfo->m_uNeedSignExtend = itrAccount->second.m_uNeedSignExtend;
    pSmsInfo->m_uPayType = itrAccount->second.m_uPayType;
    pSmsInfo->m_uOverRateCharge = itrAccount->second.m_uOverRateCharge;
    pSmsInfo->m_strExtpendPort.assign(pReq->m_strSrcId.data());             //from tcp client.
    pSmsInfo->m_strContent  = utf8Content;
    pSmsInfo->m_strSrcPhone.assign(pReq->m_strSrcId.data());
    pSmsInfo->m_uSmsFrom    = pReq->m_uSmsFrom;
    pSmsInfo->m_Phonelist = pReq->m_vecPhone;
    pSmsInfo->m_uIdentify = itrAccount->second.m_uIdentify;
    pSmsInfo->m_uBelong_sale = itrAccount->second.m_uBelong_sale;
    pSmsInfo->m_uAccountExtAttr = itrAccount->second.m_uAccountExtAttr;

    if (SMS_FROM_ACCESS_SMPP == pReq->m_uSmsFrom)
    {
        pSmsInfo->m_uDestAddrTon = pReq->m_uDestAddrTon;
        pSmsInfo->m_uDestAddrNpi = pReq->m_uDestAddrNpi;
        pSmsInfo->m_uSourceAddrTon = pReq->m_uSourceAddrTon;
        pSmsInfo->m_uSourceAddrNpi = pReq->m_uSourceAddrNpi;
    }

    for(vector<string>::iterator itp = pSmsInfo->m_Phonelist.begin(); itp != pSmsInfo->m_Phonelist.end(); itp++)
    {
        //rm 86 +86 and some
        if(m_honeDao.m_pVerify->CheckPhoneFormat(*itp))//格式错误的号码在插入数据库的时候生成uuid
        {
            map<string, string>::iterator itphone = shortInfo.m_mapPhoneRefIDs.find(*itp);  //
            if(itphone == shortInfo.m_mapPhoneRefIDs.end())
            {
                shortInfo.m_mapPhoneRefIDs[itp->data()] = getUUID();        //generator uuid for every phone in every submits
            }
            else
            {
                pSmsInfo->m_RepeatPhonelist.push_back(*itp);        //save repeat phones
            }
        }
    }

    pSmsInfo->m_strDate   = submitTime;
    pSmsInfo->m_uNeedAudit= itrAccount->second.m_uNeedaudit;
    pSmsInfo->m_uPkNumber = pReq->m_uPkNumber;
    pSmsInfo->m_uPkTotal = pReq->m_uPkTotal;
    pSmsInfo->m_uRandCode = pReq->m_uRandCode;
    pSmsInfo->m_uShortSmsCount = 1; ////start
    pSmsInfo->m_vecShortInfo.push_back(shortInfo);

    if (!checkSpeed(itrAccount->second, pSmsInfo))
    {
        SAFE_DELETE(pSmsInfo);
        return;
    }

    ////short messege
    if( pReq->m_uPkTotal == 1 )
    {
        pSmsInfo->m_uRecvCount++;
        pSmsInfo->m_uLongSms = 0;
        pSmsInfo->m_strContentWithSign = utf8Content;

        if(false == CheckMsgParam( pReq,pSmsInfo,utf8Content,itrAccount->second ))
        {
            LogElk("[%s:%s] Check msg param is failed!",
                pSmsInfo->m_strClientId.data(), pSmsInfo->m_strSmsId.data());
        }
        else
        {
            ////build msg send to chttpsendThread
            proSetInfoToRedis(pSmsInfo);
        }
        SAFE_DELETE( pSmsInfo );
        return;
    }
    ///long message
    else
    {
        if (pSmsInfo->m_uAccountExtAttr & ACCOUNT_EXT_MERGE_LONG_REDIS)
        {
            mergeLong(pReq, pSmsInfo, itrAccount->second);
            return;
        }

        pSmsInfo->m_uLongSms = 1;

        string longMsgKey("");
        longMsgKey.append(pSmsInfo->m_strClientId);
        longMsgKey.append("&");

        if( pSmsInfo->m_Phonelist.size() == 1 )
        {
            longMsgKey.append(pSmsInfo->m_Phonelist.at(0));
        }
        else
        {
            string strPhone = "";
            UInt32 uPhoneSize = pSmsInfo->m_Phonelist.size();
            for (UInt32 i = 0; i < uPhoneSize;i++)
            {
                strPhone.append(pSmsInfo->m_Phonelist.at(i));
            }

            string md5Phones;
            unsigned char md5[16] = { 0 };
            MD5((const unsigned char*) strPhone.c_str(), strPhone.length(), md5);
            std::string HEX_CHARS = "0123456789abcdef";
            for (int i = 0; i < 16; i++)
            {
                md5Phones.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
                md5Phones.append(1, HEX_CHARS.at(md5[i] & 0x0F));
            }

            longMsgKey.append(md5Phones);

        }

        char charRandCode[10] = {0};
        snprintf(charRandCode, 10,"%d", pSmsInfo->m_uRandCode);
        longMsgKey.append("&");
        longMsgKey.append(charRandCode);

        LongSmsMap::iterator itLongMsg = m_mapLongSmsMng.find(longMsgKey);
        if( itLongMsg == m_mapLongSmsMng.end())
        {
            LogDebug("[%s: ] insert long sms pkNumber[%d], pkTotal[%d], randCode[%d]!",
                    pSmsInfo->m_strSmsId.data(),
                    pSmsInfo->m_uPkNumber,
                    pSmsInfo->m_uPkTotal,
                    pSmsInfo->m_uRandCode);

            pSmsInfo->m_pLongSmsTimer = SetTimer(LONG_MSG_WAIT_TIMER_ID, longMsgKey, m_uiLongSmsMergeTimeOut * 1000);
            pSmsInfo->m_uRecvCount++;
            m_mapLongSmsMng[longMsgKey] = pSmsInfo;
            itLongMsg = m_mapLongSmsMng.find(longMsgKey);
        }
        else
        {
            itLongMsg->second->m_vecShortInfo.push_back(pSmsInfo->m_vecShortInfo.at(0));
            itLongMsg->second->m_uRecvCount++;
            itLongMsg->second->m_uShortSmsCount++;
            itLongMsg->second->m_Phonelist.clear();
            itLongMsg->second->m_Phonelist = pSmsInfo->m_Phonelist;
            SAFE_DELETE( pSmsInfo );
            if( itLongMsg->second->m_uRecvCount == itLongMsg->second->m_uPkTotal)
            {
                SAFE_DELETE( itLongMsg->second->m_pLongSmsTimer );
            }
        }

        if(false == CheckMsgParam(pReq,itLongMsg->second,utf8Content,itrAccount->second))
        {

            if(itLongMsg->second->m_uRecvCount == itLongMsg->second->m_uPkTotal)
            {
                SAFE_DELETE( itLongMsg->second );
                m_mapLongSmsMng.erase(itLongMsg);
            }
            return;
        }

        if(itLongMsg->second->m_uRecvCount == itLongMsg->second->m_uPkTotal )
        {
            if(itLongMsg->second->m_bIsNeedSendReport == false)
            {
                ///merge messege succeed
                if(NULL != mergeLongSms(itLongMsg->second))
                {
                    LogNotice("[%s: ] content[%s] merge long sms is success.",
                              itLongMsg->second->m_strSmsId.data(),itLongMsg->second->m_strContent.data());
                    //SendSubmitResp(pSmsInfo, TCP_RESULT_OK);
                    ////build msg send to chttpsendThread
                    if (false == CheckSignExtendAndKeyWord(itLongMsg->second))
                    {
                        LogElk("[%s:%s]CheckSignExtendAndKeyWord is failed!",
                            itLongMsg->second->m_strClientId.data(), itLongMsg->second->m_strSmsId.data());
                        itLongMsg->second->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
                        itLongMsg->second->m_strYZXErrCode = itLongMsg->second->m_strErrorCode;
                        UpdateRecord(itLongMsg->second);
                        SendFailedDeliver(itLongMsg->second);
                    }
                    else
                    {
                        proSetInfoToRedis(itLongMsg->second);
                    }

                    SAFE_DELETE( itLongMsg->second  );
                    m_mapLongSmsMng.erase(itLongMsg);

                }
                ///merge the messege failed
                else
                {
                    LogError("[%s: ] merge log messege error!",itLongMsg->second->m_strSmsId.data());
                    itLongMsg->second->m_strErrorCode = CAN_NOT_GET_ALL_SHORT_MSGS;
                    itLongMsg->second->m_strYZXErrCode = CAN_NOT_GET_ALL_SHORT_MSGS;
                    itLongMsg->second->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
                    UpdateRecord(itLongMsg->second); ////update all short messege
                    SendFailedDeliver(itLongMsg->second);
                    SAFE_DELETE( itLongMsg->second );
                    m_mapLongSmsMng.erase(itLongMsg);

                }
            }
            ///the first messege is ok,but other short messege is not ok,they all give the ok response to client.
            ///so we need to push failed report to client
            else
            {
                LogWarn("[%s: ]the first messege is ok,but other short messege is not ok,need send report!"
                    ,itLongMsg->second->m_strSmsId.data());
                //SendSubmitResp(pSmsInfo, TCP_RESULT_OK);
                itLongMsg->second->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
                itLongMsg->second->m_strErrorCode = CAN_NOT_GET_ALL_SHORT_MSGS;
                itLongMsg->second->m_strYZXErrCode = CAN_NOT_GET_ALL_SHORT_MSGS;
                UpdateRecord(itLongMsg->second);
                SendFailedDeliver(itLongMsg->second);

                delete itLongMsg->second;
                itLongMsg->second = NULL;
                m_mapLongSmsMng.erase(itLongMsg);
            }
        }
        else
        {
            //SendSubmitResp(itLongMsg->second, TCP_RESULT_OK);
            //InsertAccessDb(itLongMsg->second);
        }
    }
    return;
}

bool CUnityThread::checkSpeed(const SmsAccount& smsAccount, SMSSmsInfo* pSmsInfo)
{
    if (0 == smsAccount.m_uiSpeed)
    {
        LogDebug("[%s] No Speed Limit.", smsAccount.m_strAccount.data());
        return true;
    }

    if ((1 != pSmsInfo->m_uPkTotal) && (smsAccount.m_uAccountExtAttr & ACCOUNT_EXT_MERGE_LONG_REDIS))
    {
        LogDebug("[%s] No Speed Limit for long sms in redis.", smsAccount.m_strAccount.data());
        return true;
    }

    UInt32 uiPhoneNum = pSmsInfo->m_Phonelist.size();

    SpeedCtrlMapIter iter = m_mapSpeedCtrl.find(smsAccount.m_strAccount);
    if (m_mapSpeedCtrl.end() == iter)
    {
        LogDebug("[%s] Speed[%u] Check Start...",
            smsAccount.m_strAccount.data(),
            smsAccount.m_uiSpeed);

        // 群发场景下，1个TCP包，3个手机号码，假如设置阈值为2，因为是一个包提交上来的
        // 所以要么回成功响应要么回失败响应，这种场景下，按成功算，记下已发送短信数量
        // 下次继续计算
        SpeedCtrl speedCtrl;
        speedCtrl.m_uiRecvCount = uiPhoneNum;
        speedCtrl.m_ulCheckStart = now_microseconds();

        if (1 != pSmsInfo->m_uPkTotal)
        {
            string strKey = join(pSmsInfo->m_Phonelist, ",") + "_" + to_string(pSmsInfo->m_uRandCode);
            speedCtrl.m_setLongSmsKey.insert(strKey);
            LogDebug("[%s] save long sms key[%s].", smsAccount.m_strAccount.data(), strKey.data());
        }

        m_mapSpeedCtrl[smsAccount.m_strAccount] = speedCtrl;
        return true;
    }

    SpeedCtrl& speedCtrl = iter->second;
    UInt64 ulCheckEnd = now_microseconds();

    // 如果在1秒之内
    if (ulCheckEnd - speedCtrl.m_ulCheckStart < 1000000)
    {
        speedCtrl.m_uiRecvCount = speedCtrl.m_uiRecvCount + uiPhoneNum;

        // 如果超速了
        if (speedCtrl.m_uiRecvCount > smsAccount.m_uiSpeed)
        {
            // 如果是长短信的后续短信，则不返回失败
            if (1 != pSmsInfo->m_uPkTotal)
            {
                string strKey = join(pSmsInfo->m_Phonelist, ",") + "_" + to_string(pSmsInfo->m_uRandCode);
                LogDebug("[%s] Long sms key[%s].", smsAccount.m_strAccount.data(), strKey.data());

                if (speedCtrl.m_setLongSmsKey.end() != speedCtrl.m_setLongSmsKey.find(strKey))
                {
                    LogDebug("[%s] Long sms key[%s], recvCount[%u] over speed[%u] limit, "
                        "but not return error.",
                        smsAccount.m_strAccount.data(),
                        strKey.data(),
                        speedCtrl.m_uiRecvCount,
                        smsAccount.m_uiSpeed);
                    return true;
                }
            }

            LogElk("[%s] recvCount[%u] over speed[%u] limit.",
                smsAccount.m_strAccount.data(),
                speedCtrl.m_uiRecvCount,
                smsAccount.m_uiSpeed);

            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = TCP_FLOW_CTRL;
            pSmsInfo->m_strYZXErrCode = TCP_FLOW_CTRL;
            InsertAccessDb(pSmsInfo);

            SendSubmitResp(pSmsInfo, TCP_RESULT_FLOWCTRL);
            return false;
        }
    }
    else
    {
        speedCtrl.m_uiRecvCount = uiPhoneNum;
        speedCtrl.m_ulCheckStart = ulCheckEnd;
        speedCtrl.m_setLongSmsKey.clear();
    }

    if (1 != pSmsInfo->m_uPkTotal)
    {
        string strKey = join(pSmsInfo->m_Phonelist, ",") + "_" + to_string(pSmsInfo->m_uRandCode);
        speedCtrl.m_setLongSmsKey.insert(strKey);
        LogDebug("[%s] save long sms key[%s].", smsAccount.m_strAccount.data(), strKey.data());
    }

    LogDebug("[%s] RecvCount[%u].", smsAccount.m_strAccount.data(), speedCtrl.m_uiRecvCount);
    return true;
}

SMSSmsInfo* CUnityThread::mergeLongSms(SMSSmsInfo* pSmsInfo)
{
    if (NULL == pSmsInfo)
    {
        LogError("[%s: ] pSmsInfo is NULL!",pSmsInfo->m_strSmsId.data());
        return NULL;
    }

    string strLongContent = "";
    bool iSuccess = false;
    ////merge long sms content
    for (UInt32 iPkNumber = 1; iPkNumber <= pSmsInfo->m_uPkTotal; iPkNumber++)
    {
        iSuccess = false;
        UInt32 uInfoSize = pSmsInfo->m_vecShortInfo.size();
        for (UInt32 i = 0; i < uInfoSize;i++)
        {
            if (iPkNumber == pSmsInfo->m_vecShortInfo.at(i).m_uPkNumber)
            {
                strLongContent.append(pSmsInfo->m_vecShortInfo.at(i).m_strShortContent);
                iSuccess = true;
                break;
            }

        }

        if (false == iSuccess)
        {
            break;
        }
    }

    if (false == iSuccess)
    {
        LogElk("[%s: ] can not get all short msgs!",pSmsInfo->m_strSmsId.data());

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode= CAN_NOT_GET_ALL_SHORT_MSGS;
        return NULL;
    }

    pSmsInfo->m_strContent.assign(strLongContent);
    pSmsInfo->m_strContentWithSign.assign(strLongContent);

    return pSmsInfo;
}

bool CUnityThread::CheckSignExtendAndKeyWord(SMSSmsInfo* pSmsInfo)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return false;
    }

    ////check content empty
    if (pSmsInfo->m_strContent.empty())
    {
        LogError("[%s: ] content is empty!",pSmsInfo->m_strSmsId.data());
        ////pSmsInfo->m_strErrorCode.assign(strKeyWord);
        pSmsInfo->m_strErrorCode = CONTENT_EMPTY;
        pSmsInfo->m_iErrorCode = ACCESS_HTTPSERVER_RETURNCODE_PARAMERR;
        pSmsInfo->m_strError = HTTPS_RESPONSE_MSG_6;
        return false;
    }

    //// check sign extend
    if (false == CheckSignExtend(pSmsInfo))
    {
        LogError("[%s: ] account[%s] CheckSignExtend is failed!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strClientId.data());
        return false;
    }

    return true;
}

bool CUnityThread::CheckMsgParam(CTcpSubmitReqMsg* pReq,SMSSmsInfo* pSmsInfo,string& utf8Content,SmsAccount& AccountInfo)
{
    if (!Comm::isNumber(pSmsInfo->m_strExtpendPort))
    {
        LogError("[%s:%s] Invalid extend[%s], must numerical string.",
            pSmsInfo->m_strClientId.data(),
            pSmsInfo->m_strSmsId.data(),
            pSmsInfo->m_strExtpendPort.data());

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode =  CHINESE_1;
        pSmsInfo->m_strYZXErrCode = CHINESE_1;
        InsertAccessDb(pSmsInfo);

        SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);
        return false;
    }

    ////check account status
    if (1 != AccountInfo.m_uStatus)
    {
        LogElk("[%s: ] account[%s] status[%d] is not ok!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strClientId.data(),AccountInfo.m_uStatus);
        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode =  ACCOUNT_LOCKED;
        pSmsInfo->m_strYZXErrCode = ACCOUNT_LOCKED;
        InsertAccessDb(pSmsInfo);

        SendSubmitResp(pSmsInfo, TCP_RESULT_AUTH_ERROR);
        return false;
    }

    //CHECK ENCODE
    utils::UTFString utfHelper;
    char strDst[64] = {0};
    if(!utfHelper.IsTextUTF8(utf8Content.c_str(), utf8Content.length(), strDst))
    {
        LogElk("[%s: ] content[%x,%x,%x,%x,%x,%x] is over utf8!", pSmsInfo->m_strSmsId.data(), strDst[0], strDst[1], strDst[2], strDst[3], strDst[4], strDst[5]);

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = ERRORCODE_CONTENT_ENCODE_ERROR;
        pSmsInfo->m_strYZXErrCode = ERRORCODE_CONTENT_ENCODE_ERROR;
        InsertAccessDb(pSmsInfo);

        SendSubmitResp(pSmsInfo, TCP_RESULT_ERROR_MAX);
        return false;
    }


    ////check msglength
    UInt32 msgLength = 0;
    UInt32 maxWord = 0;
    msgLength = utfHelper.getUtf8Length(utf8Content);
    if (true == Comm::IncludeChinese((char*)utf8Content.data()))
    {
        maxWord = 70;
    }
    else
    {
        maxWord = 140;
    }

    if(pReq->m_uMsgLength > 140 || msgLength > maxWord)
    {
        LogElk("[%s: ] content length[%d] or msglenth[%u]is too long!",pSmsInfo->m_strSmsId.data(),pReq->m_uMsgLength,msgLength);

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = CONTENT_LENGTH_TOO_LONG  ;
        pSmsInfo->m_strYZXErrCode = CONTENT_LENGTH_TOO_LONG ;
        InsertAccessDb(pSmsInfo);

        SendSubmitResp(pSmsInfo, TCP_RESULT_MSG_LENGTH_ERROR);
        return false;
    }


    ////check phone num
    if (pReq->m_uPhoneNum > 100)
    {
        LogElk("[%s: ] phone count[%d] is too many!",pSmsInfo->m_strSmsId.data(),pReq->m_uPhoneNum);

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = PHONE_COUNT_TOO_MANY;
        pSmsInfo->m_strYZXErrCode = PHONE_COUNT_TOO_MANY;
        InsertAccessDb(pSmsInfo);

        SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);
        return false;
    }

    //long msg
    if(1 == pSmsInfo->m_uLongSms)
    {
        //check pktotal
        if (pReq->m_uPkTotal > 10)
        {
            LogElk("[%s: ] pktotal[%d] is too many!",pSmsInfo->m_strSmsId.data(),pReq->m_uPkTotal);

            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = PKTOTAL_TOO_MANY;
            pSmsInfo->m_strYZXErrCode = PKTOTAL_TOO_MANY;
            InsertAccessDb(pSmsInfo);

            SendSubmitResp(pSmsInfo, TCP_RESULT_OTHER_ERROR);
            return false;
        }

        if (pSmsInfo->m_uPkNumber > pSmsInfo->m_uPkTotal)
        {
            LogElk("[%s: ] account[%s] pkNumber[%d] > pkTotal[%d]!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strClientId.data(),pSmsInfo->m_uPkNumber,pSmsInfo->m_uPkTotal);

            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = PKNUMBER_BIG_PKTOTAL;
            pSmsInfo->m_strYZXErrCode = PKNUMBER_BIG_PKTOTAL;
            InsertAccessDb(pSmsInfo);
            SendSubmitResp(pSmsInfo, TCP_RESULT_OTHER_ERROR);
            return false;
        }
    }

    ////check process
    return CheckProcess(pSmsInfo);
}
bool CUnityThread::CheckProcess(SMSSmsInfo* pSmsInfo)
{
    vector<string> ErrorPhone; //// phone format error vec
    vector<string> BackPhone;  //// backlist phone vec
    //vector<string> RepeatPhone;
    vector<string> CorrectPhne;
    vector<string> BlankPhone;
    //// check phone format and blacklist
    if (false == CheckPhoneFormatAndBlackList(pSmsInfo,ErrorPhone,BackPhone))
    {
        LogElk("[%s: ] account[%s] CheckPhoneFormatAndBlackList is failed!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strClientId.data());

        if((BackPhone.size() == 0) && (ErrorPhone.size() == 0))
        {
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            BlankPhone.push_back("");
            pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
            pSmsInfo->m_Phonelist.clear();
            pSmsInfo->m_Phonelist = BlankPhone;
            InsertAccessDb(pSmsInfo);
            SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);
        }
        else
        {
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            ///// phone format error
            pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
            pSmsInfo->m_Phonelist.clear();
            pSmsInfo->m_Phonelist = ErrorPhone;
            InsertAccessDb(pSmsInfo);
            //SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);

            //// phone blacklist
            pSmsInfo->m_strErrorCode = BLACKLIST;
            pSmsInfo->m_Phonelist.clear();
            pSmsInfo->m_Phonelist = BackPhone;
            InsertAccessDb(pSmsInfo);
            //SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);

            ////repeat phone
            pSmsInfo->m_strErrorCode = REPEAT_PHONE;
            pSmsInfo->m_Phonelist.clear();
            pSmsInfo->m_Phonelist = pSmsInfo->m_RepeatPhonelist;
            InsertAccessDb(pSmsInfo);
            SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);
        }
        return false;
    }
    //// check signExtend and keyword
    if(pSmsInfo ->m_uPkTotal == 1)
    {
        if (false == CheckSignExtendAndKeyWord(pSmsInfo))
        {
            LogElk("[%s: ] account[%s] CheckSignExtendAndKeyWord is failed!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strClientId.data());
            pSmsInfo->m_Phonelist.insert(pSmsInfo->m_Phonelist.end(),ErrorPhone.begin(),ErrorPhone.end());
            pSmsInfo->m_Phonelist.insert(pSmsInfo->m_Phonelist.end(),BackPhone.begin(),BackPhone.end());

            pSmsInfo->m_strYZXErrCode = pSmsInfo->m_strErrorCode;
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            InsertAccessDb(pSmsInfo);
            SendSubmitResp(pSmsInfo, TCP_RESULT_PARAM_ERROR);

            return false;
        }
    }
    ////check ok
    pSmsInfo->m_strErrorCode = "";
    pSmsInfo->m_uState = SMS_STATUS_INITIAL;
    InsertAccessDb(pSmsInfo);       //will fill pSmsInfo->m_mapLegalPhones
    SendSubmitResp(pSmsInfo, TCP_RESULT_OK);
    CorrectPhne = pSmsInfo->m_Phonelist;

    pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    ////phone format error
    pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
    pSmsInfo->m_strYZXErrCode = PHONE_FORMAT_IS_ERROR;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist = ErrorPhone;
    InsertAccessDb(pSmsInfo);
    if (pSmsInfo->m_uPkTotal == pSmsInfo->m_uRecvCount)
    {
        SendFailedDeliver(pSmsInfo);
    }

    ////phone blacklist
    pSmsInfo->m_strErrorCode = BLACKLIST;
    pSmsInfo->m_strYZXErrCode = BLACKLIST;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist = BackPhone;
    InsertAccessDb(pSmsInfo);
    if (pSmsInfo->m_uPkTotal == pSmsInfo->m_uRecvCount)
    {
        SendFailedDeliver(pSmsInfo);
    }

    ////repeat phone
    pSmsInfo->m_strErrorCode = REPEAT_PHONE;
    pSmsInfo->m_strYZXErrCode = REPEAT_PHONE;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist = pSmsInfo->m_RepeatPhonelist;
    InsertAccessDb(pSmsInfo);
    if (pSmsInfo->m_uPkTotal == pSmsInfo->m_uRecvCount)
    {
        SendFailedDeliver(pSmsInfo);
    }

    ////replace correct phonelist
    pSmsInfo->m_strErrorCode = "";
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist = CorrectPhne;
    pSmsInfo->m_uState = 0;

    return true;
}

void CUnityThread::SendFailedDeliver(SMSSmsInfo* pSmsInfo)
{
    UInt32  phoneCount = pSmsInfo->m_Phonelist.size();
    for(UInt32 i = 0; i < phoneCount; i++)
    {
        for(UInt32 j = 0; j < pSmsInfo->m_uShortSmsCount; j++)
        {
            CTcpDeliverReqMsg* pDeliver = new CTcpDeliverReqMsg();
            pDeliver->m_strLinkId = pSmsInfo->m_vecShortInfo.at(j).m_strLinkId;
            pDeliver->m_uResult = 8;
            pDeliver->m_uSubmitId = pSmsInfo->m_vecShortInfo.at(j).m_uSubmitId;
            pDeliver->m_strPhone = pSmsInfo->m_Phonelist.at(i);
            pDeliver->m_strYZXErrCode = pSmsInfo->m_strYZXErrCode;
            pDeliver->m_iMsgType = MSGTYPE_TCP_DELIVER_REQ;

            if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP)
            {
                g_pCMPPServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3)
            {
                g_pCMPP3ServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMPP)
            {
                g_pSMPPServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP)
            {
                g_pSMGPServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_HTTP)
            {
                delete pDeliver;
                pDeliver = NULL;
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
            {
                delete pDeliver;
                pDeliver = NULL;

                CSgipFailRtReqMsg* pFailRt = new CSgipFailRtReqMsg();
                pFailRt->m_strClientId.assign(pSmsInfo->m_strClientId);
                pFailRt->m_strPhone.assign(pSmsInfo->m_Phonelist.at(i));
                pFailRt->m_uState = 8;
                pFailRt->m_iMsgType = MSGTYPE_TCP_DELIVER_REQ;

                std::vector<string> vecSequence;
                Comm comm;
                comm.splitExVector(pSmsInfo->m_vecShortInfo.at(j).m_strSgipNodeAndTimeAndId,"|",vecSequence);
                if (vecSequence.size() != 3)
                {
                    LogError("===sgip== sequence[%s] is analyze failed.",pSmsInfo->m_vecShortInfo.at(j).m_strSgipNodeAndTimeAndId.data());
                    pFailRt->m_uSequenceIdNode = 0;
                    pFailRt->m_uSequenceIdTime = 0;
                    pFailRt->m_uSequenceId = 0;
                }
                else
                {
                    pFailRt->m_uSequenceIdNode = atoi(vecSequence.at(0).data());
                    pFailRt->m_uSequenceIdTime = atoi(vecSequence.at(1).data());
                    pFailRt->m_uSequenceId = atoi(vecSequence.at(2).data());
                }
                g_pSgipRtAndMoThread->PostMsg(pFailRt);
            }
            else
            {
                LogError("[%s:%s] smsfrom[%u] is invalid type.",pSmsInfo->m_strSmsId.data(),pDeliver->m_strPhone.data(),pSmsInfo->m_uSmsFrom);
                delete pDeliver;
            }
        }
    }

    return;
}

bool CUnityThread::CheckSignExtend(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    ////count fee number
    utils::UTFString utfHelper;
    int iLength = utfHelper.getUtf8Length(pSmsInfo->m_strContent);

    if ((iLength <= 70)
    || (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP)
    || (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMPP)
    || (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP)
    || (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
    || (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3))
    {
        pSmsInfo->m_uSmsNum = 1;
    }
    else
    {
        pSmsInfo->m_uSmsNum = (iLength + 66) / 67;
    }

    string strSign = "";
    string strContent = "";
    string strExtpendPort = "";
    string strSignPort = "";
    UInt32 uShowSignType = 1;
    UInt32 uChina = 0;    // 0 english,  1 china

    int ret = ExtractSignAndContent(pSmsInfo->m_strContent, strContent, strSign, uChina, uShowSignType);
    if ( 0 != ret )
    {
        if (!getContentWithSign(pSmsInfo, strSign, uChina))
        {
            LogError("Call getContentWithSign failed.");

            pSmsInfo->m_strErrorCode = CHINESE_2;
            pSmsInfo->m_iErrorCode = ret;
            pSmsInfo->m_strError = HTTPS_RESPONSE_MSG_6;
            return false;
        }
    }

    if (0 == pSmsInfo->m_uNeedExtend)
    {
        strExtpendPort = "";
    }
    else
    {
        strExtpendPort = pSmsInfo->m_strExtpendPort;
    }

    if (false == strSign.empty())
    {
        //// not support sign port
        if (0 == pSmsInfo->m_uNeedSignExtend)
        {
            ;
        }
        else
        {
            if (false == getSignPort(pSmsInfo->m_strClientId,strSign,strSignPort))
            {
                pSmsInfo->m_strErrorCode = SIGN_NOT_REGISTER;
                pSmsInfo->m_iErrorCode = ACCESS_HTTPSERVER_RETURNCODE_SIGNNOTREGISTED;
                pSmsInfo->m_strError = HTTPS_RESPONSE_MSG_9;
                return false;
            }
            else
            {
                ;
            }
        }
    }
    else
    {
        LogError("[%s: ] content[%s] is not contain sign!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strContent.data());
        pSmsInfo->m_strErrorCode = "content is not contain sign";
        pSmsInfo->m_iErrorCode = ACCESS_HTTPSERVER_RETURNCODE_PARAMERR;
        pSmsInfo->m_strError = HTTPS_RESPONSE_MSG_6;
        return false;
    }

    pSmsInfo->m_strIsChina.assign(Comm::int2str(uChina));
    pSmsInfo->m_strSign.assign(strSign);
    pSmsInfo->m_strExtpendPort.assign(strExtpendPort);
    pSmsInfo->m_uShowSignType = uShowSignType;
    pSmsInfo->m_strSignPort.assign(strSignPort);

    if (!strContent.empty())
    {
        pSmsInfo->m_strContent.assign(strContent);
    }

    return true;
}

bool CUnityThread::getContentWithSign(SMSSmsInfo* pSmsInfo, string& strSign, UInt32& uiChina)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    LogDebug("clientid:%s, accountExtAttr:%u, extpendPort:%s.",
        pSmsInfo->m_strClientId.data(),
        pSmsInfo->m_uAccountExtAttr,
        pSmsInfo->m_strExtpendPort.data());

    if ((pSmsInfo->m_uAccountExtAttr & ACCOUNT_EXT_PORT_TO_SIGN)
    && (1 == pSmsInfo->m_uNeedExtend)
    && (getPortSign(pSmsInfo->m_strClientId, pSmsInfo->m_strExtpendPort, strSign)))
    {
        bool bChineseSign = Comm::IncludeChinese((char*)strSign.data());
        bool bChineseContent = Comm::IncludeChinese((char*)pSmsInfo->m_strContent.data());

        pSmsInfo->m_uiSignManualFlag = 1;

        // 需要将签名拼到后面，否则计费会有问题，只影响入库，不影响MQ消息
        if (bChineseSign || bChineseContent)
        {
            static string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
            static string strRight = http::UrlCode::UrlDecode("%e3%80%91");

            pSmsInfo->m_strContentWithSign = pSmsInfo->m_strContent + strLeft + strSign + strRight;
            uiChina = 1;
        }
        else
        {
            pSmsInfo->m_strContentWithSign = pSmsInfo->m_strContent + "[" + strSign + "]";
            uiChina = 0;
        }

        return true;
    }

    return false;
}

bool CUnityThread::getSignPort(string& strClientId,string& strSign, string& strOut)
{
    string strKey = strClientId + "&" + strSign;
    ClientIdAndSignMap::iterator itr = m_mapClientSignPort.find(strKey);
    if (itr == m_mapClientSignPort.end())
    {
        LogWarn("===clientId[%s], sign[%s] is not find table in t_sms_clientid_signport.",strClientId.data(),strSign.data());
        return false;
    }

    strOut = itr->second.m_strSignPort;
    return true;
}

bool CUnityThread::isSignExist(const string& strSrcContent, string& strOut)
{
    string strSign;
    UInt32 uChina;
    UInt32 uShowSignType;
    string strLeft;
    string strRight;
    string strContent = strSrcContent;

    Comm::trimSign(strContent);

    strOut = strContent;

    if (true == Comm::IncludeChinese((char*)strContent.data()))
    {
        strLeft = "%e3%80%90";
        strRight = "%e3%80%91";
        strLeft = http::UrlCode::UrlDecode(strLeft);
        strRight = http::UrlCode::UrlDecode(strRight);
        uChina = 1;
    }
    else
    {
        strLeft = "[";
        strRight = "]";
        uChina = 0;
    }

    std::string::size_type lPos = strContent.find(strLeft);
    if (lPos == std::string::npos)
    {
        LogWarn("this is not find leftSign[%s]",strLeft.data());
        return false;
    }
    else if (0 == lPos)   ////sign is before
    {
        std::string::size_type rPos = strContent.find(strRight);
        if (rPos == std::string::npos)
        {
            LogWarn("can not find sign. content[%s]",strContent.data());
            return false;

        }
        else if((rPos+strRight.length()) == strContent.length())
        {
            LogWarn("can not find sign. content[%s]",strContent.data());
            return false;
        }
        ////extract sign and content
        uShowSignType = 1;
        strOut = strContent.substr(rPos+strRight.length());
        strSign = strContent.substr(strLeft.length(),rPos - strLeft.length());
    }
    else //// sign is after
    {
        lPos = strContent.rfind(strLeft);
        std::string::size_type rPos = strContent.rfind(strRight);
        if ((lPos == std::string::npos) || (rPos == std::string::npos))
        {
            LogWarn("cannot find sign.content[%s]",strContent.data());
            return false;

        }
        else if ((rPos+strRight.length()) != strContent.length())
        {
            LogWarn("can not find sign. content[%s]",strContent.data());
            return false;
        }

        uShowSignType = 2;
        strOut = strContent.substr(0,lPos);
        strSign = strContent.substr(lPos+strLeft.length(),rPos-lPos-strLeft.length());
    }

    //// check sign length
    utils::UTFString utfHelper;
    int length = utfHelper.getUtf8Length(strSign);

    if (length <= 1)
    {
        LogWarn("WARN, sign length=0. length[%d]", length);
        return false;
    }
    else if (length > 100)  //lpj::为阿里改动，签名长度
    {
        LogWarn("WARN, sign length>8. length[%d]", length);
        return false;
    }

    return true;
}


int CUnityThread::ExtractSignAndContent(string& strContent,string& strOut,string& strSign,UInt32& uChina,UInt32& uShowSignType)
{
    string strLeft = "";
    string strRight = "";

    Comm::trimSign(strContent);

    if (true == Comm::IncludeChinese((char*)strContent.data()))
    {
        strLeft = "%e3%80%90";
        strRight = "%e3%80%91";
        strLeft = http::UrlCode::UrlDecode(strLeft);
        strRight = http::UrlCode::UrlDecode(strRight);
        uChina = 1;
    }
    else
    {
        strLeft = "[";
        strRight = "]";
        uChina = 0;
    }

    std::string::size_type lPos = strContent.find(strLeft);
    if (lPos == std::string::npos)
    {
        LogWarn("this is not find leftSign[%s]",strLeft.data());
        strOut = strContent;
        return ACCESS_HTTPSERVER_RETURNCODE_PARAMERR; /// -6
    }
    else if (0 == lPos)   ////sign is before
    {
        std::string::size_type rPos = strContent.find(strRight);
        if((rPos == std::string::npos))
        {
            //can not find sign
            LogWarn("can not find sign. content[%s]",strContent.data());
            strOut = strContent;
            return ACCESS_HTTPSERVER_RETURNCODE_PARAMERR;   //-6

        }
        else if((rPos+strRight.length()) == strContent.length())
        {
            //can not find sign
            LogWarn("can not find sign. content[%s]",strContent.data());
            return ACCESS_HTTPSERVER_RETURNCODE_PARAMERR;   // -6
        }
        ////extract sign and content
        uShowSignType = 1;
        strOut = strContent.substr(rPos+strRight.length());
        strSign = strContent.substr(strLeft.length(),rPos - strLeft.length());
    }
    else //// sign is after
    {
        lPos = strContent.rfind(strLeft);
        std::string::size_type rPos = strContent.rfind(strRight);
        if ((lPos == std::string::npos) || (rPos == std::string::npos))
        {
            LogWarn("cannot find sign.content[%s]",strContent.data());
            strOut = strContent;
            return ACCESS_HTTPSERVER_RETURNCODE_PARAMERR; //// -6

        }
        else if ((rPos+strRight.length()) != strContent.length())
        {
            //can not find sign
            LogWarn("can not find sign. content[%s]",strContent.data());
            return ACCESS_HTTPSERVER_RETURNCODE_PARAMERR;   // -6
        }

        uShowSignType = 2;
        strOut = strContent.substr(0,lPos);
        strSign = strContent.substr(lPos+strLeft.length(),rPos-lPos-strLeft.length());
    }

    //// check sign length
    utils::UTFString utfHelper;
    int length = utfHelper.getUtf8Length(strSign);


    if(length <=1)
    {
        LogWarn("WARN, sign length=0. length[%d]", length);
        return ACCESS_HTTPSERVER_RETURNCODE_ERRORLENGTH;  ///-8
    }
    else if(length > 100)  //lpj::为阿里改动，签名长度
    {
        LogWarn("WARN, sign length>8. length[%d]", length);
        return ACCESS_HTTPSERVER_RETURNCODE_ERRORLENGTH;  ///-8
    }
    else
    {
        //OK
        return 0;
    }
}

bool CUnityThread::CheckPhoneFormatAndBlackList(SMSSmsInfo* pSmsInfo,vector<string>& formatErrorVec,vector<string>& blackListVec)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return false;
    }

    ////check phone format
    for (vector<string>::iterator itrPhone = pSmsInfo->m_Phonelist.begin(); itrPhone != pSmsInfo->m_Phonelist.end();)
    {
        string& strTempPhone = *itrPhone;
        if(false == m_honeDao.m_pVerify->CheckPhoneFormat(strTempPhone))
        {
            LogError("[%s: ] phone[%s] format is error!",pSmsInfo->m_strSmsId.data(),strTempPhone.data());
            if(false == m_honeDao.m_pVerify->isVisiblePhoneFormat(strTempPhone))
            {
                formatErrorVec.push_back(INVISIBLE_NUMBER_STRING);
            }
            else
            {
                formatErrorVec.push_back(strTempPhone);
            }

            itrPhone = pSmsInfo->m_Phonelist.erase(itrPhone);
        }
        else
        {
            if(pSmsInfo->m_strPhone.empty())
            {
                pSmsInfo->m_strPhone.assign(*itrPhone);
            }
            else
            {
                pSmsInfo->m_strPhone.append(",");
                pSmsInfo->m_strPhone.append(*itrPhone);
            }
            itrPhone++;
        }
    }

    ////check repeat phone
    map<string,int> phoneMapInfo;
    for (vector<string>::iterator itrPhone = pSmsInfo->m_Phonelist.begin(); itrPhone != pSmsInfo->m_Phonelist.end();)
    {
        map<string,int>::iterator itrMap = phoneMapInfo.find(*itrPhone);
        if (itrMap == phoneMapInfo.end())
        {
            phoneMapInfo[*itrPhone] = 1;
            itrPhone++;
        }
        else
        {
            itrPhone= pSmsInfo->m_Phonelist.erase(itrPhone);
        }
    }

    if (pSmsInfo->m_strPhone.empty())
    {
        LogError("[%s: ] no send phone!",pSmsInfo->m_strSmsId.data());
        ////blankVec.push_back("");
        return false;
    }

    return true;
}

void CUnityThread::ConstructResp(SMSSmsInfo* pSmsInfo, UInt8 uResult)
{
    UInt32 count = 0;
    if(pSmsInfo->m_uLongSms == 1)
    {
         count = pSmsInfo->m_uRecvCount - 1;
    }
    uResult = (uResult >= TCP_RESULT_ERROR_MAX) ? TCP_RESULT_OTHER_ERROR : uResult;
    CTcpSubmitRespMsg* pResp = new CTcpSubmitRespMsg();
    pResp->m_strLinkId = pSmsInfo->m_vecShortInfo.at(count).m_strLinkId;
    pResp->m_uResult = uResult;
    pResp->m_uSequenceNum = pSmsInfo->m_vecShortInfo.at(count).m_uSequenceNum;
    pResp->m_uSubmitId = pSmsInfo->m_vecShortInfo.at(count).m_uSubmitId;
    pResp->m_iMsgType = MSGTYPE_TCP_SUBMIT_RESP;

    if (SMS_FROM_ACCESS_CMPP == pSmsInfo->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrErrorCodeMap[pSmsInfo->m_uSmsFrom][uResult];
        g_pCMPPServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_CMPP3 == pSmsInfo->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrErrorCodeMap[pSmsInfo->m_uSmsFrom][uResult];
        g_pCMPP3ServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_SMPP == pSmsInfo->m_uSmsFrom)
    {
        if (TCP_RESULT_FLOWCTRL == uResult)
        {
            pResp->m_uResult = m_arrErrorCodeMap[pSmsInfo->m_uSmsFrom][uResult];
        }

        g_pSMPPServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_SMGP == pSmsInfo->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrErrorCodeMap[pSmsInfo->m_uSmsFrom][uResult];
        g_pSMGPServiceThread->PostMsg(pResp);
    }
    else if (SMS_FROM_ACCESS_SGIP == pSmsInfo->m_uSmsFrom)
    {
        pResp->m_uResult = m_arrErrorCodeMap[pSmsInfo->m_uSmsFrom][uResult];
        std::vector<string> vecSequence;
        Comm comm;
        comm.splitExVector(pSmsInfo->m_vecShortInfo.at(count).m_strSgipNodeAndTimeAndId,"|",vecSequence);
        if (vecSequence.size() != 3)
        {
            LogError("===sgip== sequence[%s] is analyze failed.",pSmsInfo->m_vecShortInfo.at(count).m_strSgipNodeAndTimeAndId.data());
            pResp->m_uSequenceIdNode = 0;
            pResp->m_uSequenceIdTime = 0;
            pResp->m_uSequenceId = 0;
        }
        else
        {
            pResp->m_uSequenceIdNode = atoi(vecSequence.at(0).data());
            pResp->m_uSequenceIdTime = atoi(vecSequence.at(1).data());
            pResp->m_uSequenceId = atoi(vecSequence.at(2).data());
        }
        g_pSGIPServiceThread->PostMsg(pResp);
    }
    else
    {
        LogWarn("[%s: ] smsfrom[%u] is invalid!",pSmsInfo->m_strSmsId.data(),pSmsInfo->m_uSmsFrom);
    }
}
void CUnityThread::SendSubmitResp(SMSSmsInfo* pSmsInfo, UInt8 uResult)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return;
    }

    if(pSmsInfo->m_uLongSms == 0)
    {
        ConstructResp(pSmsInfo, uResult);
        return;
    }
    else
    {
        ///////////////
        if(pSmsInfo->m_vecShortInfo.size() ==1)         //?
        {
            //first msg of long msg
            pSmsInfo->m_uFirstShortSmsResp = uResult;

        }
        else
        {
            //set report flag
            if(pSmsInfo->m_uFirstShortSmsResp == TCP_RESULT_OK  && uResult !=TCP_RESULT_OK)
            {
                pSmsInfo->m_bIsNeedSendReport = true;
            }

            //return
            uResult = pSmsInfo->m_uFirstShortSmsResp;
        }

        ConstructResp(pSmsInfo, uResult);

    }
    return;
}

void CUnityThread::InsertAccessDb(SMSSmsInfo* pSmsInfo)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return;
    }

    UInt32 j = 0;
    if ((pSmsInfo->m_uPkTotal != 1) && (pSmsInfo->m_uRecvCount >= 1))
    {
        j = pSmsInfo->m_uRecvCount -1;
    }

    string strDate = pSmsInfo->m_strDate.substr(0, 8);  //get date.

    string strTempDesc = "";
    map<string,string>::iterator itrDesc = m_mapSystemErrorCode.find(pSmsInfo->m_strErrorCode);
    if (itrDesc != m_mapSystemErrorCode.end())
    {
        strTempDesc.assign(pSmsInfo->m_strErrorCode);
        strTempDesc.append("*");
        strTempDesc.append(itrDesc->second);
    }
    else
    {
        strTempDesc.assign(pSmsInfo->m_strErrorCode);
    }

    UInt32 uOperaterType = 0;
    UInt32  phoneCount = pSmsInfo->m_Phonelist.size();
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

    ////LogDebug("here is InsertAccessDb");
    for(UInt32 i = 0; i < phoneCount; i++)
    {
        string phone = pSmsInfo->m_Phonelist.at(i);
        if (m_honeDao.m_pVerify->CheckPhoneFormat(phone))
        {
            uOperaterType = m_honeDao.getPhoneType(phone);
        }
        string tmpId = "";
        if("" != pSmsInfo->m_strErrorCode)  //20160902 add by fangjinxiong.
        {
            tmpId = getUUID();
        }
        else
        {
            tmpId = pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[phone];     //here only ok phones, repeatphones need generat another uuid
        }

        std::string msg = pSmsInfo->m_vecShortInfo.at(j).m_strShortContent;

        char content[3072] = {0};
        char sign[128] = {0};
        char strUid[64] = {0};
        char strTempParam[2048] = {0};
        UInt32 position = pSmsInfo->m_strSign.length();
        if(pSmsInfo->m_strSign.length() > 100)
        {
            position = Comm::getSubStr(pSmsInfo->m_strSign,100);
        }
        if(MysqlConn != NULL)
        {
            mysql_real_escape_string(MysqlConn, content, msg.data(), msg.length());

            //将uid字段转义
            mysql_real_escape_string(MysqlConn, strUid, pSmsInfo->m_strUid.substr(0, 60).data(), pSmsInfo->m_strUid.substr(0, 60).length());

            //将sign字段转义
            mysql_real_escape_string(MysqlConn, sign, pSmsInfo->m_strSign.substr(0, position).data(), pSmsInfo->m_strSign.substr(0, position).length());

            if (!pSmsInfo->m_strTemplateParam.empty())
            {
                mysql_real_escape_string(MysqlConn, strTempParam, pSmsInfo->m_strTemplateParam.data(),
                                 pSmsInfo->m_strTemplateParam.length());
            }
        }

        char sql[10240]  = {0};
        utils::UTFString utfHelper;

        UInt32 uiLongSms = 0;
        if (pSmsInfo->m_uPkTotal > 1)
        {
            uiLongSms = 1;
        }
        else
        {
            int iLength = utfHelper.getUtf8Length(pSmsInfo->m_strContentWithSign);
            uiLongSms = (iLength > 70) ? 1 : 0;
        }

        int iLength2 = utfHelper.getUtf8Length(msg);
        UInt32 uiChargeNum = (iLength2 > 70) ? ((iLength2 + 66) / 67) : 1;
        UInt32 uiShowSignType = (1 == pSmsInfo->m_uiSignManualFlag) ? 1 : pSmsInfo->m_uShowSignType;
        string strBelongSale = Comm::int2str(pSmsInfo->m_uBelong_sale);

        snprintf(sql, 10240,"insert into t_sms_access_%u_%s(id,content,srcphone,phone,smscnt,smsindex,sign,submitid,smsid,clientid,"
                     "operatorstype,smsfrom,state,errorcode,date,innerErrorcode,channelid,smstype,charge_num,paytype,agent_id,username ,isoverratecharge,"
                     "uid,showsigntype,product_type,c2s_id,process_times,longsms,channelcnt,template_id,temp_params,sid,belong_sale,sub_id)"
                     "  values('%s', '%s', '%s', '%s', '%u', '%u', '%s', '%lu', '%s', '%s', '%u',"
                     "'%u','%d','%s','%s','%s','%d','%d','%u','%u','%lu','%s','%u','%s','%u','%u','%lu','%u','%u','%d', %s, '%s', '%s', %s ,'%s');",
                 pSmsInfo->m_uIdentify,
                 strDate.data(),
                 tmpId.data(),
                 content,
                 pSmsInfo->m_strSrcPhone.substr(0, 20).data(),
                 pSmsInfo->m_Phonelist[i].substr(0, 20).data(),
                 pSmsInfo->m_vecShortInfo.at(j).m_uPkTotal,
                 pSmsInfo->m_vecShortInfo.at(j).m_uPkNumber,
                 sign,
                 pSmsInfo->m_vecShortInfo.at(j).m_uSubmitId,
                 pSmsInfo->m_strSmsId.data(),
                 pSmsInfo->m_strClientId.data(),
                 uOperaterType,
                 pSmsInfo->m_uSmsFrom,
                 pSmsInfo->m_uState,
                 strTempDesc.data(),
                 pSmsInfo->m_strDate.data(),
                 strTempDesc.data(),
                 0,
                 atoi(pSmsInfo->m_strSmsType.data()),
                 uiChargeNum,
                 pSmsInfo->m_uPayType,
                 pSmsInfo->m_uAgentId,
                 pSmsInfo->m_strUserName.data(),
                 pSmsInfo->m_uOverRateCharge,
                 strUid,
                 uiShowSignType,
                 pSmsInfo->m_uProductType,
                 g_uComponentId,
                 pSmsInfo->m_uProcessTimes,
                 uiLongSms,
                 1,
                 (pSmsInfo->m_strTemplateId.empty()) ? "NULL" : pSmsInfo->m_strTemplateId.data(),
                 strTempParam,
                 " ",
                 (!strBelongSale.compare("0")) ? "NULL" : strBelongSale.data(),
                 "0");

        LogDebug("[%s: ] access insert DB sql[%s].",pSmsInfo->m_strSmsId.data(), sql);

        publishMqC2sDbMsg(tmpId, sql);

        MONITOR_INIT( MONITOR_RECEIVE_MT_SMS );
        MONITOR_VAL_SET("clientid", pSmsInfo->m_strClientId);
        MONITOR_VAL_SET("username", pSmsInfo->m_strUserName);
        MONITOR_VAL_SET_INT("operatorstype", uOperaterType);
        MONITOR_VAL_SET_INT("smsfrom", pSmsInfo->m_uSmsFrom);
        MONITOR_VAL_SET_INT("paytype", pSmsInfo->m_uPayType);
        MONITOR_VAL_SET("sign", sign );
        MONITOR_VAL_SET("smstype", pSmsInfo->m_strSmsType);
        MONITOR_VAL_SET("date", pSmsInfo->m_strDate);
        MONITOR_VAL_SET("uid",  strUid);
        MONITOR_VAL_SET("smsid", pSmsInfo->m_strSmsId);
        MONITOR_VAL_SET("phone", pSmsInfo->m_Phonelist[i].substr(0, 20).data());
        MONITOR_VAL_SET_INT("id", pSmsInfo->m_uIdentify);
        MONITOR_VAL_SET_INT("state", pSmsInfo->m_uState);
        MONITOR_VAL_SET("errrorcode", pSmsInfo->m_strErrorCode);
        MONITOR_VAL_SET("errordesc", strTempDesc);
        MONITOR_VAL_SET_INT("component_id", g_uComponentId);
        MONITOR_PUBLISH(g_pMQMonitorPublishThread );
    }
}


/*
    IOMQ link err, and g_pIOMQPublishThread.queuesize > m_uIOProduceMaxChcheSize.
    then will update db and send report.
*/
void CUnityThread::UpdateRecord(SMSSmsInfo* pInfo)
{
    if (NULL == pInfo)
    {
        LogError("pInfo is NULL.");
        return;
    }

    pInfo->m_strSubmit.assign(pInfo->m_strErrorCode);

    string strDate = pInfo->m_strDate.substr(0, 8); //get date.

    string strTempDesc = "";
    map<string,string>::iterator itrDesc = m_mapSystemErrorCode.find(pInfo->m_strErrorCode);
    if (itrDesc != m_mapSystemErrorCode.end())
    {
        strTempDesc.assign(pInfo->m_strErrorCode);
        strTempDesc.append("*");
        strTempDesc.append(itrDesc->second);
    }
    else
    {
        strTempDesc.assign(pInfo->m_strErrorCode);
    }

    UInt32 uPhoneListSize = pInfo->m_Phonelist.size();
    UInt32 uShortInfoSize = pInfo->m_vecShortInfo.size();
    for (UInt32 i = 0; i < uPhoneListSize; i++)
    {
        for (UInt32 j = 0; j < uShortInfoSize; j++)
        {
            string stridtmp = pInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[pInfo->m_Phonelist.at(i)];

            char sql[1024]  = {0};
            snprintf(sql, 1024,"UPDATE t_sms_access_%d_%s SET state='%d', errorcode='%s',innerErrorcode='%s' where id = '%s' and date ='%s';",
                pInfo->m_uIdentify,
                strDate.data(),
                pInfo->m_uState,
                strTempDesc.data(),
                strTempDesc.data(),
                stridtmp.data(),
                pInfo->m_strDate.data());

            LogDebug("[%s:%s] update sql[%s].",pInfo->m_strSmsId.data(),pInfo->m_Phonelist.at(i).data(), sql);

            publishMqC2sDbMsg(stridtmp, sql);

            MONITOR_INIT( MONITOR_RECEIVE_MT_SMS );
            MONITOR_VAL_SET("clientid", pInfo->m_strClientId);
            MONITOR_VAL_SET("username", pInfo->m_strUserName);
            MONITOR_VAL_SET_INT("operatorstype", pInfo->m_uOperater );
            MONITOR_VAL_SET_INT("smsfrom", pInfo->m_uSmsFrom);
            MONITOR_VAL_SET_INT("paytype", pInfo->m_uPayType);
            MONITOR_VAL_SET("sign", pInfo->m_strSign );
            MONITOR_VAL_SET("smstype", pInfo->m_strSmsType );
            MONITOR_VAL_SET("date", pInfo->m_strDate);
            MONITOR_VAL_SET("uid",  pInfo->m_strUid );
            MONITOR_VAL_SET("smsid", pInfo->m_strSmsId);
            MONITOR_VAL_SET("phone", pInfo->m_Phonelist.at(i).data());
            MONITOR_VAL_SET_INT("id",pInfo->m_uIdentify);
            MONITOR_VAL_SET_INT("state",  pInfo->m_uState);
            MONITOR_VAL_SET("errrorcode", pInfo->m_strErrorCode);
            MONITOR_VAL_SET("errordesc", strTempDesc);
            MONITOR_VAL_SET_INT("component_id", g_uComponentId);
            MONITOR_PUBLISH(g_pMQMonitorPublishThread );
        }
    }
}

bool CUnityThread::ContentCoding(string& strSrc, UInt8 uCodeType, string& strDst)
{
    utils::UTFString utfHelper;

    if (0 == uCodeType)
    {
        utfHelper.Ascii2u(strSrc,strDst);
    }
    else if (8 == uCodeType)
    {
        utfHelper.u162u(strSrc,strDst);
    }
    else if (15 == uCodeType)
    {
        utfHelper.g2u(strSrc,strDst);
    }
    else if (3 == uCodeType)
    {
        LogWarn("==warn== codeType[%u] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (4 == uCodeType)
    {
        LogWarn("==warn== codeType[%u] is not support.",uCodeType);
        strDst = strSrc;
    }
    else
    {
        strDst = strSrc;
        LogError("=err== strSrc[%s],codeType[%d] is invalid code type.",strSrc.data(),uCodeType);
    }

    LogDebug("strSrc[%s],strSrcLen[%d],---strDst[%s],---datacoding[%d].",strSrc.data(),strSrc.length(),strDst.data(),uCodeType);
    return true;
}

void CUnityThread::getMQconfig(UInt32 ioperator, UInt32 smstype, string& strExchange, string& strRoutingKey)
{
    LogDebug("operator[%d], smstype[%d]",ioperator,smstype);

    switch(ioperator)
    {
        case YIDONG:
        {
            if(smstype == SMS_TYPE_MARKETING || smstype == SMS_TYPE_USSD || smstype == SMS_TYPE_FLUSH_SMS)  //YINGXIAO
            {
                strExchange = m_strYDyingxiaoExchange;
                strRoutingKey = m_strYDyingxiaoRoutingKey;

            }
            else
            {
                strExchange = m_strYDhangyeExchange;
                strRoutingKey = m_strYDhangyeRoutingKey;
            }
            break;
        }
        case LIANTONG:
        {
            if(smstype == SMS_TYPE_MARKETING || smstype == SMS_TYPE_USSD || smstype == SMS_TYPE_FLUSH_SMS)  //YINGXIAO
            {
                strExchange = m_strLTyingxiaoExchange;
                strRoutingKey = m_strLTyingxiaoRoutingKey;
            }
            else
            {
                strExchange = m_strLThangyeExchange;
                strRoutingKey = m_strLThangyeRoutingKey;
            }
            break;
        }
        case DIANXIN:
        {
            if (smstype == SMS_TYPE_MARKETING || smstype == SMS_TYPE_USSD || smstype == SMS_TYPE_FLUSH_SMS) //YINGXIAO
            {
                strExchange = m_strDXyingxiaoExchange;
                strRoutingKey = m_strDXyingxiaoRoutingKey;

            }
            else
            {
                strExchange = m_strDXhangyeExchange;
                strRoutingKey = m_strDXhangyeRoutingKey;
            }
            break;
        }
        case FOREIGN:
        {
            if (smstype == SMS_TYPE_USSD || smstype == SMS_TYPE_FLUSH_SMS)
            {
                LogError("phone operator[%d] error for smstype[%d]", ioperator, smstype);
                return;
            }
            else if(smstype == SMS_TYPE_MARKETING)  //YINGXIAO
            {
                strExchange = m_strLTyingxiaoExchange;
                strRoutingKey = m_strLTyingxiaoRoutingKey;

            }
            else
            {
                strExchange = m_strLThangyeExchange;
                strRoutingKey = m_strLThangyeRoutingKey;
            }
            break;
        }
        case XNYD:
        {
            if(smstype == SMS_TYPE_MARKETING || smstype == SMS_TYPE_USSD || smstype == SMS_TYPE_FLUSH_SMS)  //YINGXIAO
            {
                strExchange = m_strYDyingxiaoExchange;
                strRoutingKey = m_strYDyingxiaoRoutingKey;

            }
            else
            {
                strExchange = m_strYDhangyeExchange;
                strRoutingKey = m_strYDhangyeRoutingKey;
            }
            break;
        }
        case XNLT:
        {
            if(smstype == SMS_TYPE_MARKETING || smstype == SMS_TYPE_USSD || smstype == SMS_TYPE_FLUSH_SMS)  //YINGXIAO
            {
                strExchange = m_strLTyingxiaoExchange;
                strRoutingKey = m_strLTyingxiaoRoutingKey;

            }
            else
            {
                strExchange = m_strLThangyeExchange;
                strRoutingKey = m_strLThangyeRoutingKey;
            }
            break;
        }
        case XNDX:
        {
            if(smstype == 5)    //YINGXIAO
            {
                strExchange = m_strDXyingxiaoExchange;
                strRoutingKey = m_strDXyingxiaoRoutingKey;

            }
            else
            {
                strExchange = m_strDXhangyeExchange;
                strRoutingKey = m_strDXhangyeRoutingKey;
            }
            break;
        }
        default:
        {
            LogError("phone operator error");
        }
    }
}

void CUnityThread::HandleGetLinkedClientReq(TMsg* pMsg)
{
    //AccountMap::iterator itrAccount = m_AccountMap.find(pReq->m_strAccount);
    CLinkedClientListRespMsg* presp = new CLinkedClientListRespMsg();
    for(AccountMap::iterator itrAccount = m_AccountMap.begin(); itrAccount != m_AccountMap.end(); itrAccount++)
    {
        if((itrAccount->second.m_uSmsFrom == SMS_FROM_ACCESS_CMPP3 || itrAccount->second.m_uSmsFrom == SMS_FROM_ACCESS_CMPP || itrAccount->second.m_uSmsFrom == SMS_FROM_ACCESS_SMGP)&&
            itrAccount->second.m_uLinkCount>0)
        {
            //(SMS_FROM_ACCESS_CMPP || SMS_FROM_ACCESS_SMGP)&& LINK>0
            ////LogDebug("add link client[%s], linkCnt[%d]", itrAccount->first.c_str(), itrAccount->second.m_uLinkCount);
            presp->m_list_Client.push_back(itrAccount->first);
        }
    }

    if(presp->m_list_Client.size() ==0)
    {
        ////LogDebug("no client link");
        delete presp;
        presp = NULL;
    }
    else
    {
        ////LogDebug("linked client sum[%u]", presp->m_list_Client.size());
        presp->m_iMsgType = MSGTYPE_GET_LINKED_CLIENT_RESQ;
        pMsg->m_pSender->PostMsg(presp);
    }
}


bool CUnityThread::updateMQWeightGroupInfo(const map<string,ComponentRefMq>& componentRefMqInfo)
{
    m_mapMQWeightGroups.clear();

    for (map<string,ComponentRefMq>::iterator itr = m_componentRefMqInfo.begin();itr !=m_componentRefMqInfo.end(); ++itr)
    {
        const ComponentRefMq& componentRefMq = itr->second;
        // Only do weight choice when it's the producer of mq, and it's not DB message
        if (componentRefMq.m_uMode == PRODUCT_MODE
                && componentRefMq.m_strMessageType != MESSAGE_TYPE_DB
                && componentRefMq.m_uComponentId == g_uComponentId)
        {
            if (componentRefMq.m_uWeight == 0)
            {
                LogWarn("MQ ref weight of component[%ld]&message_type[%s]&mq[%u] is 0. Ignore it",
                        g_uComponentId, componentRefMq.m_strMessageType.c_str(), componentRefMq.m_uMqId);
                continue;
            }

            LogNotice("Add MQ ref weight of component[%ld]&message_type[%s] => mq[%u]:[%u]",
                    g_uComponentId, componentRefMq.m_strMessageType.c_str(),
                    componentRefMq.m_uMqId, componentRefMq.m_uWeight);

            mq_weightgroups_map_t::iterator wgIter = m_mapMQWeightGroups.find(componentRefMq.m_strMessageType);
            if (wgIter == m_mapMQWeightGroups.end())
            {
                MQWeightGroup mqWeightGroup(componentRefMq.m_strMessageType);
                mqWeightGroup.addMQWeightInfo(componentRefMq.m_uMqId, componentRefMq.m_uWeight);
                m_mapMQWeightGroups.insert(std::make_pair(componentRefMq.m_strMessageType, mqWeightGroup));
            }
            else
            {
                wgIter->second.addMQWeightInfo(componentRefMq.m_uMqId, componentRefMq.m_uWeight);
            }
        }
    }

    return true;
}

bool CUnityThread::mergeLong(CTcpSubmitReqMsg* pReq, SMSSmsInfo* pSmsInfo, const SmsAccount& accountInfo)
{
    CHK_NULL_RETURN_FALSE(pReq);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    pSmsInfo->m_ulSequence = m_SnManager.getSn();
    pSmsInfo->m_uRecvCount = 1;
    pSmsInfo->m_uLongSms = 1;

    mergeLongGetKey(pSmsInfo);

    LogNotice("==mergeLong== SmsId(%s), MergeLongKey(%s), Session(%lu), PkNumber(%u:%u), randCode(%u).",
            pSmsInfo->m_strSmsId.data(),
            pSmsInfo->m_strMergeLongKey.data(),
            pSmsInfo->m_ulSequence,
            pSmsInfo->m_uPkNumber,
            pSmsInfo->m_uPkTotal,
            pSmsInfo->m_uRandCode);


    LongSmsRedisMap::iterator itLongMsg = m_mapLongSmsMngRedis.find(pSmsInfo->m_ulSequence);

    if (itLongMsg == m_mapLongSmsMngRedis.end())
    {
        LogDebug("MergeLongKey(%s), Create session(%lu) in cache.",
            pSmsInfo->m_strMergeLongKey.data(),
            pSmsInfo->m_ulSequence);

        m_mapLongSmsMngRedis[pSmsInfo->m_ulSequence] = pSmsInfo;
    }
    else
    {
        LogError("MergeLongKey(%s), Find session(%lu) in cache.",
            pSmsInfo->m_strMergeLongKey.data(),
            pSmsInfo->m_ulSequence);

        return false;
    }

    UInt32 uRet = mergeLongCheckMsgParam(pReq, pSmsInfo, pSmsInfo->m_strContent, accountInfo);

    SendSubmitResp(pSmsInfo, uRet);
    if (uRet == TCP_RESULT_OK)
    {
        mergeLongGetRedisCount(pSmsInfo);
    }
    else
    {
        InsertAccessDb(pSmsInfo);
        SAFE_DELETE(pSmsInfo);
    }

    return true;
}

bool CUnityThread::mergeLongGetKey(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

//    foreach(string& str, pSmsInfo->m_Phonelist)
//    {
//        LogDebug("clientid(%s), smsid(%s), phone(%s).",
//            pSmsInfo->m_strClientId.data(),
//            pSmsInfo->m_strSmsId.data(),
//            str.data());
//    }

    pSmsInfo->m_strMergeLongKey.clear();
    pSmsInfo->m_strMergeLongKey.append(pSmsInfo->m_strClientId);
    pSmsInfo->m_strMergeLongKey.append("&");

    if (pSmsInfo->m_Phonelist.size() == 1)
    {
        pSmsInfo->m_strMergeLongKey.append(pSmsInfo->m_Phonelist.at(0));
    }
    else
    {
        string strPhone = "";
        UInt32 uPhoneSize = pSmsInfo->m_Phonelist.size();
        for (UInt32 i = 0; i < uPhoneSize;i++)
        {
            strPhone.append(pSmsInfo->m_Phonelist.at(i));
        }

        string md5Phones;
        unsigned char md5[16] = { 0 };
        MD5((const unsigned char*) strPhone.c_str(), strPhone.length(), md5);
        std::string HEX_CHARS = "0123456789abcdef";
        for (int i = 0; i < 16; i++)
        {
            md5Phones.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            md5Phones.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }

        pSmsInfo->m_strMergeLongKey.append(md5Phones);
    }

    pSmsInfo->m_strMergeLongKey.append("&");
    pSmsInfo->m_strMergeLongKey.append(Comm::int2str(pSmsInfo->m_uRandCode));

    return true;
}

bool CUnityThread::mergeLongGetRedisCount(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    TRedisReq* pReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pReq);
    CHK_NULL_RETURN_FALSE(g_pRedisThreadPool);

    pReq->m_RedisCmd.append("HINCRBY mergeLong:");
    pReq->m_RedisCmd.append(pSmsInfo->m_strMergeLongKey);
    pReq->m_RedisCmd.append(" pkNumbers 1");
    pReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pReq->m_strKey= "mergeLong";
    pReq->m_strSessionID = Comm::int2str(MERGE_LONG_GET_COUNT);
    pReq->m_iSeq = pSmsInfo->m_ulSequence;
    pReq->m_pSender = this;

    LogDebug("smsid(%s), redis cmd(%s).",
        pSmsInfo->m_strSmsId.data(),
        pReq->m_RedisCmd.data());

    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pReq);

    return true;
}

bool CUnityThread::handleRedisRsp(TMsg *pMsg)
{
    TRedisResp *pRedis = (TRedisResp*)pMsg;

    CHK_NULL_RETURN_FALSE(pRedis);
    CHK_NULL_RETURN_FALSE(pRedis->m_RedisReply);

    LongSmsRedisMap::iterator iter;
    CHK_MAP_FIND_UINT_RET_FALSE(m_mapLongSmsMngRedis, iter, pMsg->m_iSeq);

    SMSSmsInfo* pSmsInfo = iter->second;
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    UInt64 ulSession = Comm::strToUint64(pMsg->m_strSessionID);

    switch (ulSession)
    {
        case MERGE_LONG_GET_COUNT:
        {
            mergeLongRedisRspCount(pSmsInfo, pRedis->m_RedisReply);
            break;
        }
        case MERGE_LONG_GET_SHORTS:
        {
            if (!mergeLongRedisRspGetShorts(ulSession, pSmsInfo, pRedis->m_RedisReply))
            {
                if (NULL == pSmsInfo->m_pLongSmsGetAllTimer)
                {
                    mergeLongDelRedisCache(pSmsInfo);
                    mergeLongReleaseResource(pSmsInfo);
                }
                break;
            }

            if (!mergeLongMsgRealProc(pSmsInfo))
            {
                LogError("Call mergeLongMsgRealProc failed.");
                break;
            }

            mergeLongDbInsertProcess(pSmsInfo);
            proSetInfoToRedis(pSmsInfo);
            mergeLongDelRedisCache(pSmsInfo);
            mergeLongReleaseResource(pSmsInfo);
            break;
        }
        case MERGE_LONG_TIMEOUT_GET_SHORTS:
        {
            if (!mergeLongRedisRspGetShorts(ulSession, pSmsInfo, pRedis->m_RedisReply))
            {
                mergeLongDelRedisCache(pSmsInfo);
                mergeLongReleaseResource(pSmsInfo);
                break;
            }

            mergeLongTimeout(pSmsInfo, pRedis->m_RedisReply);
            break;
        }
        default:
        {
            LogWarn("MergeLongKey:%s, sequence:%lu, Invalid session:%lu.",
                pSmsInfo->m_strMergeLongKey.data(),
                pMsg->m_iSeq,
                ulSession);
            break;
        }
    }

    freeReply(pRedis->m_RedisReply);
    return true;
}

bool CUnityThread::mergeLongRedisRspCount(SMSSmsInfo* pSmsInfo, redisReply* pRedisReply)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);
    CHK_NULL_RETURN_FALSE(pRedisReply);

    if (REDIS_REPLY_INTEGER == pRedisReply->type)
    {
        pSmsInfo->m_uiLongSmsRecvCountRedis = pRedisReply->integer;
    }
//    else if (REDIS_REPLY_STRING == pRedisReply->type)
//    {
//        pSmsInfo->m_uiLongSmsRecvCountRedis = Comm::strToUint32(pRedisReply->str);
//    }
    else
    {
        LogError("MergeLongKey(%s), Invalid redis-type(%d).",
            pSmsInfo->m_strMergeLongKey.data(),
            pRedisReply->type);

        return false;
    }

    LogDebug("MergeLongKey(%s), SmsId(%s), PkNum(%u:%u), redis-pkNumbers(%u).",
        pSmsInfo->m_strMergeLongKey.data(),
        pSmsInfo->m_strSmsId.data(),
        pSmsInfo->m_uPkNumber,
        pSmsInfo->m_uPkTotal,
        pSmsInfo->m_uiLongSmsRecvCountRedis);

    if (pSmsInfo->m_uiLongSmsRecvCountRedis == pSmsInfo->m_uPkTotal)
    {
        mergeLongGetAllMsg(pSmsInfo, MERGE_LONG_GET_SHORTS);
    }
    else
    {
        mergeLongSetSmsDetail(pSmsInfo);

        if( 1 == pSmsInfo->m_uiLongSmsRecvCountRedis )
        {
            mergeLongSetTimeout(pSmsInfo);

            pSmsInfo->m_pLongSmsRedisTimer = SetTimer(MERGE_LONG_REDIS_TIMER_ID,
                                                    Comm::int2str(pSmsInfo->m_ulSequence),
                                                    m_uiLongSmsMergeTimeOut * 1000);
        }
        else
        {
            mergeLongReleaseResource(pSmsInfo);
        }
    }

    return true;
}

bool CUnityThread::mergeLongSetSmsDetail(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    string encodeData;

    const ShortSmsInfo &shortSms = pSmsInfo->m_vecShortInfo.at(0);

    encodeData.append("content=");
    encodeData.append(Base64::Encode(pSmsInfo->m_strContent));

    encodeData.append("&smsid=");
    encodeData.append(pSmsInfo->m_strSmsId);

    encodeData.append("&state=");
    encodeData.append(Comm::int2str(pSmsInfo->m_uState));

    encodeData.append("&errcode=");
    encodeData.append(pSmsInfo->m_strErrorCode);

    encodeData.append("&PkNumber=");
    encodeData.append(Comm::int2str(shortSms.m_uPkNumber));

    encodeData.append("&SubmitId=");
    encodeData.append(Comm::int2str(shortSms.m_uSubmitId));

    encodeData.append("&SequenceNum=");
    encodeData.append(Comm::int2str(shortSms.m_uSequenceNum));

    encodeData.append("&LinkId=");
    encodeData.append(shortSms.m_strLinkId);

    encodeData.append("&SgipNodeAndTimeAndId=");
    encodeData.append(shortSms.m_strSgipNodeAndTimeAndId);

    encodeData.append("&c2sId=");
    encodeData.append(Comm::int2str(g_uComponentId));

    {
        string strPhoneRefIDs;
        typedef map<string, string> map_type;
        typedef const map_type::value_type const_pair;

        foreach(const_pair& node, shortSms.m_mapPhoneRefIDs)
        {
            strPhoneRefIDs.append(node.first);
            strPhoneRefIDs.append("=");
            strPhoneRefIDs.append(node.second);
            strPhoneRefIDs.append(";");
        }

        if (!strPhoneRefIDs.empty())
        {
            strPhoneRefIDs.erase(strPhoneRefIDs.length()-1);
        }

        encodeData.append("&PhoneRefIDs=");
        encodeData.append(strPhoneRefIDs);
    }


    TRedisReq* pReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pReq);
    CHK_NULL_RETURN_FALSE(g_pRedisThreadPool);

    pReq->m_RedisCmd.append("HMSET mergeLong:");
    pReq->m_RedisCmd.append(pSmsInfo->m_strMergeLongKey);
    pReq->m_RedisCmd.append(" shortContent_");
    pReq->m_RedisCmd.append(Comm::int2str(pSmsInfo->m_uPkNumber));
    pReq->m_RedisCmd.append(" ");
    pReq->m_RedisCmd.append(encodeData);
    pReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pReq->m_strKey= "mergeLong";

    LogDebug("SmsId(%s), redis cmd(%s).", pSmsInfo->m_strSmsId.data(), pReq->m_RedisCmd.data());

    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pReq);

    return true ;
}

bool CUnityThread::mergeLongSetTimeout(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_NULL(pSmsInfo);

    // 设置redis超时时间，必须比定时器长
    // 否则定时器超时查redis查不到，就当作合并成功处理了
    TRedisReq* pReq = new TRedisReq();
    CHK_NULL_RETURN_NULL(pReq);
    CHK_NULL_RETURN_NULL(g_pRedisThreadPool);

    pReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pReq->m_strKey = "mergeLong";
    pReq->m_RedisCmd.append("EXPIRE mergeLong:");
    pReq->m_RedisCmd.append(pSmsInfo->m_strMergeLongKey);
    pReq->m_RedisCmd.append(" ");
    pReq->m_RedisCmd.append(Comm::int2str(2 * m_uiLongSmsMergeTimeOut));

    LogDebug("SmsId(%s), redis cmd(%s).", pSmsInfo->m_strSmsId.data(), pReq->m_RedisCmd.data());

    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pReq);

    return true;
}

bool CUnityThread::mergeLongTimeout(SMSSmsInfo* pSmsInfo, redisReply* pRedisReply)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);
    CHK_NULL_RETURN_FALSE(pRedisReply);

    if (pSmsInfo->m_uiLongSmsRecvCountRedis == pSmsInfo->m_uPkTotal)
    {
        LogDebug("No need process.");
        return true;
    }

    LogNotice("MergeLongKey(%s), clientid(%s), smsid(%s). pkNumbers(%u:%u), Redis merge longsms time out.",
        pSmsInfo->m_strMergeLongKey.data(),
        pSmsInfo->m_strClientId.data(),
        pSmsInfo->m_strSmsId.data(),
        pSmsInfo->m_uiLongSmsRecvCountRedis,
        pSmsInfo->m_uPkTotal);

    foreach(ShortSmsInfo& sms, pSmsInfo->m_vecShortInfo)
    {
        sms.m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        sms.m_strErrCode = LONG_SMS_MERGE_TIMEOUT;
    }

    pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    pSmsInfo->m_strErrorCode = LONG_SMS_MERGE_TIMEOUT;
    pSmsInfo->m_strYZXErrCode = LONG_SMS_MERGE_TIMEOUT;

    mergeLongSendFailedDeliver(pSmsInfo, pSmsInfo->m_Phonelist, pSmsInfo->m_vecShortInfo);
    mergeLongDbInsertProcess(pSmsInfo);
    mergeLongDelRedisCache(pSmsInfo);

    return true;
}

bool CUnityThread::mergeLongRedisRspGetShorts(UInt64 ulSession, SMSSmsInfo* pSmsInfo, redisReply* pRedisReply)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);
    CHK_NULL_RETURN_FALSE(pRedisReply);

    if (ulSession == MERGE_LONG_GET_SHORTS)
    {
        UInt32 uiElements = pSmsInfo->m_uPkTotal * 2;

        if ((pRedisReply->elements < 2) || (pRedisReply->elements > uiElements))
        {
            LogError("MergeLongKey:%s, Smsid:%s, Redis-elements:%u, Expect-elements:%u. Get all split-sms failed.",
                pSmsInfo->m_strMergeLongKey.data(),
                pSmsInfo->m_strSmsId.data(),
                pRedisReply->elements,
                uiElements);

            for (size_t i = 1; i <= pRedisReply->elements; i += 2)
            {
                string strKey = pRedisReply->element[i-1]->str;
                string strVal = pRedisReply->element[i]->str;

                LogDebug("Key(%s), Val(%s).", strKey.data(), strVal.data());
            }

            return false;
        }

        pSmsInfo->m_pLongSmsGetAllTimer = NULL;

        if (uiElements != pRedisReply->elements)
        {
            LogWarn("MergeLongKey:%s, Smsid:%s, Redis-elements:%u, Expect-elements:%u. Get all split-sms failed. Query redis again.",
                pSmsInfo->m_strMergeLongKey.data(),
                pSmsInfo->m_strSmsId.data(),
                pRedisReply->elements,
                uiElements);

            pSmsInfo->m_pLongSmsGetAllTimer = SetTimer(MERGE_LONG_REDIS_GETALL_TIMER_ID,
                                                        Comm::int2str(pSmsInfo->m_ulSequence),
                                                        1 * 1000);
            return false;
        }
    }
    else if (ulSession == MERGE_LONG_TIMEOUT_GET_SHORTS)
    {
        if (0 == pRedisReply->elements)
        {
            LogDebug("MergeLongKey:%s, Smsid:%s, Redis-elements:%u. Merge long sms time out, No need process.",
                pSmsInfo->m_strMergeLongKey.data(),
                pSmsInfo->m_strSmsId.data(),
                pRedisReply->elements);

            return false;
        }

        UInt32 uiElements = (pSmsInfo->m_uPkTotal + 1) * 2;

        if ((pRedisReply->elements < 2) || (pRedisReply->elements > uiElements))
        {
            LogWarn("MergeLongKey:%s, Smsid:%s, Redis-elements:%u, Expect-elements:%u. Internal error.",
                pSmsInfo->m_strMergeLongKey.data(),
                pSmsInfo->m_strSmsId.data(),
                pRedisReply->elements,
                uiElements);

            return false;
        }

        LogDebug("MergeLongKey:%s, Smsid:%s, Redis-elements:%u, Expect-elements:%u. Merge long sms time out.",
            pSmsInfo->m_strMergeLongKey.data(),
            pSmsInfo->m_strSmsId.data(),
            pRedisReply->elements,
            uiElements);
    }

    pSmsInfo->m_uRecvCount = 0;
    pSmsInfo->m_uShortSmsCount = 0;

    if (MERGE_LONG_TIMEOUT_GET_SHORTS == ulSession)
    {
        pSmsInfo->m_vecShortInfo.clear();
    }

    for (size_t i = 1; i <= pRedisReply->elements; i += 2)
    {
        string strKey = pRedisReply->element[i-1]->str;
        string strVal = pRedisReply->element[i]->str;

        LogDebug("MergeLongKey(%s), Key(%s), Val(%s).",
            pSmsInfo->m_strMergeLongKey.data(),
            strKey.data(),
            strVal.data());

        if (string::npos != strKey.find("shortContent"))
        {
            if (!mergeLongParseMsgDetail(pSmsInfo, strVal))
            {
                LogError("Call mergeLongParseShortMsg failed.");
                return false;
            }
        }
        else if (string::npos != strKey.find("pkNumbers"))
        {
            pSmsInfo->m_uiLongSmsRecvCountRedis = Comm::strToUint32(strVal);
        }
    }

    return true;
}

bool CUnityThread::mergeLongGetAllMsg(SMSSmsInfo* pSmsInfo, UInt32 uiSessionId)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    TRedisReq* pReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pReq);
    CHK_NULL_RETURN_FALSE(g_pRedisThreadPool);

    pReq->m_RedisCmd.append("HGETALL mergeLong:");
    pReq->m_RedisCmd.append(pSmsInfo->m_strMergeLongKey);
    pReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pReq->m_strKey= "mergeLong";
    pReq->m_iSeq = pSmsInfo->m_ulSequence;
    pReq->m_strSessionID = Comm::int2str(uiSessionId);
    pReq->m_pSender = this;

    LogDebug("Smsid(%s), pkNumber(%u:%u), Get All Short Msg, redis cmd(%s).",
        pSmsInfo->m_strSmsId.data(),
        pSmsInfo->m_uPkNumber,
        pSmsInfo->m_uPkTotal,
        pReq->m_RedisCmd.data());

    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pReq);
    return true;
}

bool CUnityThread::mergeLongParseMsgDetail(SMSSmsInfo* pSmsInfo, string &strValKey)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    ShortSmsInfo shortSms;
    map<string, string> redisMapStrings;
    Comm::splitExMap(strValKey, string("&"), redisMapStrings);

    shortSms.m_strShortContent          = Base64::Decode(redisMapStrings["content"]);
    shortSms.m_uPkNumber                = Comm::strToUint32(redisMapStrings["PkNumber"]);
    shortSms.m_uPkTotal                 = pSmsInfo->m_uPkTotal;
    shortSms.m_uSubmitId                = Comm::strToUint64(redisMapStrings["SubmitId"]);
    shortSms.m_uSequenceNum             = Comm::strToUint32(redisMapStrings["SequenceNum"]);
    shortSms.m_strLinkId                = redisMapStrings["LinkId"];
    shortSms.m_strSgipNodeAndTimeAndId  = redisMapStrings["SgipNodeAndTimeAndId"];
    shortSms.m_strSmsid                 = redisMapStrings["smsid"];
    shortSms.m_uState                   = Comm::strToUint32(redisMapStrings["state"]);
    shortSms.m_strErrCode               = redisMapStrings["errcode"];
    shortSms.m_ulC2sId                  = Comm::strToUint64(redisMapStrings["c2sId"]);

    string strPhoneVal                  = redisMapStrings["PhoneRefIDs"];

    Comm::splitExMap(strPhoneVal, ";", shortSms.m_mapPhoneRefIDs);

//    if (1 == shortSms.m_uPkNumber)
//    {
//        pSmsInfo->m_strSmsId = shortSms.m_strSmsid;
//
//        LogDebug("SmsId(%s).", pSmsInfo->m_strSmsId.data());
//    }

    pSmsInfo->m_vecShortInfo.push_back(shortSms);
    pSmsInfo->m_uShortSmsCount++;
    pSmsInfo->m_uRecvCount++;

    return true;
}

bool CUnityThread::mergeLongSmsEx(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    string strLongContent = "";
    bool iSuccess = false;
    ////merge long sms content
    for (UInt32 iPkNumber = 1; iPkNumber <= pSmsInfo->m_uPkTotal; iPkNumber++)
    {
        iSuccess = false;
        UInt32 uInfoSize = pSmsInfo->m_vecShortInfo.size();
        for (UInt32 i = 0; i < uInfoSize;i++)
        {
            if (iPkNumber == pSmsInfo->m_vecShortInfo.at(i).m_uPkNumber)
            {
                strLongContent.append(pSmsInfo->m_vecShortInfo.at(i).m_strShortContent);
                iSuccess = true;
                break;
            }

        }

        if (false == iSuccess)
        {
            break;
        }
    }

    if (false == iSuccess)
    {
        LogError("MergeLongKey:%s, Smsid:%s, can not get all short msgs.",
            pSmsInfo->m_strMergeLongKey.data(),
            pSmsInfo->m_strSmsId.data());

        return false;
    }

    pSmsInfo->m_strContent.assign(strLongContent);
    pSmsInfo->m_strContentWithSign.assign(strLongContent);

    LogNotice("MergeLongKey:%s, smsid:%s, merge long sms in redis success. content:%s.",
        pSmsInfo->m_strMergeLongKey.data(),
        pSmsInfo->m_strSmsId.data(),
        pSmsInfo->m_strContent.data());

    return true;
}

bool CUnityThread::mergeLongDelRedisCache(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    TRedisReq* pReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pReq);
    CHK_NULL_RETURN_FALSE(g_pRedisThreadPool);

    pReq->m_RedisCmd.append("DEL mergeLong:");
    pReq->m_RedisCmd.append(pSmsInfo->m_strMergeLongKey);
    pReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pReq->m_strKey= "mergeLong";

    LogDebug("Smsid(%s), redis cmd(%s).",
        pSmsInfo->m_strSmsId.data(),
        pReq->m_RedisCmd.data());

    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pReq);

    return true;
}

bool CUnityThread::mergeLongReleaseResource(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    LogDebug("MergeLongKey:%s, merge Long sms Release Resource.",
        pSmsInfo->m_strMergeLongKey.data());

    LongSmsRedisMap::iterator iter = m_mapLongSmsMngRedis.find(pSmsInfo->m_ulSequence);

    if (iter == m_mapLongSmsMngRedis.end())
    {
        m_mapLongSmsMngRedis.erase(iter);
    }

    SAFE_DELETE(pSmsInfo->m_pLongSmsTimer);
    SAFE_DELETE(pSmsInfo->m_pLongSmsRedisTimer);
    SAFE_DELETE(pSmsInfo);
    return true;
}


UInt32 CUnityThread::mergeLongCheckMsgParam(CTcpSubmitReqMsg* pReq,
                                                    SMSSmsInfo* pSmsInfo,
                                                    const string& utf8Content,
                                                    const SmsAccount& AccountInfo)
{
    if ((NULL == pReq) || (NULL == pSmsInfo))
    {
        LogError("pReq is NULL.");
        return TCP_RESULT_OTHER_ERROR;
    }

    ////check account status
    if ( 1 != AccountInfo.m_uStatus )
    {
        LogElk("[%s: ] account[%s] status[%d] is not ok!",
                pSmsInfo->m_strSmsId.data(),
                pSmsInfo->m_strClientId.data(),
                AccountInfo.m_uStatus);
        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode =  ACCOUNT_LOCKED;
        pSmsInfo->m_strYZXErrCode = ACCOUNT_LOCKED;
        return TCP_RESULT_AUTH_ERROR;
    }

    //CHECK ENCODE
    utils::UTFString utfHelper;
    char strDst[64] = {0};
    if(!utfHelper.IsTextUTF8(utf8Content.c_str(), utf8Content.length(), strDst))
    {
        LogElk("[%s: ] content[%x,%x,%x,%x,%x,%x] is over utf8!",
                pSmsInfo->m_strSmsId.data(),
                strDst[0], strDst[1], strDst[2],
                strDst[3], strDst[4], strDst[5]);
        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = ERRORCODE_CONTENT_ENCODE_ERROR;
        pSmsInfo->m_strYZXErrCode = ERRORCODE_CONTENT_ENCODE_ERROR;
        return TCP_RESULT_ERROR_MAX;
    }

    ////check msglength
    UInt32 msgLength = 0;
    UInt32 maxWord = 0;
    msgLength = utfHelper.getUtf8Length(utf8Content);
    if (true == Comm::IncludeChinese((char*)utf8Content.data()))
    {
        maxWord = 70;
    }
    else
    {
        maxWord = 140;
    }
//    if ((pSmsInfo->m_uAccountExtAttr & ACCOUNT_EXT_PORT_TO_SIGN)
//    && (1 == pSmsInfo->m_uNeedExtend)
//    && (pReq->m_uPkNumber == pReq->m_uPkTotal))
//    {
//    }
//    else
//    {
        if( pReq->m_uMsgLength > 140 || msgLength > maxWord )
        {
            LogElk("[%s: ] content length[%d] or msglenth[%u]is too long!",pSmsInfo->m_strSmsId.data(),pReq->m_uMsgLength,msgLength);

            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = CONTENT_LENGTH_TOO_LONG;
            pSmsInfo->m_strYZXErrCode = CONTENT_LENGTH_TOO_LONG;
            return TCP_RESULT_MSG_LENGTH_ERROR;
        }
//    }

    ////check phone num
    if ( pReq->m_uPhoneNum > 100 )
    {
        LogElk("[%s: ] phone count[%u] is too many!",
                pSmsInfo->m_strSmsId.data(),pReq->m_uPhoneNum);
        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = PHONE_COUNT_TOO_MANY;
        pSmsInfo->m_strYZXErrCode = PHONE_COUNT_TOO_MANY;
        return TCP_RESULT_PARAM_ERROR;
    }

    //long msg
    if( 1 == pSmsInfo->m_uLongSms )
    {
        //check pktotal
        if ( pReq->m_uPkTotal > 10 )
        {
            LogElk("[%s: ] pktotal[%u] is too many!",
                    pSmsInfo->m_strSmsId.data(),pReq->m_uPkTotal);
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = PKTOTAL_TOO_MANY;
            pSmsInfo->m_strYZXErrCode = PKTOTAL_TOO_MANY;
            return TCP_RESULT_OTHER_ERROR;
        }

        if ( pSmsInfo->m_uPkNumber > pSmsInfo->m_uPkTotal )
        {
            LogElk("[%s: ] account[%s] pkNumber[%u] > pkTotal[%u]!",
                    pSmsInfo->m_strSmsId.data(),
                    pSmsInfo->m_strClientId.data(),
                    pSmsInfo->m_uPkNumber,pSmsInfo->m_uPkTotal );
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = PKNUMBER_BIG_PKTOTAL;
            pSmsInfo->m_strYZXErrCode = PKNUMBER_BIG_PKTOTAL;
            return TCP_RESULT_OTHER_ERROR;
        }
    }
    else
    {
        if ( false == CheckSignExtendAndKeyWord( pSmsInfo ))
        {
            LogElk("[%s: ] account[%s] CheckSignExtendAndKeyWord is failed!",
                     pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strClientId.data());
            pSmsInfo->m_strYZXErrCode = pSmsInfo->m_strErrorCode;
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            return TCP_RESULT_PARAM_ERROR;
        }
    }

    vector<string> vecBlackList;
    if ( false == CheckPhoneFormatAndBlackList(pSmsInfo, pSmsInfo->m_ErrorPhoneList, vecBlackList))
    {
        LogElk("[%s: ] account[%s] CheckPhoneFormatAndBlackList is failed!",
                pSmsInfo->m_strSmsId.data(),
                pSmsInfo->m_strClientId.data());
        pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
        pSmsInfo->m_strYZXErrCode = PHONE_FORMAT_IS_ERROR;
        return TCP_RESULT_PARAM_ERROR ;
    }

    pSmsInfo->m_strErrorCode = "";
    pSmsInfo->m_uState = SMS_STATUS_INITIAL;

    return TCP_RESULT_OK;

}

bool CUnityThread::mergeLongDbInsertProcess(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    for (UInt32 i = 0; i < pSmsInfo->m_vecShortInfo.size(); i++)
    {
        pSmsInfo->m_uRecvCount = i + 1;
        pSmsInfo->m_uState = pSmsInfo->m_vecShortInfo.at(i).m_uState;
        pSmsInfo->m_strErrorCode = pSmsInfo->m_vecShortInfo.at(i).m_strErrCode;
        pSmsInfo->m_strYZXErrCode = pSmsInfo->m_vecShortInfo.at(i).m_strErrCode;

        mergeLongInsertAccessDb(pSmsInfo, pSmsInfo->m_Phonelist);

        if (!pSmsInfo->m_RepeatPhonelist.empty())
        {
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = REPEAT_PHONE;
            pSmsInfo->m_strYZXErrCode = REPEAT_PHONE;

            mergeLongInsertAccessDb(pSmsInfo, pSmsInfo->m_RepeatPhonelist);

        }

        if (!pSmsInfo->m_ErrorPhoneList.empty())
        {
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
            pSmsInfo->m_strYZXErrCode = PHONE_FORMAT_IS_ERROR;

            mergeLongInsertAccessDb(pSmsInfo, pSmsInfo->m_ErrorPhoneList);
        }
    }

    if (!pSmsInfo->m_RepeatPhonelist.empty())
    {
        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = REPEAT_PHONE;
        pSmsInfo->m_strYZXErrCode = REPEAT_PHONE;

        mergeLongSendFailedDeliver(pSmsInfo, pSmsInfo->m_RepeatPhonelist, pSmsInfo->m_vecShortInfo);

    }

    if (!pSmsInfo->m_ErrorPhoneList.empty())
    {
        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
        pSmsInfo->m_strYZXErrCode = PHONE_FORMAT_IS_ERROR;
        mergeLongSendFailedDeliver(pSmsInfo, pSmsInfo->m_ErrorPhoneList, pSmsInfo->m_vecShortInfo);
    }

    return true;
}

bool CUnityThread::mergeLongMsgRealProc(SMSSmsInfo* pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    if (mergeLongSmsEx(pSmsInfo))
    {
        if (!CheckSignExtendAndKeyWord(pSmsInfo))
        {
            LogElk("MergeLongKey:%s, smsid:%s, clientid:%s, Call CheckSignExtendAndKeyWord failed.",
                pSmsInfo->m_strMergeLongKey.data(),
                pSmsInfo->m_strSmsId.data(),
                pSmsInfo->m_strClientId.data());

            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strYZXErrCode = pSmsInfo->m_strErrorCode;

            foreach(ShortSmsInfo& sms, pSmsInfo->m_vecShortInfo)
            {
                sms.m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
                sms.m_strErrCode = pSmsInfo->m_strErrorCode;
            }

            mergeLongDbInsertProcess(pSmsInfo);
            mergeLongSendFailedDeliver(pSmsInfo, pSmsInfo->m_Phonelist, pSmsInfo->m_vecShortInfo);
            return false;
        }
        else
        {
            //proSetInfoToRedis(pSmsInfo);
        }
    }
    else
    {
        LogElk("MergeLongKey:%s, smsid:%s, merge log messege error!",
            pSmsInfo->m_strMergeLongKey.data(),
            pSmsInfo->m_strSmsId.data());

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        pSmsInfo->m_strErrorCode = CAN_NOT_GET_ALL_SHORT_MSGS;
        pSmsInfo->m_strYZXErrCode = CAN_NOT_GET_ALL_SHORT_MSGS;

        foreach(ShortSmsInfo& sms, pSmsInfo->m_vecShortInfo)
        {
            sms.m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            sms.m_strErrCode = pSmsInfo->m_strErrorCode;
        }

        mergeLongDbInsertProcess(pSmsInfo);
        mergeLongSendFailedDeliver(pSmsInfo, pSmsInfo->m_Phonelist, pSmsInfo->m_vecShortInfo);
        return false;
    }

    return true;
}

void CUnityThread::mergeLongSendFailedDeliver(SMSSmsInfo* pSmsInfo,
                                                const vector<string>& phoneList,
                                                const vector<ShortSmsInfo>& vecShortInfo)
{
    LogDebug("phoneCount:%u, ShortSmsCount:%u", phoneList.size(), vecShortInfo.size());

    for(UInt32 i = 0; i < phoneList.size(); i++)
    {
        for( UInt32 j = 0; j < vecShortInfo.size(); j++ )
        {
            CTcpDeliverReqMsg* pDeliver = new CTcpDeliverReqMsg();
            pDeliver->m_strLinkId = pSmsInfo->m_vecShortInfo.at(j).m_strLinkId;
            pDeliver->m_uResult = 8;
            pDeliver->m_uSubmitId = pSmsInfo->m_vecShortInfo.at(j).m_uSubmitId;
            pDeliver->m_strPhone = phoneList.at(i);
            pDeliver->m_strYZXErrCode = pSmsInfo->m_strYZXErrCode;
            pDeliver->m_iMsgType = MSGTYPE_TCP_DELIVER_REQ;

            LogDebug("== pack failed deliver ==, phone:%s, ErrCode:%s",
                pDeliver->m_strPhone.data(),
                pDeliver->m_strYZXErrCode.data());

            if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP)
            {
                g_pCMPPServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_CMPP3)
            {
                g_pCMPP3ServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMPP)
            {
                g_pSMPPServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SMGP)
            {
                g_pSMGPServiceThread->PostMsg(pDeliver);
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_HTTP)
            {
                delete pDeliver;
                pDeliver = NULL;
            }
            else if (pSmsInfo->m_uSmsFrom == SMS_FROM_ACCESS_SGIP)
            {
                delete pDeliver;
                pDeliver = NULL;

                CSgipFailRtReqMsg* pFailRt = new CSgipFailRtReqMsg();
                pFailRt->m_strClientId.assign(pSmsInfo->m_strClientId);
                pFailRt->m_strPhone.assign(phoneList.at(i));
                pFailRt->m_uState = 8;
                pFailRt->m_iMsgType = MSGTYPE_TCP_DELIVER_REQ;

                std::vector<string> vecSequence;
                Comm comm;
                comm.splitExVector(pSmsInfo->m_vecShortInfo.at(j).m_strSgipNodeAndTimeAndId,"|",vecSequence);
                if (vecSequence.size() != 3)
                {
                    LogElk("sequence[%s] is analyze failed.",
                            pSmsInfo->m_vecShortInfo.at(j).m_strSgipNodeAndTimeAndId.data());
                    pFailRt->m_uSequenceIdNode = 0;
                    pFailRt->m_uSequenceIdTime = 0;
                    pFailRt->m_uSequenceId = 0;
                }
                else
                {
                    pFailRt->m_uSequenceIdNode = atoi(vecSequence.at(0).data());
                    pFailRt->m_uSequenceIdTime = atoi(vecSequence.at(1).data());
                    pFailRt->m_uSequenceId = atoi(vecSequence.at(2).data());
                }
                g_pSgipRtAndMoThread->PostMsg(pFailRt);
            }
            else
            {
                LogElk("[%s:%s] smsfrom[%u] is invalid type.",
                        pSmsInfo->m_strSmsId.data(),pDeliver->m_strPhone.data(),pSmsInfo->m_uSmsFrom);
                delete pDeliver;
            }
        }
    }

    return;
}

void CUnityThread::mergeLongInsertAccessDb(SMSSmsInfo* pSmsInfo, const vector<string>& phonelist)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return;
    }

    UInt32 j = 0;
    if (pSmsInfo->m_uPkTotal != 1)
    {
        j = pSmsInfo->m_uRecvCount - 1;
    }

    string strDate = pSmsInfo->m_strDate.substr(0, 8);  //get date.

    string strTempDesc = "";
    map<string,string>::iterator itrDesc = m_mapSystemErrorCode.find(pSmsInfo->m_strErrorCode);
    if (itrDesc != m_mapSystemErrorCode.end())
    {
        strTempDesc.assign(pSmsInfo->m_strErrorCode);
        strTempDesc.append("*");
        strTempDesc.append(itrDesc->second);
    }
    else
    {
        strTempDesc.assign(pSmsInfo->m_strErrorCode);
    }

    UInt32 uOperaterType = 0;
    UInt32  phoneCount = phonelist.size();
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

    ////LogDebug("here is InsertAccessDb");
    for(UInt32 i = 0; i < phoneCount; i++)
    {
        string phone = phonelist.at(i);
        if (m_honeDao.m_pVerify->CheckPhoneFormat(phone))
        {
            uOperaterType = m_honeDao.getPhoneType(phone);
        }
        string tmpId = "";
        if("" != pSmsInfo->m_strErrorCode)  //20160902 add by fangjinxiong.
        {
            tmpId = getUUID();
        }
        else
        {
            tmpId = pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[phone];     //here only ok phones, repeatphones need generat another uuid
        }

        UInt64 ulC2sId = pSmsInfo->m_vecShortInfo.at(j).m_ulC2sId;
        string msg = pSmsInfo->m_vecShortInfo.at(j).m_strShortContent;

        char content[3072] = {0};
        char sign[128] = {0};
        char strUid[64] = {0};
        char strTempParam[2048] = {0};
        UInt32 position = pSmsInfo->m_strSign.length();
        if(pSmsInfo->m_strSign.length() > 100)
        {
            position = Comm::getSubStr(pSmsInfo->m_strSign,100);
        }
        if(MysqlConn != NULL)
        {
            mysql_real_escape_string(MysqlConn, content, msg.data(), msg.length());

            //将uid字段转义
            mysql_real_escape_string(MysqlConn, strUid, pSmsInfo->m_strUid.substr(0, 60).data(), pSmsInfo->m_strUid.substr(0, 60).length());

            //将sign字段转义
            mysql_real_escape_string(MysqlConn, sign, pSmsInfo->m_strSign.substr(0, position).data(), pSmsInfo->m_strSign.substr(0, position).length());

            if (!pSmsInfo->m_strTemplateParam.empty())
            {
                mysql_real_escape_string(MysqlConn, strTempParam, pSmsInfo->m_strTemplateParam.data(),
                                 pSmsInfo->m_strTemplateParam.length());
            }
        }

        char sql[10240]  = {0};
        utils::UTFString utfHelper;

        UInt32 uiLongSms = 0;
        if (pSmsInfo->m_uPkTotal > 1)
        {
            uiLongSms = 1;
        }
        else
        {
            int iLength = utfHelper.getUtf8Length(pSmsInfo->m_strContentWithSign);
            uiLongSms = (iLength > 70) ? 1 : 0;
        }

        int iLength2 = utfHelper.getUtf8Length(msg);
        UInt32 uiChargeNum = (iLength2 > 70) ? ((iLength2 + 66) / 67) : 1;
        UInt32 uiShowSignType = (1 == pSmsInfo->m_uiSignManualFlag) ? 1 : pSmsInfo->m_uShowSignType;
        string strBelongSale = Comm::int2str(pSmsInfo->m_uBelong_sale);

        snprintf(sql, 10240,"insert into t_sms_access_%u_%s(id,content,srcphone,phone,smscnt,smsindex,sign,submitid,smsid,clientid,"
                     "operatorstype,smsfrom,state,errorcode,date,innerErrorcode,channelid,smstype,charge_num,paytype,agent_id,username ,isoverratecharge,"
                     "uid,showsigntype,product_type,c2s_id,process_times,longsms,channelcnt,template_id,temp_params,sid,belong_sale,sub_id)"
                     "  values('%s', '%s', '%s', '%s', '%u', '%u', '%s', '%lu', '%s', '%s', '%u',"
                     "'%u', '%d', '%s', '%s','%s','%d','%d','%u','%u','%lu','%s','%u','%s','%u','%u','%lu','%u','%u','%d', %s, '%s', '%s', %s ,'%s');",
                 pSmsInfo->m_uIdentify,
                 strDate.data(),
                 tmpId.data(),
                 content,
                 pSmsInfo->m_strSrcPhone.substr(0, 20).data(),
                 phonelist[i].substr(0, 20).data(),
                 pSmsInfo->m_vecShortInfo.at(j).m_uPkTotal,
                 pSmsInfo->m_vecShortInfo.at(j).m_uPkNumber,
                 sign,
                 pSmsInfo->m_vecShortInfo.at(j).m_uSubmitId,
                 pSmsInfo->m_strSmsId.data(),
                 pSmsInfo->m_strClientId.data(),
                 uOperaterType,
                 pSmsInfo->m_uSmsFrom,
                 pSmsInfo->m_uState,
                 strTempDesc.data(),
                 pSmsInfo->m_strDate.data(),
                 strTempDesc.data(),
                 0,
                 atoi(pSmsInfo->m_strSmsType.data()),
                 uiChargeNum,
                 pSmsInfo->m_uPayType,
                 pSmsInfo->m_uAgentId,
                 pSmsInfo->m_strUserName.data(),
                 pSmsInfo->m_uOverRateCharge,
                 strUid,
                 uiShowSignType,
                 pSmsInfo->m_uProductType,
                 (0 == ulC2sId) ? g_uComponentId : ulC2sId,
                 pSmsInfo->m_uProcessTimes,
                 uiLongSms,
                 1,
                 (pSmsInfo->m_strTemplateId.empty()) ? "NULL" : pSmsInfo->m_strTemplateId.data(),
                 strTempParam,
                 " ",
                 (!strBelongSale.compare("0")) ? "NULL" : strBelongSale.data(),
                 "0");

        LogDebug("[%s: ] access insert DB sql[%s].",pSmsInfo->m_strSmsId.data(), sql);

        publishMqC2sDbMsg(tmpId, sql);

        MONITOR_INIT( MONITOR_RECEIVE_MT_SMS );
        MONITOR_VAL_SET("clientid", pSmsInfo->m_strClientId);
        MONITOR_VAL_SET("username", pSmsInfo->m_strUserName);
        MONITOR_VAL_SET_INT("operatorstype", uOperaterType);
        MONITOR_VAL_SET_INT("smsfrom", pSmsInfo->m_uSmsFrom);
        MONITOR_VAL_SET_INT("paytype", pSmsInfo->m_uPayType);
        MONITOR_VAL_SET("sign", sign );
        MONITOR_VAL_SET("smstype", pSmsInfo->m_strSmsType);
        MONITOR_VAL_SET("date", pSmsInfo->m_strDate);
        MONITOR_VAL_SET("uid",  strUid);
        MONITOR_VAL_SET("smsid", pSmsInfo->m_strSmsId);
        MONITOR_VAL_SET("phone", phonelist[i].substr(0, 20).data());
        MONITOR_VAL_SET_INT("id", pSmsInfo->m_uIdentify);
        MONITOR_VAL_SET_INT("state", pSmsInfo->m_uState);
        MONITOR_VAL_SET("errrorcode", pSmsInfo->m_strErrorCode);
        MONITOR_VAL_SET("errordesc", strTempDesc);
        MONITOR_VAL_SET_INT("component_id", g_uComponentId);
        MONITOR_PUBLISH(g_pMQMonitorPublishThread );
    }
}


