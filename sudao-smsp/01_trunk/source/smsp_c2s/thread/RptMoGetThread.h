/******************************************************************************

  Copyright (C), 2001-2017, ucpaas Co., Ltd.

 ******************************************************************************
  File Name     : RptMoGetThread.h
  Version       : 
  Author        : fjx
  Created       : 2017/3/23
  Last Modified :
  Description   : RptMoget. 
  Function List :1. 每隔2mins检查 CMPP/SMPP 和SGIP上行链路。 若存在通路，则从redis中拉取状态报告推过去。
  				 2. SGIP上行链路每隔10mins将没有链路的再创建一次，创建后推消息到本线程，本线程拉取状态报告并推送出去
 
  
  History       :
  1.Date        : 2017/3/23
    Author      : fxh
    Modification: Created file

******************************************************************************/


#ifndef __C_RPRMOGETTHREAD_H__
#define __C_RPRMOGETTHREAD_H__

#include "Thread.h"
//#include "sockethandler.h"
//#include "internalservice.h"
#include "SnManager.h"
#include <map>
//#include "socket.h"
//#include "ChannelScheduler.h"
//#include "ChannelMng.h"
//#include "HttpSender.h"
//#include "SmspUtils.h"



using namespace std;
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
	string m_strRtUrl;
	string m_strRtData;
	string m_strMoUrl;
	string m_strMoData;
	UInt32 m_iHttpmode;
	UInt32 m_uChannelType;
	UInt32 m_uChannelIdentify;
	CThreadWheelTimer* m_pTimer;
};

class TReportRepostMsg: public TMsg
{
public:
	TReportRepostMsg()
	{
		m_uSmsFrom = 0;
		m_istatus = 0;
	}

	string m_strCliendid;
	UInt32 m_uSmsFrom;
	string m_strMsgid;
	int	m_istatus;
	string m_strRemark;
	string m_strPhone;
	string m_strErr;		//for smgp only m_strErr  m_strShortmsg
	
};

class TMoRepostMsg: public TMsg
{
	//clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*
public:
	TMoRepostMsg()
	{
		m_uSmsFrom = 0;
	}

	string m_strCliendid;
	string m_strDestid;
	string m_strPhone;
	string m_strContent;
	UInt32 m_uSmsFrom;
	
};



class RptMoGetThread : public CThread
{
	//typedef std::map<UInt32, TReportGetChannels*> TReportGetChannelsMap;

public:
	RptMoGetThread(const char *name);
	~RptMoGetThread();
	bool Init();
	UInt32 GetSessionMapSize();

private:
	void HandleMsg(TMsg* pMsg);
	void HandleTimeOutMsg(TMsg* pMsg);

	UInt32 getRptMoByUser(string strCliendid);
	void HandleRedisListResp(TMsg* pMsg);

	void HandleGetLinkedClientResp(TMsg* pMsg);

	void HandleClientLoginSucInfo(TMsg* pMsg);
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);
private:
	UInt32 m_uMaxGetSize;
	UInt32 m_uTime;
	CThreadWheelTimer* m_pTimer_CheckLink;

	//CThreadWheelTimer* m_pTimer_exltRedisCmdAgain;

	SnManager* m_pSnManager;
};

#endif ///__C_RPRMOGETTHREAD_H__

