#include "CAlarmThread.h"

#include "UTFString.h"
#include "propertyutils.h"
#include "UrlCode.h"
#include "main.h"

#include <stdio.h>
#include <stdarg.h>

#include <algorithm>

using namespace models;

#define ctoi(m,n) ((m[n]-'0')*10+m[n+1]-'0')
#define etime(a,b,c,d) ((long)(c-a)*1000000+d-b)

extern UInt64 g_uComponentId;

const void SendAlarm(int alarmId, const char *key, const char *format, ...)
{
    if(alarmId == 0 || key == NULL)
    {
        return;
    }

    char szTime[60] = {0};
    char strContent[MAX_ALARM_LENGTH + 1] = {0};
    tm ptm = {0};
    time_t now = time(NULL);

    localtime_r((const time_t *) &now,&ptm);

    snprintf(szTime, 60, "%04d-%02d-%02d %02d:%02d:%02d %" PRIu64 " ",
            (1900 + ptm.tm_year),
            (1 + ptm.tm_mon),
            ptm.tm_mday,
            ptm.tm_hour,
            ptm.tm_min,
            ptm.tm_sec,
            g_uComponentId);

    snprintf(strContent, 1024 ,"%s", szTime);

    va_list argp;
    va_start(argp,format);
    vsnprintf(&strContent[strlen(strContent)], MAX_ALARM_LENGTH - strlen(strContent), format, argp);
    va_end(argp);

    if(g_pAlarmThread == NULL)
    {
        LogWarn("g_pAlarmThread == NULL, content[%s]", strContent);
        return;
    }

    TAlarmReq* pAlarmReq = new TAlarmReq();
    pAlarmReq->m_iAlarmID = alarmId;
    pAlarmReq->m_strKey = key;
    pAlarmReq->m_strContent = strContent;
    pAlarmReq->m_iMsgType = MSGTYPE_ALARM_REQ;
    g_pAlarmThread->PostMsg((TMsg*)pAlarmReq);
}


CAlarmThread::CAlarmThread(const char *name) : CThread(name)
{
    // CHANNELWARNING defaul: 30
    m_uTimeRate = 30;

	m_uDBAlarmTimeIntvl = 0;
}

CAlarmThread::~CAlarmThread()
{
    if(NULL != m_pInternalService)
    {
        delete m_pInternalService;
        m_pInternalService = NULL;
    }
}

bool CAlarmThread::Init()
{
    if(false == CThread::Init())
    {
        return false;
    }

    m_SysUserMap.clear();
    m_SysNoticeMap.clear();
    m_SysNoticeUserList.clear();

    g_pRuleLoadThread->getSysUserMap(m_SysUserMap);
    g_pRuleLoadThread->getSysNoticeMap(m_SysNoticeMap);
    g_pRuleLoadThread->getSysNoticeUserList(m_SysNoticeUserList);

    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    m_pInternalService = new InternalService();
    if(NULL == m_pInternalService)
    {
        LogError("m_pInternalService is NULL!")
        return false;
    }
    m_pInternalService->Init();
    m_pLinkedBlockPool = new LinkedBlockPool();

	PropertyUtils::GetValue("dbalarm_time_intvl_mins", m_uDBAlarmTimeIntvl);
	LogDebug("dbalarm_time_intvl_mins[%d]", m_uDBAlarmTimeIntvl);
    return true;
}

void CAlarmThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "CHANNELWARNING";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        std::size_t pos = strTmp.find_last_of(";");
        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).", strSysPara.c_str(), strTmp.c_str());
            break;
        }

        int nTmp = atoi(strTmp.substr(pos + 1).data());
        if (0 > nTmp)
        {
            LogError("Invalid system parameter(%s) value(%s, %d).", strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uTimeRate = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uTimeRate);
}

void CAlarmThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL && 0 == iSelect )
        {
            usleep(1000*10);
        }
        else if (NULL != pMsg)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void CAlarmThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
	m_iCount++;
	pthread_mutex_unlock(&m_mutex);

    //struct timeval start;
    //gettimeofday(&start, NULL);
    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_ALARM_REQ:
        {
            HandleAlarmReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYS_USER_UPDATE_REQ:
        {
            HandleSysUserUpdateReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYS_NOTICE_UPDATE_REQ:
        {
            HandleSysNoticeUpdateReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYS_NOTICE_USER_UPDATE_REQ:
        {
            HandleSysNoticeUserUpdateReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == pSysParamReq)
            {
                break;
            }

            GetSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        case MSGTYPE_HTTPRESPONSE:
        {
            HandleHttpResponseMsg(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            LogDebug("message timeout!");
            HandleTimeOutMsg(pMsg);
            break;
        }
        default:
        {
            break;
        }
    }
}

void CAlarmThread::HandleAlarmReq(TMsg* pMsg)
{
    TAlarmReq* pAlarmReq = (TAlarmReq*)pMsg;
    if(NULL == pAlarmReq)
    {
        LogError("pAlarmReq is NULL!");
        return;
    }

    int iAlarmId = pAlarmReq->m_iAlarmID;
    std::string strKey = pAlarmReq->m_strKey;
    std::string strContent = pAlarmReq->m_strContent;

    LogDebug("thdName[%s] tid:[%d]\n", GetThreadName().data(), GetThreadId());

    char cAlarmId[64] = {0};
    snprintf(cAlarmId,64 ,"%d", iAlarmId);
    std::string key(cAlarmId);
    key = key + "_" + strKey;

    LogDebug("m_SysUserMap size:%d|m_SysNoticeMap size:%d|m_SysNoticeUserList size:%d",m_SysUserMap.size(), m_SysNoticeMap.size(), m_SysNoticeUserList.size());
    if(false == IsAlarmed(key, strContent, iAlarmId))
    {
        LogNotice("This is not over Rate time Alarming. key:%s|content:%s", key.data(), strContent.data());
        return;
    }

    GetAlarmPersonInfo();
    SendAlarmByMail(strContent);
    SendAlarmByPhone(iAlarmId, strContent);
    LogDebug("This is Alarming. key:%s|content:%s", key.data(), strContent.data());

    return;
}

void CAlarmThread::HandleSysUserUpdateReq(TMsg* pMsg)
{
    TUpdateSysUserReq* pUpdateSysUserReq = (TUpdateSysUserReq*)pMsg;
    if(NULL == pUpdateSysUserReq)
    {
        LogError("pUpdateSysUserReq is NULL!");
        return;
    }
    m_SysUserMap.clear();
    m_SysUserMap = pUpdateSysUserReq->m_SysUserMap;
    LogDebug("update m_SysUserMap. size:%d", m_SysUserMap.size());
}

void CAlarmThread::HandleSysNoticeUpdateReq(TMsg* pMsg)
{
    TUpdateSysNoticeReq* pUpdateSysNoticeReq = (TUpdateSysNoticeReq*)pMsg;
    if(NULL == pUpdateSysNoticeReq)
    {
        LogError("pUpdateSysNoticeReq is NULL!");
        return;
    }
    m_SysNoticeMap.clear();
    m_SysNoticeMap= pUpdateSysNoticeReq->m_SysNoticeMap;
    LogDebug("update m_SysNoticeMap. size:%d", m_SysNoticeMap.size());
}

void CAlarmThread::HandleSysNoticeUserUpdateReq(TMsg* pMsg)
{
    TUpdateSysNoticeUserReq* pUpdateSysNoticeUserReq = (TUpdateSysNoticeUserReq*)pMsg;
    if(NULL == pUpdateSysNoticeUserReq)
    {
        LogError("pUpdateSysNoticeUserReq is NULL!");
        return;
    }
    m_SysNoticeUserList.clear();
    m_SysNoticeUserList = pUpdateSysNoticeUserReq->m_SysNoticeUserList;
    LogDebug("update m_SysNoticeUserList. size:%d", m_SysNoticeUserList.size());
}

void CAlarmThread::HandleHttpResponseMsg(TMsg* pMsg)
{
	THttpResponseMsg* pHttpResponse = (THttpResponseMsg*)pMsg;
	if(NULL == pHttpResponse)
    {
        LogError("pHttpResponse is NULL!");
        return;
    }

	LogDebug("get response,status[%d], status code[%d]!", pHttpResponse->m_uStatus,
		pHttpResponse->m_tResponse.GetStatusCode());

	SessionMap::iterator it = m_mapSession.find(pHttpResponse->m_iSeq);
	if (it == m_mapSession.end())
	{
		LogWarn("iSeq[%lu] is not find in m_mapSession.", pHttpResponse->m_iSeq);
		return;
	}

	it->second->Destory();
	if(it->second != NULL)
	{
		delete it->second;
		it->second = NULL;
	}
	m_mapSession.erase(it);
}

void CAlarmThread::HandleTimeOutMsg(TMsg* pMsg)
{
    SessionMap::iterator it = m_mapSession.find(pMsg->m_iSeq);
    if (it == m_mapSession.end())
    {
        LogWarn("iSeq[%lu] is not find in m_mapSession.", pMsg->m_iSeq);
        return;
    }

	it->second->Destory();
	if(it->second != NULL)
	{
		delete it->second;
		it->second = NULL;
	}
	m_mapSession.erase(it);
}

bool CAlarmThread::IsAlarmed(const std::string key, const std::string value, int alarmid)
{
    if (key.empty())
    {
        LogError("key is empty.");
        return false;
    }

	UInt32 iTimeIntvl = 0;
	if(alarmid == DB_CONN_FAILED_ALARM || alarmid == REDIS_CONN_FAILED_ALARM)
	{
		 iTimeIntvl = m_uDBAlarmTimeIntvl;
	}
	else
	{
		iTimeIntvl = m_uTimeRate;
	}

	time_t now_t = time(NULL);
    AlarmTimeMap::iterator itr = m_mapAlarmTime.find(key);
    if (itr == m_mapAlarmTime.end())
    {
        m_mapAlarmTime[key] = now_t;
        return true;
    }

    if ((now_t - itr->second) > iTimeIntvl*60)
    {
        itr->second = now_t;
        return true;
    }
    else
    {
        LogNotice("last warn time[%ld],now time[%ld],TimeRate[%d].", itr->second,
        	now_t, iTimeIntvl*60);
        return false;
    }
}

void CAlarmThread::GetAlarmPersonInfo()
{
    if(m_SysUserMap.empty() || m_SysNoticeMap.empty() || m_SysNoticeUserList.empty())
    {
        LogWarn("m_SysUserMap or m_SysNoticeMap or m_SysNoticeUserList size is ZERO!")
        return;
    }

    m_strPersonMailInfo.clear();
	m_NoticeTypeAmMap.clear();

    tm ptm = {0};
    time_t now_t = time(NULL);
    localtime_r((const time_t *) &now_t, &ptm);
    int now = ptm.tm_hour * 3600 + ptm.tm_min * 60 + ptm.tm_sec;
    int iPhoneNum = 0;
    int iMailNum = 0;

    SysNoticeUserList::iterator it;
    for(it = m_SysNoticeUserList.begin(); it != m_SysNoticeUserList.end(); ++it)
    {
        SysNoticeUser snu = *it;
        SysNoticeMap::iterator iter;
		string strPersonPhoneInfo = "";
        iter = m_SysNoticeMap.find(snu.m_iNoticeId);
        if (iter == m_SysNoticeMap.end())
        {
            continue;
        }
        SysNotice sn = iter->second;

        if(1 != sn.m_iStatus)
        {
            continue;
        }
        std::string start_t = sn.m_StartTime;
        std::string end_t = sn.m_EndTime;
        int start = ctoi(start_t.data(), 0)*3600+ctoi(start_t.data(), 3)*60+ctoi(start_t.data(), 6);
        int end = ctoi(end_t.data(), 0)*3600+ctoi(end_t.data(), 3)*60+ctoi(end_t.data(), 6);
        if(now < start || end < now)
        {
            continue;
        }
        SysUserMap::iterator iter1;
        iter1 = m_SysUserMap.find(snu.m_iUserId);
        if(iter1 == m_SysUserMap.end())
        {
            continue;
        }

        SysUser su = iter1->second;
        if(1 == snu.m_iAlarmType)
        {
            m_strPersonMailInfo.append(su.m_Email);
            m_strPersonMailInfo.append(",");
            iMailNum++;
        }
        else if(2 == snu.m_iAlarmType)
        {
            strPersonPhoneInfo.append(su.m_Mobile);
            strPersonPhoneInfo.append(",");
        	NoticeTypeAlarmMap::iterator itNt = m_NoticeTypeAmMap.find(sn.m_ucNoticeType);
        	if (itNt == m_NoticeTypeAmMap.end())
        	{
        		m_NoticeTypeAmMap.insert(make_pair(sn.m_ucNoticeType, strPersonPhoneInfo));
			}
			else
			{
				itNt->second += strPersonPhoneInfo;
			}

            iPhoneNum++;
        }
        else if(3 == snu.m_iAlarmType)
        {
            strPersonPhoneInfo.append(su.m_Mobile);
            strPersonPhoneInfo.append(",");
        	NoticeTypeAlarmMap::iterator itNt = m_NoticeTypeAmMap.find(sn.m_ucNoticeType);
        	if (itNt == m_NoticeTypeAmMap.end())
        	{
        		m_NoticeTypeAmMap.insert(make_pair(sn.m_ucNoticeType, strPersonPhoneInfo));
			}
			else
			{
				itNt->second += strPersonPhoneInfo;
			}
			iPhoneNum++;
            m_strPersonMailInfo.append(su.m_Email);
            m_strPersonMailInfo.append(",");
            iMailNum++;
        }
        else
        {
            LogWarn("not find alarm type:%d", snu.m_iAlarmType);
        }

        LogDebug("noticeid:%d|userid:%d|alarmtype:%d|mail[%d]:%s|phone[%d]:%s",snu.m_iNoticeId, snu.m_iUserId,
            snu.m_iAlarmType, iMailNum, su.m_Email.data(), iPhoneNum, su.m_Mobile.data());
    }
}

void CAlarmThread::SendAlarmByMail(const std::string& strData)
{
	if (NULL == g_pDisPathDBThreadPool)
	{
		LogNotice("g_pDisPathDBThreadPool is null,not must mail warn.");
		return;
	}

    if(0 == m_strPersonMailInfo.size())
    {
        LogWarn("m_strPersonMailInfo size is ZERO!");
        return;
    }

    m_strPersonMailInfo.erase(m_strPersonMailInfo.length()-1);

    LogWarn("mail to SQL. count:%d|addr:%s|content:%s", m_strPersonMailInfo.size(), m_strPersonMailInfo.data(), strData.data());

    //send to SQL
    char sql[4096] = {0};
    snprintf(sql,4096,"insert into t_sms_send_warn(mail_content,mail_address,status) values('%s','%s','%d');",
		strData.data(),m_strPersonMailInfo.data(),2);
    TDBQueryReq* pMsg = new TDBQueryReq();
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }
    pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pMsg->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(pMsg);
}

void CAlarmThread::SendAlarmByPhone(int nAlarmID, const std::string& strData)
{
    if(m_NoticeTypeAmMap.empty())
    {
        LogWarn("m_NoticeTypeAmMap size is ZERO!");
        return;
    }

	string strUrl = "";
    string strPostData = "";
    PropertyUtils::GetValue("alarmurl", strUrl);
    PropertyUtils::GetValue("alarmopostdata", strPostData);

    if("" == strUrl || "" == strPostData)
    {
        LogError("strUrl or strPostData is NULL!");
        return;
    }
	string strPersonPhoneInfo = "";
	NoticeTypeAlarmMap::iterator itNtAm;
	// ÏµÍ³¸æ¾¯,·¶Î§[1 - 100]
	// ÔËÓª¸æ¾¯,·¶Î§[101 - 200]
	if(1 <= nAlarmID && nAlarmID <= 100)
	{
		itNtAm = m_NoticeTypeAmMap.find(0);
		if (itNtAm != m_NoticeTypeAmMap.end())
		{
			strPersonPhoneInfo = itNtAm->second;
		}
	}
	else if(101 <= nAlarmID && nAlarmID <= 200)
	{
		itNtAm = m_NoticeTypeAmMap.find(1);
		if (itNtAm != m_NoticeTypeAmMap.end())
		{
			strPersonPhoneInfo = itNtAm->second;
		}
	}
	if (strPersonPhoneInfo.empty())
	{
		LogWarn("strPersonPhoneInfo size is ZERO!");
		return;
	}

	/* build session */
	UInt64 iSeq = m_SnMng.getSn();
	AlarmSession* pAlarmSession = new AlarmSession();
	pAlarmSession->m_strUrl.assign(strUrl);
	pAlarmSession->m_strPostData.assign(strPostData);
	pAlarmSession->m_strContent.assign(strData);
	pAlarmSession->m_strphone.assign(strPersonPhoneInfo);

	/* trim the phones */
	pAlarmSession->m_strphone.erase(pAlarmSession->m_strphone.length() - 1);
    LogWarn("phone to Channel.num:%s|content:%s", pAlarmSession->m_strphone.data(), pAlarmSession->m_strContent.data());

	/* fill the phone num */
    std::string::size_type pos = pAlarmSession->m_strPostData.find("%phone%");
    if (pos == std::string::npos)
    {
		LogError("not find '%phone%'!");
		pAlarmSession->Destory();
		delete pAlarmSession;

        return;
    }
	pAlarmSession->m_strPostData.replace(pos, strlen("%phone%"), pAlarmSession->m_strphone);

	/* fill content */
	string str1 = "%e3%80%90%e4%ba%91%e4%b9%8b%e8%ae%af%e3%80%91";
	str1 = http::UrlCode::UrlDecode(str1);
    std::string strContent = "";
    strContent.assign(str1);
    strContent.append(pAlarmSession->m_strContent);
    pos = pAlarmSession->m_strPostData.find("%content%");
    if (pos == std::string::npos)
    {
        LogError("not find '%content%'!");
		pAlarmSession->Destory();
		delete pAlarmSession;

		return;
    }
	pAlarmSession->m_strPostData.replace(pos, strlen("%content%"), strContent);

	/* Http send */
    http::HttpSender* pHttpSender = new http::HttpSender();
    std::vector<string> vhead;
	std::string strIP;

    if (false == pHttpSender->Init(m_pInternalService, iSeq, this)
		|| false == m_SmspUtils.CheckIPFromUrl(pAlarmSession->m_strUrl, strIP))
    {
        LogError("pHttpSender->Init is failed or CheckIPFromUrl err, url: %s.", pAlarmSession->m_strUrl.c_str());
		pHttpSender->Destroy();
		pAlarmSession->Destory();
		delete pHttpSender;
		delete pAlarmSession;

        return;
    }
    pHttpSender->setIPCache(inet_addr(strIP.c_str()));
	pHttpSender->Post(pAlarmSession->m_strUrl, pAlarmSession->m_strPostData, &vhead);

	/* add to session */
	pAlarmSession->m_pTimer = SetTimer(iSeq, "", 5000);
    pAlarmSession->m_pHttpSender = pHttpSender;
	m_mapSession[iSeq] = pAlarmSession;

	return;
}

UInt32 CAlarmThread::GetSessionMapSize()
{
	return m_mapSession.size();
}

