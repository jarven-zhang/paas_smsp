
#ifndef __H_CMSS_CLUSTER__
#define __H_CMSS_CLUSTER__

#include "Thread.h"
#include <map>
#include "SmsContext.h"
#include "SmsAccount.h"
#include "SnManager.h"
#include "CHTTPSendThread.h"


enum {
	SMS_CLUSTER_TYPE_NONE,    /*不组包*/
	SMS_CLUSTER_TYPE_NUM_MUTI,/*单内容多号码*/
	SMS_CLUSTER_TYPE_ALL_MUTI,/*多内容多号码*/
	SMS_CLUSTER_TYPE_MAX,
};


/* 消息类型*/
enum{
	SMS_CLUSTER_MSG_TYPE_NONE,
	SMS_CLUSTER_MSG_TYPE_SUCCESS, /* 发送成功*/
	SMS_CLUSTER_MSG_TYPE_FAIL,    /* 发送失败*/
	SMS_CLUSTER_MSG_TYPE_REBACK   /* 需要重新reback*/,
};

#define SMS_CLUSTER_TX_SEND    "CLUSTER_SESSION_TX_SEND"
#define SMS_CLUSTER_TX_REBACK  "CLUSTER_SESSION_TX_REBACK"

#define SMS_CLUSTER_SESSION_TIMEROUT "CLUSETR_SESSION_TIMEOUT" 
#define SMS_CLUSTER_SESSION_SENDTIME "CLUSTER_SESSION_SEND"
#define SMS_CLUSTER_RECOVER_TIMEER   "CLUSTER_RECOVER_TIMER"


class CSMSClusterStatusReq {
public:
	CSMSTxStatusReport txReport;
public:	
	UInt32 uMsgType;
	string strExchange ;
	string strRoutingKey;
};

class CSMSClusterStatusReqMsg : public TMsg {	
public:
	CSMSClusterStatusReq txClusterStatus;
};

struct smsCtxInfo_t {
	string       smsId;
	SmsHttpInfo  http_msg;
};

typedef std::map<UInt64, smsCtxInfo_t* > clusterContextMap;

class CSMSClusterSession
{
public:
	CSMSClusterSession()
	{
		m_smsContextsMap.clear();
		m_strPhones.clear(); 
		m_pWheelTime = NULL;
		m_uStartTime = 0;	
		m_status = CC_TX_STATUS_INIT;
		m_uTotalCount = 0;		
		m_uRecvCount  = 0;	
		m_uErrCount   = 0; 		
		m_uclusterMaxNumber = 0;
		m_uClusterMaxTime = 0 ;
	    m_uClusterRecvNumber = 0;
	};

	~CSMSClusterSession()
	{
		if( m_pWheelTime != NULL ){
			delete m_pWheelTime;
			m_pWheelTime = NULL;
		}

		clusterContextMap::iterator sItr = m_smsContextsMap.begin();
		while( sItr!=m_smsContextsMap.end()){
			SAFE_DELETE( sItr->second );
			m_smsContextsMap.erase(sItr++);
		}
	};
	
public:
	clusterContextMap  m_smsContextsMap;    /* 组包的数据*/
	std::set<string>   m_strPhones;         /* 组包的手机号码,  号码不能重*/
	CThreadWheelTimer* m_pWheelTime;
	UInt64             m_uStartTime;		
	UInt64             m_uEndTime;          /*会话结束时间*/ 	
	UInt32             m_status;
	string             m_strSubmitSmsid;    /*提交的短信ID*/
	UInt32             m_uTotalCount;       /*总的分段数量*/	
	UInt32             m_uRecvCount;        /*收到的分段数量*/
	UInt32             m_uErrCount;         /*错误的分段数*/
	UInt32 			   m_uclusterMaxNumber; /*最大组包个数*/
	UInt32             m_uClusterMaxTime;   /*最大组包时间*/
	UInt32             m_uClusterRecvNumber;/*收到的组包数*/
	bool               m_bRedis;
	string             m_strRedisKey;       /*redis保存的可以值*/
	UInt64             m_uSessionId;        /*SessionId */
	bool               m_bRecover;         

};

class CSMSCluster : public CThread
{	
	/*组包内部状态*/
	enum
	{
		SMS_CLUSTER_STATUS_INIT,	/* 初始化状态*/ 
		SMS_CLUSTER_STATUS_WAIT,	/* 还需要等待组包条件*/
		SMS_CLUSTER_STATUS_SEND,	/* 已经发送没有响应回来*/
		SMS_CLUSTER_STATUS_SUCCESS, /* 发送成功*/
		SMS_CLUSTER_STATUS_FAIL,	/* 发送失败，状态报告*/
		SMS_CLUSTER_STATUS_REBACK,	/* 发送失败，reback处理*/
	};

	#define CLUSTER_MAX_TIME     300000  /* 最大组包时间5 分钟*/
	#define CLUSTER_MAX_NUMBER   10000   /*最大组包个数10000*/

	/* 会话超时时间大于http超时时间*/
	#define CLUSTER_SESSION_MAX_TIME_MS  ( 25*1000 )
	#define CLUSTER_SESSION_RECOVER_TIME ( 35*1000 )

	#define CLUSTER_CLUSTER_SESSION_CNT  200

	typedef list< CSMSClusterSession* > SessionsList;
	typedef std::map< UInt64, SessionsList > ClusterSessionMap;

public:

	CSMSCluster(const char *name ):CThread(name)
	{
		m_iRecoverCnt = 0;		
		m_mapClusterSession.clear();
	};

public:
	
	bool Init();
	void MainLoop();
	
	void HandleMsg( TMsg* pMsg );

	/*检查单个请求是否已经达组包条件*/
	UInt32 smsClusterCheck( SmsHttpInfo* httpMsg, UInt32 ClusterId );

	UInt32 smsClusterRequst( TMsg* pMsg );
	
	/*保存发送对象*/
	UInt32 smsClusterSaveContent( UInt32 ClusterId, CSMSClusterSession * session,SmsHttpInfo * httpMsg );
	
	/*发送短信到send模块*/
	UInt32 smsClusterSend( UInt64 ClusterId,  CSMSClusterSession *session );

	/*组发送成功*/
	UInt32 smsClusterStatusSuccess( TMsg *pMsg );

	/*组发送失败*/
	UInt32 smsClusterStatusFail( TMsg *pMsg );

	/*组发送reback */
	UInt32 smsClusterStatusReback( TMsg *pMsg );

	/*流水写入到redis*/
	UInt32 smsClusterRecordRedis( CSMSClusterSession *Param );

	/*流水写入到流水库*/
	UInt32 smsClusterRecordDB( CSMSClusterSession *Param );

	UInt32 smsRecordMutiDB(  CSMSClusterSession *session, CSMSClusterStatusReq *req );
	UInt32 smsRecordToDB( smsCtxInfo_t* smsCtx, CSMSClusterStatusReq *req );
	UInt32 smsRecordToRedis( smsCtxInfo_t *smsCtx, CSMSClusterStatusReq *req );
	UInt32 smsClusterSetMoRedis( SmsHttpInfo* HttpMsg );

	UInt32 smsSendToReport( SmsHttpInfo* HttpMsg, CSMSClusterStatusReq *req );

	UInt32 smsSendToHttp( smsCtxInfo_t* smsCtxInfo, UInt64 ClusterId, string strSendType,
	                                  UInt32 sessionId, string strPhones, UInt32 uSize = 1 );
	
	UInt32 smsSendToReback( smsCtxInfo_t* smsCtxInfo, CSMSClusterStatusReq *statusReq );
	
	UInt32 smsClusterSetLongFlagRedis( string strSmsId, string strPhone );


	UInt32 smsClusterDataDetailEncode( SmsHttpInfo* httpMsg, string &outString );
	UInt32 smsClusterDataDetailDecode( SmsHttpInfo* httpMsg, string &outString );
	UInt32 smsClusterDataIndexEncode( UInt32 uMaxTime, UInt32 uMaxNum, string &outString );
	UInt32 smsClusterDataIndexDecode(string intString, UInt64 &uEndTime, UInt32 &uMaxNum );

	UInt32 smsClusterLoadDataIdx();	
	UInt32 smsClusterLoadDataDetail( UInt64 ClusterId, UInt64 uSessionId );
	
	UInt32 smsClusterLoadRedisRsp( TMsg *pMsg );
	UInt32 smsClusterLoadDataIndexRedisRsp( UInt64 ClusterId, UInt64 sessionId, string redisData );
	
	UInt32 smsClusterLoadDataRedisRsp( UInt64 ClusterId, UInt64 uSessionId, string uSeq, string redisData );

	UInt32 smsClusterSaveToRedis( UInt64 ClusterId, UInt64 uSessionId, 
				UInt64 uMsgSeq, SmsHttpInfo* httpMsg, bool bSetIdx );
	
	UInt32 smsClusterDeleteRedisData( UInt64 ClusterId, CSMSClusterSession *session );


	///////////////////////////////////////////////////////////////////////////////

	UInt32 smsClusterFailReport( CSMSClusterSession * session,CSMSClusterStatusReq * req);
	
	UInt32 smsClusterSuccess(  CSMSClusterSession *session, CSMSClusterStatusReq *req );
	
	UInt32 smsClusterReback(  CSMSClusterSession *session, CSMSClusterStatusReq *req );
	
	UInt32 smsClusterTimeOut( TMsg * pMsg );	
	UInt32 smsClusterStatusComm( TMsg *pMsg );

	UInt32 smsClusterSessionDelete( UInt64 uClusterId, UInt64 uSessionId );
	CSMSClusterSession* smsClusterSessionGet( UInt64 uClusterId, UInt64 uSessionId, bool bGetOk = false );
	//CSMSClusterSession* smsClusterSessionCreate( UInt64 uClusterId, UInt32 uMaxNum, UInt32 uEndTime );
	CSMSClusterSession* smsClusterSessionCreate( UInt64 uClusterId, UInt64 &uSessionId,
				Int32 maxNum, Int32 maxTime, UInt32 uEndTime, bool bRecover );
	void smsClusterSetRedisResendInfo( SmsHttpInfo* pSms, string& strRedisData );
	
private:

	SnManager*          m_pSnMng;
	/*ClusterId 本地保存*/
	ClusterSessionMap   m_mapClusterSession;
	bool                m_bReCover;
	UInt32              m_maxClusterSize;
	Int32               m_iRecoverCnt;	
	CThreadWheelTimer*  m_Timer;
};

#endif
