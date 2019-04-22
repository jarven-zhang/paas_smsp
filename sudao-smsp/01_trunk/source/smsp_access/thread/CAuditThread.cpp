#include "CAuditThread.h"
#include "UrlCode.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "main.h"
#include "global.h"
#include "Fmt.h"
#include "base64.h"
#include "HttpParams.h"
#include "LogMacro.h"

#include "boost/foreach.hpp"

////////////////////////////////////////////////////////////////////////////

#define foreach BOOST_FOREACH


AuditThread* g_pAuditThread = NULL;

const UInt64 TIMER_ID_GET_CACHE_DETAIL  = 21;

const UInt64 AUDIT_REIDS_EXPIRE_TIME    = 172800; //48h
const UInt64 REDIS_EXPIRE_TIME_54H      = 194400; //54h
const UInt64 REDIS_EXPIRE_TIME_24H      = 86400;  //24h

////////////////////////////////////////////////////////////////////////////

bool startAuditThread()
{
    g_pAuditThread = new AuditThread("AuditThread");
    INIT_CHK_NULL_RET_FALSE(g_pAuditThread);
    INIT_CHK_FUNC_RET_FALSE(g_pAuditThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pAuditThread->CreateThread());
    return true;
}

void debugAuditThread()
{
    AuditReqMsg* pReq = new AuditReqMsg();

    pReq->m_strClientId = "b01351";
    pReq->m_strUserName = "test";
    pReq->m_strSmsId = "cfcf7d65-7ac8-4c84-b53a-7629cb397b18";
    pReq->m_strPhone = "13027915673";
    pReq->m_strSign = "t_sign";
    pReq->m_strSignSimple = "t_sign";
    pReq->m_strContent = "t_content";
    pReq->m_strContentSimple = "t_content";
    pReq->m_strSmsType = "5";
    pReq->m_uiPayType = 1;
    pReq->m_uiOperater = 2;
    pReq->m_strC2sDate = "20180703174027";

    pReq->m_uiSmsFrom = 3;
    pReq->m_uiShowSignType = 1;
    pReq->m_strSignPort = "";
    pReq->m_strExtpendPort = "";
    pReq->m_strIds = "9087f6c5-37b9-49f3-ba14-fcae6daafc5e";
    pReq->m_uiSmsNum = 1;
    pReq->m_strCSid = "100020210800";

    pReq->m_eMatchSmartTemplateRes = MatchVariable_MatchRegex;

    pReq->m_iMsgType = MSGTYPE_AUDIT_REQ;
    g_pAuditThread->PostMsg(pReq);

    LogNoticeP("1111111111111");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

AuditThread::Session::Session()
{
    init();
}

AuditThread::Session::Session(AuditThread* pThread)
{
    init();

    m_pThread = pThread;
    m_ulSequence = pThread->m_snManager.getSn();
    pThread->m_mapSession[m_ulSequence] = this;

    LogDebug("Sequence[%lu] add audit session.",
        m_ulSequence);
}

AuditThread::Session::~Session()
{
    if (0 != m_ulSequence)
    {
        m_pThread->m_mapSession.erase(m_ulSequence);

        LogDebug("Sequence[%lu] del audit session.",
            m_ulSequence);
    }
}

void AuditThread::Session::init()
{
    m_pThread = NULL;
    m_ulSequence = 0;

    m_uiAuditState = 0;
    m_uiAuditResult = 0;

    m_uiGroupSendLimitFlag = 0;
    m_uiGroupsendLimUserFlag = 0;

    m_uiState = 0;
}

void AuditThread::Session::getAuditContent()
{
    string s;
    s.append(m_auditReqMsg.m_strClientId);
    s.append("|");
    s.append(m_auditReqMsg.m_strSign);
    s.append("|");
    s.append(m_auditReqMsg.m_strContent);
    s.append("|");
    s.append(m_auditReqMsg.m_strSmsType);

    m_strAuditContent = Comm::GetStrMd5(s);
    m_uiGroupSendLimitFlag = false;
}

void AuditThread::Session::getGroupSendLimitAuditContent()
{
    string s;
    s.append(m_auditReqMsg.m_strClientId);
    s.append("|");
    s.append(m_auditReqMsg.m_strSign);
    s.append("|");
    s.append(m_auditReqMsg.m_strContent);
    s.append("|");
    s.append(m_auditReqMsg.m_strSmsType);
    s.append("|");
    s.append(to_string(m_auditReqMsg.m_uiOperater));

    m_strAuditContent = Comm::getCurrentDayMMDD() + "_" + Comm::GetStrMd5(s);
    m_uiGroupSendLimitFlag = true;
}

void AuditThread::Session::packMsg2Audit()
{
    m_strSendMsg.append("clientid=");
    m_strSendMsg.append(m_auditReqMsg.m_strClientId);

    m_strSendMsg.append("&username=");
    m_strSendMsg.append(Base64::Encode(m_auditReqMsg.m_strUserName));

    m_strSendMsg.append("&smsid=");
    m_strSendMsg.append(m_auditReqMsg.m_strSmsId);

    m_strSendMsg.append("&phone=");
    m_strSendMsg.append(m_auditReqMsg.m_strPhone);

    m_strSendMsg.append("&sign=");
    m_strSendMsg.append(Base64::Encode(m_auditReqMsg.m_strSign));

    m_strSendMsg.append("&content=");
    m_strSendMsg.append(Base64::Encode(m_auditReqMsg.m_strContent));

    m_strSendMsg.append("&auditcontent=");
    m_strSendMsg.append(m_strAuditContent);

    m_strSendMsg.append("&smstype=");
    m_strSendMsg.append(m_auditReqMsg.m_strSmsType);

    m_strSendMsg.append("&paytype=");
    m_strSendMsg.append(Comm::int2str(m_auditReqMsg.m_uiPayType));

    m_strSendMsg.append("&csdate=");
    m_strSendMsg.append(m_auditReqMsg.m_strC2sDate);

    if (1 == m_uiGroupSendLimitFlag)
    {
        SmsAccountMap::iterator iter;
        CHK_MAP_FIND_STR_RET(m_pThread->m_mapAccount, iter, m_auditReqMsg.m_strClientId);
        const SmsAccount& smsAccount = iter->second;

        m_strSendMsg.append("&operatortype=");
        m_strSendMsg.append(to_string(m_auditReqMsg.m_uiOperater));

        m_strSendMsg.append("&groupsendlim_userflag=");
        m_strSendMsg.append(to_string(smsAccount.m_uGroupsendLimUserflag));

        m_strSendMsg.append("&groupsendlim_time=");
        m_strSendMsg.append(to_string(smsAccount.m_uGroupsendLimTime));

        m_strSendMsg.append("&groupsendlim_num=");
        m_strSendMsg.append(to_string(smsAccount.m_uGroupsendLimNum));
    }
}

void AuditThread::Session::packMsg2MqC2sIoDown()
{
    m_strSendMsg.clear();

    m_strSendMsg.append("clientId=");
    m_strSendMsg.append(m_auditReqMsg.m_strClientId);

    m_strSendMsg.append("&userName=");
    m_strSendMsg.append(Base64::Encode(m_auditReqMsg.m_strUserName));

    m_strSendMsg.append("&smsId=");
    m_strSendMsg.append(m_auditReqMsg.m_strSmsId);

    m_strSendMsg.append("&phone=");
    m_strSendMsg.append(m_auditReqMsg.m_strPhone);

    m_strSendMsg.append("&sign=");
    m_strSendMsg.append(Base64::Encode(m_auditReqMsg.m_strSign));

    m_strSendMsg.append("&content=");
    m_strSendMsg.append(Base64::Encode(m_auditReqMsg.m_strContent));

    m_strSendMsg.append("&smsType=");
    m_strSendMsg.append(m_auditReqMsg.m_strSmsType);

    m_strSendMsg.append("&smsfrom=");
    m_strSendMsg.append(to_string(m_auditReqMsg.m_uiSmsFrom));

    m_strSendMsg.append("&signport=");
    m_strSendMsg.append(m_auditReqMsg.m_strSignPort);

    m_strSendMsg.append("&showsigntype=");
    m_strSendMsg.append(to_string(m_auditReqMsg.m_uiShowSignType));

    m_strSendMsg.append("&userextpendport=");
    m_strSendMsg.append(m_auditReqMsg.m_strExtpendPort);

    m_strSendMsg.append("&ids=");
    m_strSendMsg.append(m_auditReqMsg.m_strIds);

    m_strSendMsg.append("&csid=");
    m_strSendMsg.append(m_auditReqMsg.m_strCSid);

    m_strSendMsg.append("&csdate=");
    m_strSendMsg.append(m_auditReqMsg.m_strC2sDate);

    m_strSendMsg.append("&audit_flag=");
    m_strSendMsg.append("1");

    m_strSendMsg.append("&send_limit_flag=");
    m_strSendMsg.append(Comm::int2str(m_auditReqMsg.m_iClientSendLimitCtrlTypeFlag));
 
}

void AuditThread::Session::packMsg2MqC2sIoUp()
{
    m_strSendMsg.clear();

    m_strSendMsg.append("type=0&clientid=");
    m_strSendMsg.append(m_auditReqMsg.m_strClientId);

    m_strSendMsg.append("&smsid=");
    m_strSendMsg.append(m_auditReqMsg.m_strSmsId);

    m_strSendMsg.append("&phone=");
    m_strSendMsg.append(m_auditReqMsg.m_strPhone);

    m_strSendMsg.append("&status=6&desc=");
    m_strSendMsg.append(Base64::Encode(m_strErrCode));

    m_strSendMsg.append("&reportdesc=");
    m_strSendMsg.append(Base64::Encode(m_strErrCode));

    m_strSendMsg.append("&channelcnt=");
    m_strSendMsg.append(to_string(m_auditReqMsg.m_uiSmsNum));

    m_strSendMsg.append("&reportTime=");
    m_strSendMsg.append(to_string(time(NULL)));

    m_strSendMsg.append("&smsfrom=");
    m_strSendMsg.append(to_string(m_auditReqMsg.m_uiSmsFrom));
}

void AuditThread::Session::praseAuditResMsg(cs_t strData)
{
    web::HttpParams param;
    param.Parse(strData);

    m_strAuditDate = Comm::getCurrentTime();
    m_strAuditContent = param._map["auditcontent"];
    m_strAuditId = param._map["auditid"];
    m_uiAuditResult = to_uint<UInt32>(param._map["status"]);
    m_uiGroupSendLimitFlag = to_uint<UInt32>(param._map["groupsendlim_flag"]);
    m_uiGroupsendLimUserFlag = to_uint<UInt32>(param._map["groupsendlim_userflag"]);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

AuditThread::AuditThread(const char *name) : CThread(name)
{
    m_iAuditTimeOut = 5 * 1000;

    // AUDIT_OUT_NUMBER default: 200
    m_uiSysParaAuditOutNumber = 200;

    m_uiSysParaAuditReqMqNum = 2;

    // AUDIT_SM_TIMEAREA default: 00:00|23:59
    m_strSysParaAuditTimeAreaBegin = "00:00";
    m_strSysParaAuditTimeAreaEnd = "23:59";

    m_uiSysParaGroupSendLimCfg = 0;

    m_iAuditKeywordCovRegular = 0;
}

AuditThread::~AuditThread()
{
}

bool AuditThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());

    m_phoneDao.Init();
    m_chineseConvert.Init();
    m_keyWordCheck.initParam();

    try
    {
        string strReg = "([0-1][0-9]|[2][0-4]):[0-5][0-9]";
        m_regTime.assign(strReg.data());
    }
    catch(...)
    {
        printf("time reg compile error.");
        return false;
    }

    //system param
    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    initSysPara(sysParamMap);


    g_pRuleLoadThread->getSmsAccountMap(m_mapAccount);
    g_pRuleLoadThread->getSystemErrorCode(m_mapSysErrCode);
    g_pRuleLoadThread->getComponentRefMq(m_mapComponentRefMq);
    g_pRuleLoadThread->getComponentConfig(m_mapComponentCfg);
    g_pRuleLoadThread->getPhoneAreaMap(m_mapPhoneArea);

    g_pRuleLoadThread->getMqConfig(m_mapMqCfg);

    INIT_CHK_FUNC_RET_FALSE(initAuditReqMqQueue());

    map<string,list<string> > mapListSet;
    g_pRuleLoadThread->getNoAuditKeyWordMap(mapListSet);
    initAuditOverKeyWordSearchTree(mapListSet, m_NoAuditKeyWordMap);

    m_pTimerGetCacheDetail = SetTimer(TIMER_ID_GET_CACHE_DETAIL,
                                        "rpop cache_detail:" + to_string(g_uComponentId),
                                        m_iAuditTimeOut);

    return true;
}

void AuditThread::initSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "KEYWORD_CONVERT_REGULAR";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;
        if (strTmp.empty())
        {
            LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
            break;
        }

        std::map<std::string,std::string> mapSet;
        Comm::splitMap(strTmp, "|", ";", mapSet);
        m_iAuditKeywordCovRegular = atoi(mapSet["2"].data());
        m_keyWordCheck.initKeyWordRegular(m_iAuditKeywordCovRegular);
    }
    while (0);

    LogNotice("System parameter(%s) value(m_iAuditKeywordCovRegular[%d]).",
        strSysPara.c_str(),
        m_iAuditKeywordCovRegular);

    do
    {
        strSysPara = "AUDIT_OUT_NUMBER";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uiSysParaAuditOutNumber = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).",
        strSysPara.c_str(), m_uiSysParaAuditOutNumber);

    do
    {
        strSysPara = "AUDIT_SM_TIMEAREA";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;
        std::string::size_type pos = strTmp.find_last_of("|");
        if(string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                strSysPara.c_str(), strTmp.c_str());
            break;
        }

        m_strSysParaAuditTimeAreaBegin = strTmp.substr(0, pos);
        m_strSysParaAuditTimeAreaEnd = strTmp.substr(pos+1);

        if (!TimeRegCheck(m_strSysParaAuditTimeAreaBegin))
        {
            LogError("Invalid system parameter(%s) value(%s). BeginTime(%s).",
                strSysPara.c_str(), strTmp.c_str(), m_strSysParaAuditTimeAreaBegin.c_str());
            break;
        }

        if (!TimeRegCheck(m_strSysParaAuditTimeAreaEnd))
        {
            LogError("Invalid system parameter(%s) value(%s). EndTime(%s).",
                strSysPara.c_str(), strTmp.c_str(), m_strSysParaAuditTimeAreaEnd.c_str());
            break;
        }
    }
    while (0);

    LogNotice("System parameter(%s) value(%s, %s).",
        strSysPara.c_str(),
        m_strSysParaAuditTimeAreaBegin.c_str(),
        m_strSysParaAuditTimeAreaEnd.c_str());

    do
    {
        strSysPara = "MQ_C2SDB_AUDITREQQUEUE_NUM";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        UInt32 uTmp = atoi(strTmp.c_str());
        if (uTmp < 1 || 10 < uTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %u).",
                strSysPara.c_str(), strTmp.c_str(), uTmp);
            break;
        }

        m_uiSysParaAuditReqMqNum = uTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).",
            strSysPara.c_str(), m_uiSysParaAuditReqMqNum);

    do
    {
        strSysPara = "GROUP_SEND_LIMIT_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if (nTmp < 0)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uiSysParaGroupSendLimCfg = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uiSysParaGroupSendLimCfg);
}

bool AuditThread::initAuditReqMqQueue()
{
    vector<MqConfig> vecAuditReqMqCfg;

    set<UInt64> mqIdSet;
    for (ComponentRefMqMapIter iter = m_mapComponentRefMq.begin(); iter != m_mapComponentRefMq.end(); ++iter)
    {
        const ComponentRefMq& refMq = iter->second;

        if ((g_uComponentId == refMq.m_uComponentId)
         && (MESSAGE_AUDIT_CONTENT_REQ == refMq.m_strMessageType)
         && (PRODUCT_MODE == refMq.m_uMode))
        {
            MqConfigMapIter iterMqCfg;
            CHK_MAP_FIND_UINT_CONTINUE(m_mapMqCfg, iterMqCfg, refMq.m_uMqId);
            const MqConfig& mqCfg = iterMqCfg->second;

            vecAuditReqMqCfg.push_back(mqCfg);

            LogNoticeP("Find AuditContentReqQueue[id:%lu, name:%s, Exchange:%s, RoutingKey:%s].",
                mqCfg.m_uMqId,
                mqCfg.m_strQueue.data(),
                mqCfg.m_strExchange.data(),
                mqCfg.m_strRoutingKey.data());
        }
    }

    if (vecAuditReqMqCfg.size() != m_uiSysParaAuditReqMqNum)
    {
        LogErrorP("AUDIT_REQ_MQ size[%u] != SysParaAuditReqMqNum[%u], please check config. "
            "m_vecAuditReqMqCfg.size[%u].",
            vecAuditReqMqCfg.size(),
            m_uiSysParaAuditReqMqNum,
            m_vecAuditReqMqCfg.size());

        return false;
    }

//    vecAuditReqMqCfg.resize(m_uiSysParaAuditReqMqNum);
//    m_vecAuditReqMqCfg.swap(vecAuditReqMqCfg);

    m_vecAuditReqMqCfg = vecAuditReqMqCfg;

    return true;
}

void AuditThread::initAuditOverKeyWordSearchTree(map<string, list<string> >& mapSetIn, map<string, searchTree*>& mapSetOut)
{
    for (map<string,searchTree*>::iterator iterOld = mapSetOut.begin(); iterOld != mapSetOut.end();)
    {
        if (NULL != iterOld->second)
        {
            delete iterOld->second;
        }

        mapSetOut.erase(iterOld++);
    }

    for (map<string,list<string> >::iterator iterNew = mapSetIn.begin(); iterNew != mapSetIn.end(); ++iterNew)
    {
        searchTree* pTree = new searchTree();
        if (NULL == pTree)
        {
            LogFatal("pTree is NULL.");
            return;
        }

        pTree->initTree(iterNew->second);

        mapSetOut.insert(make_pair(iterNew->first,pTree));

        LogDebug("init %s.", iterNew->first.data());

        for (list<string>::iterator iter = iterNew->second.begin(); iter != iterNew->second.end(); ++iter)
        {
            string& strData = *iter;
            LogDebug("==> %s.", strData.data());
        }
    }
}

bool AuditThread::getMqCfgByC2sId(cs_t strC2Sid, MqConfig& mqConfig)
{
    ComponentConfigMapIter iterComponentCfg;
    CHK_MAP_FIND_STR_RET_FALSE(m_mapComponentCfg, iterComponentCfg, strC2Sid);
    const ComponentConfig& componentCfg = iterComponentCfg->second;

    MqConfigMapIter iterMqCfg;
    CHK_MAP_FIND_UINT_RET_FALSE(m_mapMqCfg, iterMqCfg, componentCfg.m_uMqId);
    mqConfig = iterMqCfg->second;

    return true;
}

bool AuditThread::getMQConfig(cs_t strPhone, cs_t strSmsType, cs_t strCsid, MqConfig& mqCfg)
{
    string strMessageType;
    UInt32 uiPhoneType = m_phoneDao.getPhoneType(strPhone);
    UInt64 uSmsType = to_uint<UInt32>(strSmsType);

    if (BusType::YIDONG == uiPhoneType)
    {
        if (uSmsType == BusType::SMS_TYPE_MARKETING || uSmsType == BusType::SMS_TYPE_USSD || uSmsType == BusType::SMS_TYPE_FLUSH_SMS)
        {
            strMessageType.assign("02");
        }
        else
        {
            strMessageType.assign("01");
        }
    }
    else if (BusType::DIANXIN == uiPhoneType)
    {
        if (uSmsType == BusType::SMS_TYPE_MARKETING || uSmsType == BusType::SMS_TYPE_USSD || uSmsType == BusType::SMS_TYPE_FLUSH_SMS)
        {
            strMessageType.assign("06");
        }
        else
        {
            strMessageType.assign("05");
        }
    }
    else if ((BusType::LIANTONG == uiPhoneType) || (BusType::FOREIGN == uiPhoneType))
    {
        if (uSmsType == BusType::SMS_TYPE_MARKETING || uSmsType == BusType::SMS_TYPE_USSD || uSmsType == BusType::SMS_TYPE_FLUSH_SMS)
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
        LogError("==except== phoneType:%u is invalid.",uiPhoneType);
        return false;
    }

    string strKey = Fmt<32>("%s_%s_0", strCsid.data(),strMessageType.data());

    map<string,ComponentRefMq>::iterator itReq;
    CHK_MAP_FIND_STR_RET_FALSE(m_mapComponentRefMq, itReq, strKey);
    const ComponentRefMq& refMq = itReq->second;

    map<UInt64, MqConfig>::iterator itrMq;
    CHK_MAP_FIND_UINT_RET_FALSE(m_mapMqCfg, itrMq, refMq.m_uMqId);
    mqCfg = itrMq->second;

    if (mqCfg.m_strExchange.empty() || mqCfg.m_strRoutingKey.empty())
    {
        LogError("mqid[%ld], m_strExchange[%s], m_strRoutingKey[%s]",
            mqCfg.m_uMqId,
            mqCfg.m_strExchange.data(),
            mqCfg.m_strRoutingKey.data());
        return false;
    }

    return true;
}

UInt32 AuditThread::getPhoneArea(cs_t strPhone)
{
    string phonehead7 = strPhone.substr(0,7);
    string phonehead8 = strPhone.substr(0,8);

    models::PhoneSectionMapIter iter = m_mapPhoneArea.find(phonehead7);
    if (iter != m_mapPhoneArea.end())
    {
        return iter->second.area_id;
    }

    iter = m_mapPhoneArea.find(phonehead8);
    if(iter != m_mapPhoneArea.end())
    {
        return iter->second.area_id;
    }

    return 0;
}

void AuditThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while (true)
    {
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if (NULL == pMsg)
        {
            usleep(10000);
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }
}

void AuditThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CAdapter::InterlockedIncrement((long*)&m_iCount);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_AUDIT_REQ:
        {
            handleAuditReqMsg(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            handleRedisRspMsg(pMsg);
            break;
        }
        case MSGTYPE_MQ_GETMQMSG_REQ:
        {
            handleMqAuditResReqMsg(pMsg);
            break;
        }
        case MSGTYPE_REDISLIST_RESP:
        {
            handleRedisListRspMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            initSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            LogNotice("RuleUpdate account update!");
            TUpdateSmsAccontReq* pAccountUpdateReq= (TUpdateSmsAccontReq*)pMsg;
            m_mapAccount = pAccountUpdateReq->m_SmsAccountMap;
            break;
        }
        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqCon = (TUpdateMqConfigReq*)(pMsg);
            m_mapMqCfg.clear();

            m_mapMqCfg = pMqCon->m_mapMqConfig;
            initAuditReqMqQueue();
            LogNotice("update t_sms_mq_configure size:%d.",m_mapMqCfg.size());
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            TUpdateComponentConfigReq* pComponetConfig = (TUpdateComponentConfigReq*)(pMsg);
            m_mapComponentCfg = pComponetConfig->m_componentConfigInfoMap;
            LogNotice("update t_sms_component size:%d",m_mapComponentCfg.size());
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            TUpdateComponentRefMqReq* pMqConfig = (TUpdateComponentRefMqReq*)(pMsg);
            m_mapComponentRefMq.clear();

            m_mapComponentRefMq = pMqConfig->m_componentRefMqInfo;
            initAuditReqMqQueue();
            LogNotice("update t_sms_component_ref_mq size:%d",m_mapComponentRefMq.size());
            break;
        }
        case MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ:
        {
            TUpdateOperaterSegmentReq* pOperater = (TUpdateOperaterSegmentReq*)pMsg;
            LogNotice("RuleUpdate ClientAndSign update. size[%d]", pOperater->m_OperaterSegmentMap.size());
            m_phoneDao.m_OperaterSegmentMap = pOperater->m_OperaterSegmentMap;
            break;
        }
        case  MSGTYPE_RULELOAD_NOAUDITKEYWORD_UPDATE_REQ:
        {
            TUpdateNoAuidtKeyWordReq* msg = (TUpdateNoAuidtKeyWordReq*)pMsg;
            LogNotice("RuleUpdate NoAuditKEYWORD update. ");
            initAuditOverKeyWordSearchTree(msg->m_NoAuditKeyWordMap, m_NoAuditKeyWordMap);
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
        {
            TUpdateSystemErrorCodeReq* pHttp = (TUpdateSystemErrorCodeReq*)pMsg;
            m_mapSysErrCode = pHttp->m_mapSystemErrorCode;
            LogNotice("update t_sms_system_error_desc size:%d.",m_mapSysErrCode.size());
            break;
        }
        case  MSGTYPE_RULELOAD_IGNORE_AUDITKEYWORD_UPDATE_REQ:
        {
            TUpdateAuidtKeyWordReq* msg = (TUpdateAuidtKeyWordReq*)pMsg;
            LogNotice("RuleUpdate IgnoreAuditKEYWORD update. size[%d]",msg->m_IgnoreAuditKeyWordMap.size());
            m_keyWordCheck.initIgnoreAuditKeyWord(msg->m_IgnoreAuditKeyWordMap);
            break;
        }
        case  MSGTYPE_RULELOAD_AUDITKEYWORD_UPDATE_REQ:
        {
            TUpdateAuidtKeyWordReq* msg = (TUpdateAuidtKeyWordReq*)pMsg;
            LogNotice("RuleUpdate AuditKEYWORD update. ");
            m_keyWordCheck.initAuditKeyWordSearchTree(msg->m_AuditKeyWordMap);
            break;
        }
        case  MSGTYPE_RULELOAD_AUDIT_CGROUPREFCLIENT_UPDATE_REQ:
        {
            TUpdateAuidtKeyWordReq* msg = (TUpdateAuidtKeyWordReq*)pMsg;
            LogNotice("RuleUpdate AuditCGroupRefClient update. size[%d]",msg->m_AuditCGroupRefClientMap.size());
            m_keyWordCheck.m_mapAuditCGroupRefClient = msg->m_AuditCGroupRefClientMap;
            break;
        }
        case  MSGTYPE_RULELOAD_AUDIT_CLIENTGROUP_UPDATE_REQ:
        {
            TUpdateAuidtKeyWordReq* msg = (TUpdateAuidtKeyWordReq*)pMsg;
            LogNotice("RuleUpdate AuditClientGroup update. size[%d]",msg->m_AuditClientGroupMap.size());
            m_keyWordCheck.m_mapAuditClientGroup = msg->m_AuditClientGroupMap;
            m_keyWordCheck.m_uDefaultGroupId = msg->m_uDefaultGroupId;
            break;
        }
        case  MSGTYPE_RULELOAD_AUDIT_KGROUPREFCATEGORY_UPDATE_REQ:
        {
            TUpdateAuidtKeyWordReq* msg = (TUpdateAuidtKeyWordReq*)pMsg;
            LogNotice("RuleUpdate AuditKGroupRefCategory update. size[%d]",msg->m_AuditKgroupRefCategoryMap.size());
            m_keyWordCheck.m_mapAuditKgroupRefCategory = msg->m_AuditKgroupRefCategoryMap;
            break;
        }
        case MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ:
        {
            TUpdatePhoneAreaReq* pReq = (TUpdatePhoneAreaReq*)pMsg;
            m_mapPhoneArea = pReq->m_PhoneAreaMap;
            LogNotice("RuleUpdate PhoneArea update. map.size[%d]", pReq->m_PhoneAreaMap.size());
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            handleTimeoutMsg(pMsg);
            break;
        }
        default:
        {
            LogError("Invalid MsgType[%x].", pMsg->m_iMsgType);
            break;
        }
    }
}

void AuditThread::sendAuditReqMsg2Audit(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    MqConfig mqConfig;
    if (!pickOneAuditReqMqQueue(pSession, mqConfig))
    {
        LogError("[%s:%s:%s] Call pickOneAuditReqMqQueue failed.",
            pSession->m_auditReqMsg.m_strClientId.data(),
            pSession->m_auditReqMsg.m_strSmsId.data(),
            pSession->m_auditReqMsg.m_strPhone.data());
        return;
    }

    pSession->packMsg2Audit();

    TMQPublishReqMsg* pReq = new TMQPublishReqMsg();
    CHK_NULL_RETURN(pReq);
    pReq->m_strExchange = mqConfig.m_strExchange;
    pReq->m_strRoutingKey = mqConfig.m_strRoutingKey;
    pReq->m_strData = pSession->m_strSendMsg;
    pReq->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;

    CHK_NULL_RETURN(g_pMQDBAuditProducerThread);
    g_pMQDBAuditProducerThread->PostMsg(pReq);

	LogDebug("Exchange[%s],RoutingKey[%s],data[%s].",
            mqConfig.m_strExchange.data(),
            mqConfig.m_strRoutingKey.data(),
            pSession->m_strSendMsg.data());

}

bool AuditThread::pickOneAuditReqMqQueue(Session* pSession, MqConfig& mqConfig)
{
    CHK_NULL_RETURN_FALSE(pSession);

    string str = "";
    str.append(pSession->m_auditReqMsg.m_strClientId);
    str.append(pSession->m_auditReqMsg.m_strSign);
    str.append(pSession->m_auditReqMsg.m_strContent);

    UInt32 uiIndex = md5Hash(str, m_vecAuditReqMqCfg.size());

    mqConfig = m_vecAuditReqMqCfg[uiIndex];

    return true;
}

void AuditThread::handleTimeoutMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    if (TIMER_ID_GET_CACHE_DETAIL == pMsg->m_iSeq)
    {
        SAFE_DELETE(m_pTimerGetCacheDetail);

        string strCurHourMins = Comm::getCurrHourMins();

        LogDebug("cmd[%s], strCurHourMins[%s], "
            "m_strSysParaAuditTimeAreaBegin[%s], m_strSysParaAuditTimeAreaEnd[%s].",
            pMsg->m_strSessionID.data(),
            strCurHourMins.data(),
            m_strSysParaAuditTimeAreaBegin.data(),
            m_strSysParaAuditTimeAreaEnd.data());

        if ((strCurHourMins < m_strSysParaAuditTimeAreaBegin) || (strCurHourMins > m_strSysParaAuditTimeAreaEnd))
        {
            m_pTimerGetCacheDetail = SetTimer(TIMER_ID_GET_CACHE_DETAIL, pMsg->m_strSessionID, m_iAuditTimeOut);
        }
        else
        {
            redisGetList(pMsg->m_strSessionID, "cache_detail", this, 0, m_uiSysParaAuditOutNumber);
        }
    }
}

bool AuditThread::TimeRegCheck(string strTime)
{
    if(strTime.empty())
    {
        return false;
    }

    string strKeyWord;
    cmatch what;
    if(regex_search(strTime.c_str(), what, m_regTime))
    {
        if(what.size() > 0)
        {
            strKeyWord = what[0].str();
        }
        LogDebug("time reg match suc , keyword[%s]!", strKeyWord.data());
        return true;
    }
    else
    {
        LogDebug("time reg match fail");
        return false;
    }
}

bool AuditThread::praseJsonAuditDetail(cs_t strData, Session* pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    std::string js;

    if (json_format_string(strData, js) < 0)
    {
        LogError("Call json_format_string failed. strData[%s].", strData.data());
        return false;
    }

    if(!reader.parse(js, root))
    {
        LogError("Call parse failed. js[%s].", js.data());
        return false;
    }

    pSession->m_auditReqMsg.m_strClientId       = root["clientid"].asString();
    pSession->m_auditReqMsg.m_strUserName       = root["username"].asString();
    pSession->m_auditReqMsg.m_strSmsId          = root["smsid"].asString();
    pSession->m_auditReqMsg.m_strPhone          = root["phone"].asString();
    pSession->m_auditReqMsg.m_strSign           = root["sign"].asString();
    pSession->m_auditReqMsg.m_strContent        = root["content"].asString();
    pSession->m_auditReqMsg.m_strSmsType        = root["smstype"].asString();
    pSession->m_auditReqMsg.m_uiSmsFrom         = root["smsfrom"].asUInt();
    pSession->m_auditReqMsg.m_uiPayType         = root["paytype"].asUInt();
    pSession->m_auditReqMsg.m_uiShowSignType    = root["showsigntype"].asUInt();
    pSession->m_auditReqMsg.m_strSignPort       = root["signextendport"].asString();
    pSession->m_auditReqMsg.m_strExtpendPort    = root["userextpendport"].asString();
    pSession->m_auditReqMsg.m_strIds            = root["ids"].asString();
    pSession->m_auditReqMsg.m_uiSmsNum          = root["clientcnt"].asUInt();
    pSession->m_auditReqMsg.m_strCSid           = root["csid"].asString();
    pSession->m_auditReqMsg.m_strC2sDate        = root["csdate"].asString();
    pSession->m_auditReqMsg.m_iClientSendLimitCtrlTypeFlag = root["send_limit_flag"].asInt();

    m_chineseConvert.ChineseTraditional2Simple(pSession->m_auditReqMsg.m_strContent, pSession->m_auditReqMsg.m_strContentSimple);
    m_chineseConvert.ChineseTraditional2Simple(pSession->m_auditReqMsg.m_strSign, pSession->m_auditReqMsg.m_strSignSimple);

    if (pSession->m_strAuditContent.empty())
    {
        // only for reids:cache_detail
        pSession->m_strAuditContent   = root["auditcontent"].asString();
        pSession->m_strAuditId        = root["audit_id"].asString();
        pSession->m_strAuditDate      = root["audit_date"].asString();
        pSession->m_uiAuditState      = root["audit_state"].asUInt();
    }

    return true;
}

void AuditThread::handleAuditReqMsg(TMsg* pMsg)
{
    AuditReqMsg* pReq = (AuditReqMsg*)pMsg;
    CHK_NULL_RETURN(pReq);

    Branchflow res = checkBranchflow(pReq);

    LogNotice("Get Branchflow[%d].", res);

    switch ( res )
    {
    case Audit:
        processAudit(pReq);
        break;
    case GroupSendLimit:
        processGroupSendLimit(pReq);
        break;
    case ContinueToSend:
        processContinueToSend(pReq);
        break;
    default:
        LogError("Invalid Branchflow[%d].", res);
        break;
    }
}

AuditThread::Branchflow AuditThread::checkBranchflow(AuditReqMsg* pReq)
{
    CHK_NULL_RETURN_CODE(pReq, End);

    SmsAccountMap::iterator iter;
    CHK_MAP_FIND_STR_RET_CODE(m_mapAccount, iter, pReq->m_strClientId, End);
    const SmsAccount& smsAccount = iter->second;

    LogDebug("m_eMatchSmartTemplateRes[%d], m_uiSysParaGroupSendLimCfg[%u], smsAccount.m_uGroupsendLimUserflag[%u].",
        pReq->m_eMatchSmartTemplateRes,
        m_uiSysParaGroupSendLimCfg,
        smsAccount.m_uGroupsendLimUserflag);

    if (NoMatch == pReq->m_eMatchSmartTemplateRes)
    {
        // 对于生产系统的用户，若没有匹配到白模板
        // 则需要判断是否满足走群发限制流程
        // 如果满足，则走群发限制流程，（群发限制流程解除后，还需要继续走审核流程）
        // 如果不满足，则直接走审核流程

        if ((GROUPSENDLIM_OFF == m_uiSysParaGroupSendLimCfg)
        || (!matchGroupsendLimitCfgMode(pReq, smsAccount))
        || (GROUPSENDLIM_USERFLAG_SPECIFIED == smsAccount.m_uGroupsendLimUserflag))
        {
            return Audit;
        }

        return GroupSendLimit;
    }
    else if ((MatchConstant == pReq->m_eMatchSmartTemplateRes)
         || (MatchVariable_NoMatchRegex == pReq->m_eMatchSmartTemplateRes))
    {
        if ((GROUPSENDLIM_OFF == m_uiSysParaGroupSendLimCfg)
        || (!matchGroupsendLimitCfgMode(pReq, smsAccount))
        || (GROUPSENDLIM_USERFLAG_PRODUCE == smsAccount.m_uGroupsendLimUserflag))
        {
            return ContinueToSend;
        }

        return GroupSendLimit;
    }
    else if (MatchVariable_MatchRegex == pReq->m_eMatchSmartTemplateRes)
    {
        return Audit;
    }
    else
    {
        return ContinueToSend;
    }

    LogWarn("MatchSmartTemplateRes[%d], SysParaGroupSendLimCfg[%u], smsAccount.GroupsendLimUserflag[%u].",
        pReq->m_eMatchSmartTemplateRes,
        m_uiSysParaGroupSendLimCfg,
        smsAccount.m_uGroupsendLimUserflag);

    return End;
}

bool AuditThread::matchGroupsendLimitCfgMode(AuditReqMsg* pReq, const SmsAccount& smsAccount)
{
    CHK_NULL_RETURN_FALSE(pReq);

    bool ret = false;
    std::string strTip = "matched!";

    do
    {
        UInt32 uTmp = 0;

        UInt32 uSmsType = atoi(pReq->m_strSmsType.c_str());
        switch (uSmsType)
        {
            case BusType::SMS_TYPE_NOTICE:
                uTmp = (1 << GROUPSENDLIM_CFG_SMS_TZ_IDX);
                break;
            case BusType::SMS_TYPE_VERIFICATION_CODE:
                uTmp = (1 << GROUPSENDLIM_CFG_SMS_YZM_IDX);
                break;
            case BusType::SMS_TYPE_MARKETING:
                uTmp = (1 << GROUPSENDLIM_CFG_SMS_YX_IDX);
                break;
        }

        if ((smsAccount.m_uGroupsendLimCfg & uTmp) == 0)
        {
            strTip = "smstype not matched!";
            break;
        }

        switch (pReq->m_uiOperater)
        {
            case BusType::YIDONG:
                uTmp = (1 << GROUPSENDLIM_CFG_OPER_YD_IDX);
                break;
            case BusType::LIANTONG:
                uTmp = (1 << GROUPSENDLIM_CFG_OPER_LT_IDX);
                break;
            case BusType::DIANXIN:
                uTmp = (1 << GROUPSENDLIM_CFG_OPER_DX_IDX);
                break;
        }

        if ((smsAccount.m_uGroupsendLimCfg & uTmp) == 0)
        {
            strTip = "operater not matched!";
            break;
        }

        ret = true;
    } while(0);

    LogNotice("[%s:%s:%s] SmsType:%s, Operater:%u, GroupsendLimitCfg[%#x] => [%s]",
        pReq->m_strClientId.data(),
        pReq->m_strSmsId.data(),
        pReq->m_strPhone.data(),
        pReq->m_strSmsType.data(),
        pReq->m_uiOperater,
        smsAccount.m_uGroupsendLimCfg,
        strTip.data());

    return ret;
}

void AuditThread::processAudit(AuditReqMsg* pReq, Session* pSession)
{
    CHK_NULL_RETURN(pReq);

    if (MatchVariable_MatchRegex == pReq->m_eMatchSmartTemplateRes)
    {
        // 智能模板匹配时,如果匹配到变量模板
        // 且变量部分又匹配到正则,则一定要走审核流程
        processAuditReal(pReq);
    }
    else
    {
        // 检查用户审核属性和审核关键字
        // 返回1，代表真的需要走审核流程了
        if (1 == checkAuditAttributeAndKeyword(pReq))
        {
            processAuditReal(pReq);
        }
        else
        {
            // 群发限制解除转审核场景
            if (NULL != pSession)
            {
                sendMsg2MqC2sIo(pSession);

                // 把审核状态、审核时间置空
                pSession->m_uiAuditState = INVALID_UINT32;
                pSession->m_strAuditDate = "NULL";

                updateDbAuditState(pSession);
            }
            else
            {
                processContinueToSend(pReq);
            }
        }
    }
}

void AuditThread::processAuditReal(AuditReqMsg* pReq)
{
    CHK_NULL_RETURN(pReq);

    Session* pSession = new Session(this);
    pSession->setAuditReqMsg(*pReq);
    pSession->getAuditContent();

    redisGet("HGETALL endauditsms:" + pSession->m_strAuditContent,
             "endauditsms",
             this,
             pSession->m_ulSequence,
             "check endauditsms");
}

void AuditThread::processGroupSendLimit(AuditReqMsg* pReq)
{
    CHK_NULL_RETURN(pReq);

    Session* pSession = new Session(this);
    pSession->setAuditReqMsg(*pReq);
    pSession->getGroupSendLimitAuditContent();

    redisGet("HGETALL endgroupsendauditsms:" + pSession->m_strAuditContent,
             "endgroupsendauditsms",
             this,
             pSession->m_ulSequence,
             "check endgroupsendauditsms");
}

void AuditThread::processContinueToSend(AuditReqMsg* pReq)
{
    CHK_NULL_RETURN(pReq);
    LogDebug("no need to process, continue to send.");

    AuditRspMsg* pRsp = new AuditRspMsg();
    CHK_NULL_RETURN(pRsp);

    pRsp->m_iSeq = pReq->m_iSeq;
    pRsp->m_iMsgType = MSGTYPE_AUDIT_RSP;
    g_pSessionThread->PostMsg(pRsp);
}

int AuditThread::checkAuditAttributeAndKeyword(AuditReqMsg* pReq)
{
    CHK_NULL_RETURN_CODE(pReq, -1);

    SmsAccountMapIter iter;
    CHK_MAP_FIND_STR_RET_CODE(m_mapAccount, iter, pReq->m_strClientId, -1);
    const SmsAccount& smsAccount = iter->second;

    LogDebug("smsAccount.m_uNeedaudit[%u].", smsAccount.m_uNeedaudit);

    int bNeedAudit = 0;

    switch (smsAccount.m_uNeedaudit)
    {
        case BusType::SMS_NO_NEED_AUDIT:
        {
            break;
        }
        case BusType::SMS_YINGXIAO_NEED_AUDIT:
        {
            if (pReq->m_strSmsType == Comm::int2str(BusType::SMS_TYPE_MARKETING))
            {
                bNeedAudit = 1;
            }
            break;
        }
        case BusType::SMS_ALL_NEED_AUDIT:
        {
            bNeedAudit = 1;
            break;
        }
        case BusType::SMS_KEYWORD_AUDIT:
        {
            string tmpSign = pReq->m_strSign;
            string tmpContent = pReq->m_strContent;
            string strIgnoreMapKey = pReq->m_strClientId + "_" + pReq->m_strSmsType;

            if (m_iAuditKeywordCovRegular & 0x01)
            {
                tmpSign = pReq->m_strSignSimple;
                tmpContent = pReq->m_strContentSimple;
            }

            string strAuditKeyword;
            if (m_keyWordCheck.AuditKeywordCheck(pReq->m_strClientId,
                                                tmpContent,
                                                tmpSign,
                                                strAuditKeyword,
                                                strIgnoreMapKey))
            {
                LogNotice("[%s:%s:%s] get audit keyword:%s.",
                    pReq->m_strClientId.data(),
                    pReq->m_strSmsId.data(),
                    pReq->m_strPhone.data(),
                    strAuditKeyword.data());

                bNeedAudit = 1;
            }

            break;
        }
        default:
        {
            LogError("[%s:%s:%s] Invalid smsAccount.Needaudit[%u].",
                pReq->m_strClientId.data(),
                pReq->m_strSmsId.data(),
                pReq->m_strPhone.data(),
                smsAccount.m_uNeedaudit);

            break;
        }
    }

    // 如果需要审核 检查是否开启免审
    if (bNeedAudit)
    {
        // 免审关键字检查
        string strFreeAuditKeyword;
        if (checkfreeAuditKeyword(pReq, strFreeAuditKeyword))
        {
            LogNotice("[%s:%s:%s] has free audit keyword[%s], so no need to audit.",
                pReq->m_strClientId.data(),
                pReq->m_strSmsId.data(),
                pReq->m_strPhone.data(),
                strFreeAuditKeyword.data());

            bNeedAudit = 0;
        }
    }

    return bNeedAudit;
}

bool AuditThread::checkfreeAuditKeyword(const AuditReqMsg* pReq, string& strOut)
{
    CHK_NULL_RETURN_FALSE(pReq);

    string strContentTemp = pReq->m_strContent;
    string strSignData = pReq->m_strSign;


    // 检测系统参数是否需要开启简繁转换
    if ( m_iAuditKeywordCovRegular & 0x01 )
    {
        strSignData = pReq->m_strSignSimple;
        strContentTemp = pReq->m_strContentSimple;
    }

    if (pReq->m_bIncludeChinese)
    {
        static const string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
        static const string strRight = http::UrlCode::UrlDecode("%e3%80%91");

        strSignData = strLeft + strSignData + strRight;
    }
    else
    {
        strSignData = "[" + strSignData + "]";
    }

    Comm::ToLower(strContentTemp);
    Comm::ToLower(strSignData);

    LogDebug("strContentTemp[%s], strSignData[%s].",
            strContentTemp.data(),
            strSignData.data());

    map<string,searchTree*>::iterator itrPlatform = m_NoAuditKeyWordMap.find("*");
    if (m_NoAuditKeyWordMap.end() != itrPlatform)
    {
        LogDebug("Find *.");

        searchTree* pTree = itrPlatform->second;

        if (pTree->searchSign(strSignData,strOut))
            return true;

        if (pTree->search(strContentTemp,strOut))
            return true;
    }

    map<string,searchTree*>::iterator itrClient = m_NoAuditKeyWordMap.find(pReq->m_strClientId);
    if (m_NoAuditKeyWordMap.end() != itrClient)
    {
        LogDebug("Find %s.", pReq->m_strClientId.data());

        searchTree* pTree = itrClient->second;

        if (pTree->searchSign(strSignData,strOut))
            return true;

        if (pTree->search(strContentTemp,strOut))
            return true;
    }

    LogDebug("not find.");

    return false;
}

void AuditThread::handleRedisRspMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TRedisResp* pRedisRsp = (TRedisResp*)pMsg;

    if ("check endauditsms" == pMsg->m_strSessionID)
    {
        processCheckEndAuditSms(pRedisRsp);
    }
    else if ("check endgroupsendauditsms" == pMsg->m_strSessionID)
    {
        processCheckGroupSendEndAuditSms(pRedisRsp);
    }

    freeReply(pRedisRsp->m_RedisReply);
}

void AuditThread::processCheckEndAuditSms(TRedisResp* pRedisRsp)
{
    CHK_NULL_RETURN(pRedisRsp);

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pRedisRsp->m_iSeq);
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    //////////////////////////////////////////////////////////////////

    int status = AUTO_AUDIT_STATUS_INVALID;
    redisReply* pRedisReply = pRedisRsp->m_RedisReply;

    if ((NULL != pRedisReply)
     && (REDIS_REPLY_ARRAY == pRedisReply->type)
     && (0 != pRedisReply->elements))
    {
        string strTemp;
        string strAuditId;

        paserReply("status", pRedisReply, strTemp);
        paserReply("auditid", pRedisReply, pSession->m_strAuditId);

        status = atoi(strTemp.data());
    }
    else
    {
        LogDebug("[%s:%s:%s] check audit query redis failed or no content.",
            pSession->m_auditReqMsg.m_strClientId.data(),
            pSession->m_auditReqMsg.m_strSmsId.data(),
            pSession->m_auditReqMsg.m_strPhone.data());
    }

    LogDebug("[%s:%s:%s] state[%d].",
        pSession->m_auditReqMsg.m_strClientId.data(),
        pSession->m_auditReqMsg.m_strSmsId.data(),
        pSession->m_auditReqMsg.m_strPhone.data(),
        status);

    pSession->m_strAuditDate = Comm::getCurrentTime();

    if (AUTO_AUDIT_STATUS_PASS == status)
    {
        // 自动审核通过,由Session线程更新审核相关字段
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_AUTO_PASS;
        checkMarketingTimeArea(pSession);
        sendAuditRspMsg(pSession);
    }
    else if (AUTO_AUDIT_STATUS_FAIL == status)
    {
        // 自动审核不通过,由Session线程更新审核相关字段
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_AUTO_FAIL;
        pSession->m_strErrCode = AUDIT_NOT_PAAS;
        sendAuditRspMsg(pSession);
    }
    else
    {
        // 无自动审核结果，转人工审核
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_WAIT;
        sendAuditRspMsg(pSession);
        setAuditDetail("audit_detail:" + pSession->m_strAuditContent, pSession);
        sendAuditReqMsg2Audit(pSession);
        updateDbAuditState(pSession);
    }

    SAFE_DELETE(pSession);
}

void AuditThread::processCheckGroupSendEndAuditSms(TRedisResp* pRedisRsp)
{
    CHK_NULL_RETURN(pRedisRsp);

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pRedisRsp->m_iSeq);
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    //////////////////////////////////////////////////////////////////

    int status = AUTO_AUDIT_STATUS_INVALID;
    redisReply* pRedisReply = pRedisRsp->m_RedisReply;

    if ((NULL != pRedisReply)
     && (REDIS_REPLY_ARRAY == pRedisReply->type)
     && (0 != pRedisReply->elements))
    {
        string strTemp;
        string strAuditId;

        paserReply("status", pRedisReply, strTemp);
        paserReply("auditid", pRedisReply, pSession->m_strAuditId);

        status = atoi(strTemp.data());
    }
    else
    {
        LogWarn("[%:%s:%s] check groupsendlim audit query redis failed or no content.",
            pSession->m_auditReqMsg.m_strClientId.data(),
            pSession->m_auditReqMsg.m_strSmsId.data(),
            pSession->m_auditReqMsg.m_strPhone.data());
    }

    LogDebug("[%s:%s:%s] state[%d].",
        pSession->m_auditReqMsg.m_strClientId.data(),
        pSession->m_auditReqMsg.m_strSmsId.data(),
        pSession->m_auditReqMsg.m_strPhone.data(),
        status);

    pSession->m_strAuditDate = Comm::getCurrentTime();

    if (AUTO_AUDIT_STATUS_PASS == status)
    {
        // 自动审核通过
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_AUTO_PASS;
        checkMarketingTimeArea(pSession);
        sendAuditRspMsg(pSession);
    }
    else if (AUTO_AUDIT_STATUS_FAIL == status)
    {
        // 自动审核不通过
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_AUTO_FAIL;
        pSession->m_strErrCode = AUDIT_NOT_PAAS;
        sendAuditRspMsg(pSession);
    }
    else if (AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_SEND == status)
    {
        // 群发限制已解除转发送
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_AUTO_PASS;
        checkMarketingTimeArea(pSession);
        sendAuditRspMsg(pSession);
    }
    else if (AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_AUDIT == status)
    {
        // 群发限制已解除转审核
        processAudit(&pSession->m_auditReqMsg);
    }
    else
    {
        // 无结果，说明当天还没进行过群发限制，则转群发限制流程
        pSession->m_uiAuditState = SMS_AUDIT_STATUS_GROUPSENDLIMIT;
        setAuditDetail("audit_detail:" + pSession->m_strAuditContent, pSession);
        sendAuditRspMsg(pSession);
        sendAuditReqMsg2Audit(pSession);
        updateDbAuditState(pSession);
    }

    SAFE_DELETE(pSession);
}

bool AuditThread::checkMarketingTimeArea(Session* pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    string strCurHourMins = Comm::getCurrHourMins();
    if ((strCurHourMins < m_strSysParaAuditTimeAreaBegin) || (strCurHourMins > m_strSysParaAuditTimeAreaEnd))
    {
        LogWarn("[%s:%s:%s] BeginTime:%s, CurrentTime:%s, EndTime:%s, put sms into cache detail.",
            pSession->m_auditReqMsg.m_strClientId.data(),
            pSession->m_auditReqMsg.m_strSmsId.data(),
            pSession->m_auditReqMsg.m_strPhone.data(),
            m_strSysParaAuditTimeAreaBegin.data(),
            strCurHourMins.data(),
            m_strSysParaAuditTimeAreaEnd.data());

        setAuditDetail("cache_detail:" + to_string(g_uComponentId), pSession);

        pSession->m_uiAuditState = SMS_AUDIT_STATUS_CACHE;
        updateDbAuditState(pSession);

        return true;
    }

    return false;
}

void AuditThread::setAuditDetail(cs_t strRedisCmdKey, Session* pSession)
{
    // audit_detail or cache_detail

    Json::Value jsonData;
    UInt32 uExpireTime = REDIS_EXPIRE_TIME_54H;

    if (string::npos != strRedisCmdKey.find("cache_detail:"))
    {
        jsonData["audit_id"]        = Json::Value(pSession->m_strAuditId);
        jsonData["auditcontent"]    = Json::Value(pSession->m_strAuditContent);
        jsonData["audit_state"]     = Json::Value(pSession->m_uiAuditState);
        jsonData["audit_date"]      = Json::Value(pSession->m_strAuditDate);
        uExpireTime = AUDIT_REIDS_EXPIRE_TIME;
    }

    jsonData["clientid"]        = Json::Value(pSession->m_auditReqMsg.m_strClientId);
    jsonData["username"]        = Json::Value(pSession->m_auditReqMsg.m_strUserName);
    jsonData["smsid"]           = Json::Value(pSession->m_auditReqMsg.m_strSmsId);
    jsonData["phone"]           = Json::Value(pSession->m_auditReqMsg.m_strPhone);
    jsonData["sign"]            = Json::Value(pSession->m_auditReqMsg.m_strSign);
    jsonData["content"]         = Json::Value(pSession->m_auditReqMsg.m_strContent);
    jsonData["smstype"]         = Json::Value(pSession->m_auditReqMsg.m_strSmsType);
    jsonData["smsfrom"]         = Json::Value(pSession->m_auditReqMsg.m_uiSmsFrom);
    jsonData["paytype"]         = Json::Value(pSession->m_auditReqMsg.m_uiPayType);
    jsonData["showsigntype"]    = Json::Value(pSession->m_auditReqMsg.m_uiShowSignType);
    jsonData["signextendport"]  = Json::Value(pSession->m_auditReqMsg.m_strSignPort);
    jsonData["userextpendport"] = Json::Value(pSession->m_auditReqMsg.m_strExtpendPort);
    jsonData["ids"]             = Json::Value(pSession->m_auditReqMsg.m_strIds);
    jsonData["clientcnt"]       = Json::Value(pSession->m_auditReqMsg.m_uiSmsNum);
    jsonData["csid"]            = Json::Value(pSession->m_auditReqMsg.m_strCSid);
    jsonData["csdate"]          = Json::Value(pSession->m_auditReqMsg.m_strC2sDate);
    jsonData["send_limit_flag"] = Json::Value(pSession->m_auditReqMsg.m_iClientSendLimitCtrlTypeFlag);

    Json::FastWriter fast_writer;
    string strDetail = Base64::Encode(fast_writer.write(jsonData));

    redisSet("lpush " + strRedisCmdKey + " " + strDetail, strRedisCmdKey);
    redisSetTTL(strRedisCmdKey, uExpireTime, strRedisCmdKey);
}

void AuditThread::sendAuditRspMsg(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    AuditRspMsg* pRsp = new AuditRspMsg();
    CHK_NULL_RETURN(pRsp);

    pRsp->m_iSeq = pSession->m_auditReqMsg.m_iSeq;
    pRsp->m_strAuditContent = pSession->m_strAuditContent;
    pRsp->m_strAuditId = pSession->m_strAuditId;
    pRsp->m_strAuditDate = pSession->m_strAuditDate;
    pRsp->m_uiAuditState = pSession->m_uiAuditState;
    pRsp->m_strErrCode = pSession->m_strErrCode;
    pRsp->m_iMsgType = MSGTYPE_AUDIT_RSP;
    g_pSessionThread->PostMsg(pRsp);

    LogDebug("==send rsp to sessionthread== m_iSeq[%lu],m_strAuditContent[%s],m_strAuditId[%s],"
        "m_strAuditDate[%s],m_uiAuditState[%u], m_strErrCode[%s].",
        pSession->m_auditReqMsg.m_iSeq,
        pSession->m_strAuditContent.data(),
        pSession->m_strAuditId.data(),
        pSession->m_strAuditDate.data(),
        pSession->m_uiAuditState,
        pSession->m_strErrCode.data());
}

void AuditThread::handleMqAuditResReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    Session* pSession = new Session(this);
    CHK_NULL_RETURN(pSession);
    pSession->praseAuditResMsg(pMsg->m_strSessionID);

    LogDebug("Sequence[%lu], AuditResult[%u].",
        pSession->m_ulSequence,
        pSession->m_uiAuditResult);

    switch (pSession->m_uiAuditResult)
    {
        case MAN_AUDIT_STATUS_PASS:
        case MAN_AUDIT_STATUS_FAIL:
        {
            processAuditResultPassOrFailed(pSession);
            break;
        }
        case MAN_AUDIT_STATUS_PASS_EXPIRE:
        case MAN_AUDIT_STATUS_FAIL_EXPIRE:
        case SYS_AUDIT_STATUS_PASS_EXPIRE:
        case SYS_AUDIT_STATUS_FAIL_EXPIRE:
        case SYS_AUDIT_STATUS_WAIT_EXPIRE:
        {
            processAuditResultExpire(pSession);
            break;
        }
        case SYS_AUDIT_STATUS_GROUPSEND_UNLIM_SEND:
        case SYS_AUDIT_STATUS_GROUPSEND_UNLIM_AUDIT:
        {
            processAuditResultGroupSendUnLimit(pSession);
            break;
        }
        default:
        {
            LogError("Invalid AuditState[%u].", pSession->m_uiAuditResult);
            break;
        }
    }
}

void AuditThread::processAuditResultPassOrFailed(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    string strRedisDbKey;
    UInt32 uiTimeout = 0;

    if (NORMAL_AUDIT_RESULT == pSession->m_uiGroupSendLimitFlag)
    {
        strRedisDbKey = "endauditsms";
        uiTimeout = AUDIT_REIDS_EXPIRE_TIME;
    }
    else if (GROUPSEND_AUDIT_RESULT == pSession->m_uiGroupSendLimitFlag)
    {
        strRedisDbKey = "endgroupsendauditsms";
        uiTimeout = REDIS_EXPIRE_TIME_24H;
    }

    string strRedisCmd;
    strRedisCmd.append("HMSET ");
    strRedisCmd.append(strRedisDbKey);
    strRedisCmd.append(":");
    strRedisCmd.append(pSession->m_strAuditContent);
    strRedisCmd.append(" auditid ");
    strRedisCmd.append(pSession->m_strAuditId);
    strRedisCmd.append(" status ");
    strRedisCmd.append(to_string(pSession->m_uiAuditResult));

    redisSet(strRedisCmd, strRedisDbKey);
    redisSetTTL(strRedisDbKey + ":" + pSession->m_strAuditContent, uiTimeout, strRedisDbKey);
    redisGetList("rpop audit_detail:" + pSession->m_strAuditContent,
        "audit_detail",
        this,
        pSession->m_ulSequence,
        m_uiSysParaAuditOutNumber);
}

void AuditThread::processAuditResultExpire(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    if (NORMAL_AUDIT_RESULT == pSession->m_uiGroupSendLimitFlag)
    {
        redisDel("needauditsms:" + pSession->m_strAuditContent, "needauditsms");
        redisDel("endauditsms:" + pSession->m_strAuditContent, "endauditsms");
    }
    else if (GROUPSEND_AUDIT_RESULT == pSession->m_uiGroupSendLimitFlag)
    {
        redisDel("needgroupsendauditsms:" + pSession->m_strAuditContent, "needgroupsendauditsms");

        if (SYS_AUDIT_STATUS_WAIT_EXPIRE != pSession->m_uiAuditResult)
        {
            UInt32 uiStatus = (GROUPSENDLIM_USERFLAG_PRODUCE == pSession->m_uiGroupsendLimUserFlag)
                            ? AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_AUDIT
                            : AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_SEND;

            string strRedisCmd;
            strRedisCmd.append("HMSET endgroupsendauditsms:");
            strRedisCmd.append(pSession->m_strAuditContent);
            strRedisCmd.append(" auditid ");
            strRedisCmd.append(pSession->m_strAuditId);
            strRedisCmd.append(" status ");
            strRedisCmd.append(to_string(uiStatus));

            redisSet(strRedisCmd, "endgroupsendauditsms");
            redisSetTTL("endgroupsendauditsms:" + pSession->m_strAuditContent,
                        REDIS_EXPIRE_TIME_24H,
                        "endgroupsendauditsms");
        }
    }

    // 获取明细数据进行处理
    redisGetList("rpop audit_detail:" + pSession->m_strAuditContent,
                 "audit_detail",
                 this,
                 pSession->m_ulSequence,
                 m_uiSysParaAuditOutNumber);
}

void AuditThread::processAuditResultGroupSendUnLimit(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    UInt32 uiStatus = (SYS_AUDIT_STATUS_GROUPSEND_UNLIM_AUDIT == pSession->m_uiAuditResult)
                    ? AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_AUDIT
                    : AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_SEND;

    string strRedisCmd;
    strRedisCmd.append("HMSET endgroupsendauditsms:");
    strRedisCmd.append(pSession->m_strAuditContent);
    strRedisCmd.append(" auditid ");
    strRedisCmd.append(pSession->m_strAuditId);
    strRedisCmd.append(" status ");
    strRedisCmd.append(to_string(uiStatus));

    redisSet(strRedisCmd, "endgroupsendauditsms");
    redisSetTTL("endgroupsendauditsms:" + pSession->m_strAuditContent,
                REDIS_EXPIRE_TIME_24H,
                "endgroupsendauditsms");
    redisGetList("rpop audit_detail:" + pSession->m_strAuditContent,
                 "audit_detail",
                 this,
                 pSession->m_ulSequence,
                 m_uiSysParaAuditOutNumber);
}

void AuditThread::handleRedisListRspMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TRedisListResp* pRsp = (TRedisListResp*)(pMsg);

    if ("audit_detail" == pRsp->m_strKey)
    {
        processGetAuditDetailRspMsg(pMsg);
    }
    else if ("cache_detail" == pRsp->m_strKey)
    {
        processGetCacheDetailRspMsg(pRsp);
    }
}

void AuditThread::processGetAuditDetailRspMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TRedisListResp* pResp = (TRedisListResp*)(pMsg);

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    if (0 != pResp->m_iResult)
    {
        LogNotice("Sequence[%lu], Auditid[%s],Auditcontent[%s],Auditstatus[%u],AuditResult[%u] "
            "no more audit_detail data.",
            pSession->m_ulSequence,
            pSession->m_strAuditId.data(),
            pSession->m_strAuditContent.data(),
            pSession->m_uiAuditState,
            pSession->m_uiAuditResult);

        SAFE_DELETE(pSession);
        return;
    }

    LogNotice("Sequence[%lu], resp.size[%d],maxsize[%d],iReturn[%d],cmd[%s]",
        pSession->m_ulSequence,
        pResp->m_pRespList->size(),
        pResp->m_uMaxSize,
        pResp->m_iResult,
        pResp->m_RedisCmd.data());

    switch (pSession->m_uiAuditResult)
    {
        case MAN_AUDIT_STATUS_PASS:
        case MAN_AUDIT_STATUS_PASS_EXPIRE:
        case SYS_AUDIT_STATUS_PASS_EXPIRE:
        {
            pSession->m_uiAuditState = SMS_AUDIT_STATUS_PASS;
            break;
        }
        case MAN_AUDIT_STATUS_FAIL:
        case MAN_AUDIT_STATUS_FAIL_EXPIRE:
        case SYS_AUDIT_STATUS_FAIL_EXPIRE:
        {
            pSession->m_uiAuditState = SMS_AUDIT_STATUS_FAIL;
            break;
        }
        case SYS_AUDIT_STATUS_GROUPSEND_UNLIM_SEND:
        {
            pSession->m_uiAuditState = SMS_AUDIT_STATUS_AUTO_PASS;
            break;
        }
        case SYS_AUDIT_STATUS_GROUPSEND_UNLIM_AUDIT:
        {
            pSession->m_uiAuditState = SMS_AUDIT_STATUS_WAIT;
            break;
        }
        case SYS_AUDIT_STATUS_WAIT_EXPIRE:
        {
            pSession->m_uiAuditState = SMS_AUDIT_STATUS_TIMEOUT;
            break;
        }
        default:
        {
            LogError("Sequence[%lu],AuditContent[%s],AuditId[%s],Invalid AuditResult[%u].",
                pSession->m_ulSequence,
                pSession->m_strAuditContent.data(),
                pSession->m_strAuditId.data(),
                pSession->m_uiAuditResult);
            break;
        }
    }

    for (list<string>::iterator it = pResp->m_pRespList->begin(); it != pResp->m_pRespList->end(); ++it)
    {
        praseJsonAuditDetail(Base64::Decode(*it), pSession);

        // 群发限制已解除转审核
        // 这里存在一个问题，如果是转审核了，但是走原审核流程进行判断时，
        // 若用户配置的是免审，所以还是不会走审核流程
        // 但是，audit_state字段的值却是0待审核

        if (SYS_AUDIT_STATUS_GROUPSEND_UNLIM_AUDIT == pSession->m_uiAuditResult)
        {
            LogDebug("Sequence[%lu], [%s:%s:%s], Group Send Limit Unrestricted 2 Audit.",
                pSession->m_ulSequence,
                pSession->m_auditReqMsg.m_strClientId.data(),
                pSession->m_auditReqMsg.m_strSmsId.data(),
                pSession->m_auditReqMsg.m_strPhone.data());

            processAudit(&pSession->m_auditReqMsg, pSession);
        }
        else
        {
            processAuditDetail(pSession);
        }
    }

    if (pResp->m_pRespList->size() >= pResp->m_uMaxSize)
    {
        redisGetList(pResp->m_RedisCmd, pResp->m_strKey, this, pResp->m_iSeq, m_uiSysParaAuditOutNumber);
    }
    else
    {
        LogNotice("Sequence[%lu], [%s:%s:%s], AuditId[%s],AuditContent[%s],AuditResult[%u],AuditState[%u], "
            "There is no more detail.",
            pSession->m_ulSequence,
            pSession->m_auditReqMsg.m_strClientId.data(),
            pSession->m_auditReqMsg.m_strSmsId.data(),
            pSession->m_auditReqMsg.m_strPhone.data(),
            pSession->m_strAuditId.data(),
            pSession->m_strAuditContent.data(),
            pSession->m_uiAuditResult,
            pSession->m_uiAuditState);

        SAFE_DELETE(pSession);
    }
}

void AuditThread::processGetCacheDetailRspMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TRedisListResp* pResp = (TRedisListResp*)(pMsg);

    if (0 != pResp->m_iResult)
    {
        LogDebug("there is no more cacheDetail.");
        m_pTimerGetCacheDetail = SetTimer(TIMER_ID_GET_CACHE_DETAIL, pResp->m_RedisCmd, m_iAuditTimeOut);
        return;
    }

    LogNotice("resp.size[%d], maxsize[%d], iReturn[%d], cmd[%s].",
        pResp->m_pRespList->size(),
        pResp->m_uMaxSize,
        pResp->m_iResult,
        pResp->m_RedisCmd.data());

    for (list<string>::iterator it = pResp->m_pRespList->begin(); it != pResp->m_pRespList->end(); ++it)
    {
        Session session;
        praseJsonAuditDetail(Base64::Decode(*it), &session);

        processAuditDetail(&session);
    }

    if (pResp->m_pRespList->size() >= pResp->m_uMaxSize)
    {
        redisGetList(pResp->m_RedisCmd, pResp->m_strKey, this, pResp->m_iSeq, m_uiSysParaAuditOutNumber);
    }
    else
    {
        m_pTimerGetCacheDetail = SetTimer(TIMER_ID_GET_CACHE_DETAIL, pResp->m_RedisCmd, m_iAuditTimeOut);
    }
}

void AuditThread::processAuditDetail(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    LogDebug("Sequence[%lu], [%s:%s:%s], Enter.",
        pSession->m_ulSequence,
        pSession->m_auditReqMsg.m_strClientId.data(),
        pSession->m_auditReqMsg.m_strSmsId.data(),
        pSession->m_auditReqMsg.m_strPhone.data());

    switch (pSession->m_uiAuditState)
    {
        case SMS_AUDIT_STATUS_PASS:
        case SMS_AUDIT_STATUS_AUTO_PASS:
        {
            if (checkMarketingTimeArea(pSession))
            {
                return;
            }
            else
            {
                sendMsg2MqC2sIo(pSession);
            }

            break;
        }
        case SMS_AUDIT_STATUS_FAIL:
        case SMS_AUDIT_STATUS_AUTO_FAIL:
        {
            pSession->m_uiState = SMS_STATUS_AUDIT_FAIL;
            pSession->m_strErrCode = AUDIT_NOT_PAAS;
            pSession->packMsg2MqC2sIoUp();
            break;
        }
        case SMS_AUDIT_STATUS_TIMEOUT:
        {
            pSession->m_uiState = SMS_STATUS_AUDIT_FAIL;
            pSession->m_strErrCode = AUDIT_TIMEOUT;
            pSession->packMsg2MqC2sIoUp();
            break;
        }
        default:
        {
            LogError("Invalid AuditState[%u].", pSession->m_uiAuditState);
            break;
        }
    }

    updateDbAuditState(pSession);
}

void AuditThread::updateDbAuditState(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    AccountMap::iterator iterAccount;
    CHK_MAP_FIND_STR_RET(m_mapAccount, iterAccount, pSession->m_auditReqMsg.m_strClientId);
    const SmsAccount& smsAccount = iterAccount->second;

    ///////////////////////////////

    string strErrorCode = pSession->m_strErrCode;
    map<string,string>::iterator iterError = m_mapSysErrCode.find(strErrorCode);
    if (iterError != m_mapSysErrCode.end())
    {
        strErrorCode.append("*");
        strErrorCode.append(iterError->second);
    }

    ///////////////////////////////

    UInt32 position = pSession->m_auditReqMsg.m_strSign.length();
    if (position > 100)
    {
        position = Comm::getSubStr(pSession->m_auditReqMsg.m_strSign, 100);
    }

    char sign[128] = {0};
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

    if (NULL != MysqlConn)
    {
        //将sign字段转义
        mysql_real_escape_string(MysqlConn,
            sign,
            pSession->m_auditReqMsg.m_strSign.substr(0, position).data(),
            pSession->m_auditReqMsg.m_strSign.substr(0, position).length());
    }

    string strSendDate = Comm::getCurrentTime();
    string strDate = pSession->m_auditReqMsg.m_strC2sDate.substr(0, 8);

    vector<string> vecId;
    Comm::splitExVector(pSession->m_auditReqMsg.m_strIds, ",", vecId);

    foreach (cs_t strId, vecId)
    {
        char sql[4096]  = {0};

        if ((SMS_AUDIT_STATUS_FAIL == pSession->m_uiAuditState)
        || (SMS_AUDIT_STATUS_TIMEOUT == pSession->m_uiAuditState))
        {
            snprintf(sql, sizeof(sql),
                "UPDATE t_sms_access_%d_%s "
                "SET state='%d',errorcode='%s',submitdate='%s',sign='%s',"
                "audit_id='%s',auditcontent='%s',audit_state='%u',audit_date='%s',innerErrorcode='%s',area='%u' "
                "WHERE id='%s' and date='%s';",
                smsAccount.m_uIdentify,
                strDate.data(),
                pSession->m_uiState,
                strErrorCode.data(),
                strSendDate.c_str(),
                sign,
                pSession->m_strAuditId.c_str(),
                pSession->m_strAuditContent.c_str(),
                pSession->m_uiAuditState,
                pSession->m_strAuditDate.c_str(),
                strErrorCode.data(),
                getPhoneArea(pSession->m_auditReqMsg.m_strPhone),
                strId.data(),
                pSession->m_auditReqMsg.m_strC2sDate.data());
        }
        else
        {
            string strAuditState = (INVALID_UINT32 == pSession->m_uiAuditState)
                                    ? "NULL"
                                    : to_string(pSession->m_uiAuditState);

            string strAuditDate = ("NULL" == pSession->m_strAuditDate)
                                    ? pSession->m_strAuditDate
                                    : "'" + pSession->m_strAuditDate + "'";

            snprintf(sql, sizeof(sql),
                "UPDATE t_sms_access_%d_%s "
                "SET audit_id='%s',auditcontent='%s',audit_state=%s,audit_date=%s,sign='%s' "
                "WHERE id='%s' and date='%s';",
                smsAccount.m_uIdentify,
                strDate.data(),
                pSession->m_strAuditId.data(),
                pSession->m_strAuditContent.data(),
                strAuditState.data(),
                strAuditDate.data(),
                sign,
                strId.data(),
                pSession->m_auditReqMsg.m_strC2sDate.data());
        }

        string strTableName;
        strTableName.append("t_sms_access_");
        strTableName.append(to_string(smsAccount.m_uIdentify));
        strTableName.append("_");
        strTableName.append(strDate);

        MONITOR_INIT(MONITOR_ACCESS_AUDIT_SMS);
        MONITOR_VAL_SET("clientid", pSession->m_auditReqMsg.m_strClientId);
        MONITOR_VAL_SET("username", pSession->m_auditReqMsg.m_strUserName);
        MONITOR_VAL_SET_INT("smsfrom",  pSession->m_auditReqMsg.m_uiSmsFrom);
        MONITOR_VAL_SET_INT("paytype",  pSession->m_auditReqMsg.m_uiPayType);
        MONITOR_VAL_SET("sign", pSession->m_auditReqMsg.m_strSign);
        MONITOR_VAL_SET("smstype", pSession->m_auditReqMsg.m_strSmsType);
        MONITOR_VAL_SET("date", pSession->m_auditReqMsg.m_strC2sDate);
        MONITOR_VAL_SET("uid", pSession->m_auditReqMsg.m_strUid);
        MONITOR_VAL_SET("dbtable", strTableName);
        MONITOR_VAL_SET("smsid", pSession->m_auditReqMsg.m_strSmsId);
        MONITOR_VAL_SET("phone", pSession->m_auditReqMsg.m_strPhone);
        MONITOR_VAL_SET("audit_id", pSession->m_strAuditId);
        MONITOR_VAL_SET_INT("audit_state", pSession->m_uiAuditState);
        MONITOR_VAL_SET("audit_date", Comm::getCurrentTime_z(0));
        MONITOR_VAL_SET_INT("costtime", time(NULL)- Comm::strToTime((char *)pSession->m_auditReqMsg.m_strC2sDate.data()));
        MONITOR_VAL_SET_INT("component_id", g_uComponentId);
        MONITOR_PUBLISH(g_pMQMonitorProducerThread);

        string strData;
        string strSql;
        strSql.append(sql);
        strSql.append("RabbitMQFlag=");
        strSql.append(strId);

        if ((SMS_AUDIT_STATUS_FAIL == pSession->m_uiAuditState)
        || (SMS_AUDIT_STATUS_TIMEOUT == pSession->m_uiAuditState))
        {
            // type=0
            MqConfig mqConfig;
		    if (!getMqCfgByC2sId(pSession->m_auditReqMsg.m_strCSid, mqConfig))
		    {
		        LogError("[%s:%s:%s] Call getMqCfgByC2sId failed. csid[%s].",
		            pSession->m_auditReqMsg.m_strClientId.data(),
		            pSession->m_auditReqMsg.m_strSmsId.data(),
		            pSession->m_auditReqMsg.m_strPhone.data(),
		            pSession->m_auditReqMsg.m_strCSid.data());
		        return;
		    }
            strData.append(pSession->m_strSendMsg);
            strData.append("&sql=");

            TMQPublishReqMsg* pReq = new TMQPublishReqMsg();
            pReq->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
            pReq->m_strExchange = mqConfig.m_strExchange;
            pReq->m_strRoutingKey = mqConfig.m_strRoutingKey;
            pReq->m_strData.append(strData);
            pReq->m_strData.append(Base64::Encode(strSql));
            g_pMQIOProducerThread->PostMsg(pReq);
			LogNotice("Exchange[%s],RoutingKey[%s],data[%s].",
            mqConfig.m_strExchange.data(),
            mqConfig.m_strRoutingKey.data(),
            strData.data());
        }
        else
        {

            TMQPublishReqMsg* pReq = new TMQPublishReqMsg();
            pReq->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
            pReq->m_strData.append(strSql);
            g_pMQC2SDBProducerThread->PostMsg(pReq);
        }

    }
}

void AuditThread::sendMsg2MqC2sIo(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    pSession->packMsg2MqC2sIoDown();

    MqConfig mqConf;
    if (!getMQConfig(pSession->m_auditReqMsg.m_strPhone,
                    pSession->m_auditReqMsg.m_strSmsType,
                    pSession->m_auditReqMsg.m_strCSid,
                    mqConf))
    {
        LogError("Call getMQConfig failed. ==mq data== %s.",
            pSession->m_strSendMsg.data());
        return;
    }

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = pSession->m_strSendMsg;
    pMQ->m_strExchange = mqConf.m_strExchange;
    pMQ->m_strRoutingKey = mqConf.m_strRoutingKey;
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMQIOProducerThread->PostMsg(pMQ);
}

