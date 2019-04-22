#ifndef __C_DIRECT_SEND_THREAD_H__
#define __C_DIRECT_SEND_THREAD_H__

#include "Thread.h"
#include "IChannel.h"
#include "SMPPChannel.h"
#include "CMPPChannel.h"
#include "SGIPChannel.h"
#include "SMGPChannel.h"
#include "CMPP3Channel.h"
#include "CMPPProvinceChannel.h"

#include "SMPPChannelSend.h"
#include "CMPPChannelSend.h"
#include "SGIPChannelSend.h"
#include "SMGPChannelSend.h"
#include "CMPP3ChannelSend.h"
#include "CMPPProvinceChannelSend.h"

#include <map>

#define etime(a,b,c,d) ((long)(c-a)*1000000+d-b)

class TDispatchToDirectSendReqMsg:public TMsg
{
public:
	smsDirectInfo m_smsParam;
};

/*内部重连*/
class TDirectChannelConnStatusMsg:public TMsg
{
public:
	TDirectChannelConnStatusMsg()
	{	
		m_uChannelId = 0;
		m_uState = 0;
	}

public:
	UInt32  m_uChannelId;
	UInt32  m_uState;
	string  m_strDesc; /*描述*/
};

class TDirectChannelUpdateMsg:public TMsg
{
public:
	TDirectChannelUpdateMsg()
	{
		m_iChannelId = 0;
		m_bDelAll = false;
	}

	int 			m_iChannelId;
	models::Channel m_Channel;
	Int64			m_lDelTime;
	bool 			m_bDelAll;
};

class TContinueLoginFailedValueMsg:public TMsg
{
public:
	TContinueLoginFailedValueMsg()
	{
		m_uValue	 = 0;
	}

	UInt32	m_uValue;
};

class TChannelCloseWaitTimeMsg:public TMsg
{
public:
	TChannelCloseWaitTimeMsg()
	{
		m_uValue	 = 0;
	}

	UInt32	m_uValue;
};

class ChannelWaitDelete
{
public:
	ChannelWaitDelete()
	{
		m_pChannel = NULL;
	}
	TDirectChannelUpdateMsg m_ChannelDelMsg;
	IChannel *				m_pChannel;
};

class CDirectSendThread : public CThread
{
	typedef std::map<int, IChannel*> ChannelInfoMap;
	typedef std::map<int, models::Channel> ChannelMap;
	
public:
	
	CDirectSendThread( const char *name ):CThread( name )
	{
		m_uContinueLoginFailedValue = 10;
		m_uTimeForPrintChlMapSize = 0;
		m_uThreadId = 0;
		m_uChannelCloseWaitTime = 15;
	}

	~CDirectSendThread()
	{		
	}
	
	bool Init(UInt32 uThreadId); 
	bool PostChannelMsg (TMsg* pMsg, int ichannelid);
	
	UInt32 GetFlowState( UInt32 uChannelId, int &flowCnt );	
	UInt32 DirectSendSimpleStatusReport( TMsg* pMsg, string errCode );

	bool InitChannelInfo(models::Channel& chan);
	void InitContinueLoginFailedValue(UInt32 uValue);
	void InitChannelCloseWaitTime(UInt32 uValue);

private:	
	virtual void HandleMsg(TMsg* pMsg);
	UInt32  HandleDispatchToDirectSendReq( TMsg* pMsg );
	void setMoRelation(IChannel* pChannel, smsDirectInfo* pSmsInfo);
	void HandleHeartTimeOut(TMsg* pMsg);
	void PrintChannelQueueSize();
	void HandleChannelCloseTimeOut(TMsg* pMsg);
	void MainLoop();
	
	bool InsertChannelMap(IChannel *pChannelInfo,models::Channel& chan);

	void HandleChannelDel(TMsg* pMsg);
	void HandleChannelAdd(TMsg* pMsg);
	void HandleChannelSlideWindowUpdate(TMsg* pMsg);
	void HandleChannelCloseWaitTimeUpdate(TMsg* pMsg);

	void HandleContinueLoginFailedValueChanged(TMsg* pMsg);

public:
	InternalService* m_pInternalService;
	UInt32 			 m_uContinueLoginFailedValue;
	UInt32			 m_uThreadId;

private:

	map<int, TMsgQueue* > m_ChannelmsgQueueMap;
	ChannelInfoMap 		  m_mapChannelInfo;	

	UInt64 m_uTimeForPrintChlMapSize;	

	map<Int64, ChannelWaitDelete> m_mapChannelWaitDel;//等待删除（关闭）的通道（结点）
	
	pthread_rwlock_t    m_rwlock; /*读写锁*/
	UInt32 				m_uChannelCloseWaitTime;
};

#endif



