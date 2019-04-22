
#ifndef __H_CMSS_CLUSTER__
#define __H_CMSS_CLUSTER__

#include "Thread.h"
#include <map>
#include "SmsContext.h"
#include "SmsAccount.h"
#include "SnManager.h"
#include "CHTTPSendThread.h"


enum {
	SMS_CLUSTER_TYPE_NONE,    /*�����*/
	SMS_CLUSTER_TYPE_NUM_MUTI,/*�����ݶ����*/
	SMS_CLUSTER_TYPE_ALL_MUTI,/*�����ݶ����*/
	SMS_CLUSTER_TYPE_MAX,
};


/* ��Ϣ����*/
enum{
	SMS_CLUSTER_MSG_TYPE_NONE,
	SMS_CLUSTER_MSG_TYPE_SUCCESS, /* ���ͳɹ�*/
	SMS_CLUSTER_MSG_TYPE_FAIL,    /* ����ʧ��*/
	SMS_CLUSTER_MSG_TYPE_REBACK   /* ��Ҫ����reback*/,
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
	clusterContextMap  m_smsContextsMap;    /* ���������*/
	std::set<string>   m_strPhones;         /* ������ֻ�����,  ���벻����*/
	CThreadWheelTimer* m_pWheelTime;
	UInt64             m_uStartTime;		
	UInt64             m_uEndTime;          /*�Ự����ʱ��*/ 	
	UInt32             m_status;
	string             m_strSubmitSmsid;    /*�ύ�Ķ���ID*/
	UInt32             m_uTotalCount;       /*�ܵķֶ�����*/	
	UInt32             m_uRecvCount;        /*�յ��ķֶ�����*/
	UInt32             m_uErrCount;         /*����ķֶ���*/
	UInt32 			   m_uclusterMaxNumber; /*����������*/
	UInt32             m_uClusterMaxTime;   /*������ʱ��*/
	UInt32             m_uClusterRecvNumber;/*�յ��������*/
	bool               m_bRedis;
	string             m_strRedisKey;       /*redis����Ŀ���ֵ*/
	UInt64             m_uSessionId;        /*SessionId */
	bool               m_bRecover;         

};

class CSMSCluster : public CThread
{	
	/*����ڲ�״̬*/
	enum
	{
		SMS_CLUSTER_STATUS_INIT,	/* ��ʼ��״̬*/ 
		SMS_CLUSTER_STATUS_WAIT,	/* ����Ҫ�ȴ��������*/
		SMS_CLUSTER_STATUS_SEND,	/* �Ѿ�����û����Ӧ����*/
		SMS_CLUSTER_STATUS_SUCCESS, /* ���ͳɹ�*/
		SMS_CLUSTER_STATUS_FAIL,	/* ����ʧ�ܣ�״̬����*/
		SMS_CLUSTER_STATUS_REBACK,	/* ����ʧ�ܣ�reback����*/
	};

	#define CLUSTER_MAX_TIME     300000  /* ������ʱ��5 ����*/
	#define CLUSTER_MAX_NUMBER   10000   /*����������10000*/

	/* �Ự��ʱʱ�����http��ʱʱ��*/
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

	/*��鵥�������Ƿ��Ѿ����������*/
	UInt32 smsClusterCheck( SmsHttpInfo* httpMsg, UInt32 ClusterId );

	UInt32 smsClusterRequst( TMsg* pMsg );
	
	/*���淢�Ͷ���*/
	UInt32 smsClusterSaveContent( UInt32 ClusterId, CSMSClusterSession * session,SmsHttpInfo * httpMsg );
	
	/*���Ͷ��ŵ�sendģ��*/
	UInt32 smsClusterSend( UInt64 ClusterId,  CSMSClusterSession *session );

	/*�鷢�ͳɹ�*/
	UInt32 smsClusterStatusSuccess( TMsg *pMsg );

	/*�鷢��ʧ��*/
	UInt32 smsClusterStatusFail( TMsg *pMsg );

	/*�鷢��reback */
	UInt32 smsClusterStatusReback( TMsg *pMsg );

	/*��ˮд�뵽redis*/
	UInt32 smsClusterRecordRedis( CSMSClusterSession *Param );

	/*��ˮд�뵽��ˮ��*/
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
	/*ClusterId ���ر���*/
	ClusterSessionMap   m_mapClusterSession;
	bool                m_bReCover;
	UInt32              m_maxClusterSize;
	Int32               m_iRecoverCnt;	
	CThreadWheelTimer*  m_Timer;
};

#endif
