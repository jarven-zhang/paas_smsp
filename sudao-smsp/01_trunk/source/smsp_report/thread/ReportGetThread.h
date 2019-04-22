#ifndef __C_REPORTGETTHREAD_H__
#define __C_REPORTGETTHREAD_H__

#include "Thread.h"
#include "sockethandler.h"
#include "internalservice.h"
#include "SnManager.h"
#include <map>
//#include "socket.h"
#include "ChannelScheduler.h"
#include "ChannelMng.h"
#include "HttpSender.h"
#include "SmspUtils.h"



using namespace std;
using namespace models;
using namespace smsp;
using namespace http;


class TReportGetChannels: public TMsg
{
public:
	TReportGetChannels()
	{
		m_iMsgType = 0;
	    m_pTimer = NULL;
		m_iChannelID = 0;
		m_iHttpmode = 0;
		m_uChannelIdentify = 0;
	}
	UInt32 m_iChannelID;
	string m_strChannelname;
	string m_strChannelLibName;
	string m_strRtUrl;
	string m_strRtData;
	string m_strMoUrl;
	string m_strMoData;
	UInt32 m_iHttpmode;
	UInt32 m_uChannelType;
	UInt32 m_uChannelIdentify;
	UInt32 m_uClusterType;
	bool   m_bSaveReport;
	string m_strMoPrefix;
	CThreadWheelTimer* m_pTimer;
};

class TReportGetSession
{
public:
	TReportGetSession()
	{
	    m_timer = NULL;
		m_pHttpSender = NULL;
		m_uChannelId = 0;
		m_iHttpmode = 0;
		m_uChannelIdentify = 0;
	}
	string m_strUrl;
	string m_strData;
	string m_strChannelname;
	string m_strChannelLibName;
	UInt32 m_iHttpmode;
	UInt32 m_uChannelId;
	UInt32 m_uChannelType;
	UInt32 m_uChannelIdentify;
	UInt32 m_uClusterType;
	bool   m_bSaveReport;
	string m_strMoPrefix; /*上行前缀*/
	
	http::HttpSender* m_pHttpSender;
	CThreadWheelTimer* m_timer;		
};



class ReportGetThread : public CThread
{
	typedef std::map<UInt32, TReportGetChannels*> TReportGetChannelsMap;
	typedef std::map<UInt64, TReportGetSession*> TReportGetSessionMap;
	typedef std::map<int,models::Channel> ChannelMap;
	typedef std::vector<smsp::StateReport> StateReportVector;
    typedef std::vector<smsp::UpstreamSms> UpstreamSmsVector;
public:
	ReportGetThread(const char *name);
	~ReportGetThread();
	bool Init();
	UInt32 GetSessionMapSize();
	
private:
	void MainLoop();
	void HandleMsg(TMsg* pMsg);
	void HandleHostIpRespMsg(UInt64 iSeq, UInt32 uResult, UInt32 uIp, string strSessionID);
	void HandleChannelInfoUpdateMsg(TMsg* pMsg);
	void HandleHttpResponseMsg(TMsg* pMsg, bool is_https = false);		//http应答
	void HandleTimeOutMsg(TMsg* pMsg);
	void SetTimerSend(UInt32 uReportsize,UInt32 uChannelId,string& strChannelLibName, string& strChannelName);

private:
	InternalService* m_pInternalService;
	SnManager m_SnMng;
	ChannelMng* m_pLibMng;
	SmspUtils m_SmspUtils;

	////key is channelId
	TReportGetChannelsMap m_mapChannelInfo;	

	////key is iSeq
	TReportGetSessionMap m_mapSession;
};

#endif ///__C_REPORTGETTHREAD_H__

