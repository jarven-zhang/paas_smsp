#ifndef __C_ALARM_THREAD_H__
#define __C_ALARM_THREAD_H__

#include <time.h>
#include <map>
#include "Thread.h"
#include "SysUser.h"
#include "SysNotice.h"
#include "SysNoticeUser.h"
#include "sockethandler.h"
#include "internalservice.h"
#include "SnManager.h"
#include "SmspUtils.h"
#include "HttpSender.h"
#include "LogMacro.h"

using namespace models;

#define MAX_ALARM_LENGTH                        1024
#define INTERVAL_TIME                           10
#define ALARM_COUNT                             3

extern const void SendAlarm(int alarmId, const char *key, const char *format, ...);

#define Alarm(a, b, ...) SendAlarm( a, b, __VA_ARGS__);

class TAlarmReq: public TMsg
{
public:
    int m_iAlarmID;
    std::string m_strKey;
    std::string m_strContent;
};

class TAlarmResp: public TMsg
{
public:
    int m_iResult;
};

class AlarmParam
{
public:
	AlarmParam()
	{
		m_iCount = 0;
		m_iTime = 0;
		m_strContent = "";
	}
    int m_iCount;
    long m_iTime;
    std::string m_strContent;
};

class AlarmSession
{
public:
	AlarmSession()
	{
		m_pHttpSender = NULL;
		m_pTimer = NULL;
	}

	void Destory()
	{
		if(m_pTimer != NULL)
		{
			delete m_pTimer;
			m_pTimer = NULL;
		}
		if(m_pHttpSender != NULL)
		{
			m_pHttpSender->Destroy();
			delete m_pHttpSender;
			m_pHttpSender = NULL;
		}
	}

public:
	string m_strUrl;
	string m_strPostData;
	string m_strContent;
	string m_strphone;
	http::HttpSender* m_pHttpSender;
	CThreadWheelTimer* m_pTimer;
};

class CAlarmThread : public CThread
{
	typedef std::map<int, SysUser>                      SysUserMap;
	typedef std::map<int, SysNotice>                    SysNoticeMap;
	typedef std::list<SysNoticeUser>                    SysNoticeUserList;
	typedef std::map<UInt64, AlarmSession*>				SessionMap;
	typedef std::map<std::string,UInt64>                AlarmTimeMap;
	// TINYINT:NoticeType
	// std::string:Mobile号码段[以,号分割]
	typedef std::map<TINYINT, std::string>				NoticeTypeAlarmMap;

public:
    CAlarmThread(const char *name);
    ~CAlarmThread();
    bool Init();
	UInt32 GetSessionMapSize();

private:
	virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);
    void HandleAlarmReq(TMsg* pMsg);
    void HandleSysUserUpdateReq(TMsg* pMsg);
    void HandleSysNoticeUpdateReq(TMsg* pMsg);
    void HandleSysNoticeUserUpdateReq(TMsg* pMsg);
	void HandleHttpResponseMsg(TMsg* pMsg);
	void HandleTimeOutMsg(TMsg* pMsg);
    bool IsAlarmed(const std::string key, const std::string value, int alarmid);
    void CleanAlarmMapOnDay();
    void GetAlarmPersonInfo();
    void SendAlarmByMail(const std::string& strData);
    void SendAlarmByPhone(int nAlarmID, const std::string& strData);
	void HandleHostIpResp(TMsg* pMsg);
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);

private:
	InternalService* m_pInternalService;
    SysUserMap m_SysUserMap;
    SysNoticeMap m_SysNoticeMap;
    SysNoticeUserList m_SysNoticeUserList;
	NoticeTypeAlarmMap m_NoticeTypeAmMap;
    std::string m_strPersonMailInfo;
	SnManager m_SnMng;
	SmspUtils m_SmspUtils;
	SessionMap m_mapSession;

	UInt32 m_uTimeRate;
	AlarmTimeMap m_mapAlarmTime;

	UInt32 m_uDBAlarmTimeIntvl;		//min
};

#endif

