#ifndef _CRedisThread_h_
#define _CRedisThread_h_

#include <string.h>
#include "Thread.h"
#include "hiredis.h"
#include <list>
#include "CRedisBase.h"

#define CHECK_REDIS_CONN_TIMEOUT       3*1000   //3s check db conn.
#define CHECK_REDIS_CONN_TIMER_MSGID   3090

using namespace std;
enum
{
	REDIS_COMMON_USE_NO_CLEAN_0 = 0,//常用不清理0区
	REDIS_COMMON_USE_CLEAN_1 = 1,//常用可清理1区
	REDIS_COMMON_USE_CLEAN_2 = 2,//常用可清理2区
	REDIS_UNCOMMON_USE_CLEAN_3 = 3,//不常用可清理3区
	REDIS_UNCOMMON_USE_CLEAN_4 = 4,//不常用可清理4区
	REDIS_COMMON_USE_CLEAN_5 = 5,//常用可清理5区 only for blacklist
	REDIS_COMMON_USE_CLEAN_6 = 6,//调度系统使用
	REDIS_COMMON_USE_CLEAN_7 = 7,//保存组包数据
	REDIS_COMMON_USE_CLEAN_8 = 8,//长短信拼接存储
	REDIS_COMMON_USE_CLEAN_9 = 9,
 	REDIS_USE_INDEX_MAX,
};

//for redis alarm
enum
{
	REDIS_CONNECT_FAIL = 0,
	REDIS_EXE_COMMAND_FAIL,
};

class TRedisReq: public TMsg
{
public:
	TRedisReq()
	{
		m_uReqTime = 0;
		m_pSender = NULL;
	}
	TRedisReq(const TRedisReq& RedisReq)
	{
		m_strSessionID = RedisReq.m_strSessionID;
		m_iSeq = RedisReq.m_iSeq;
		m_RedisCmd = RedisReq.m_RedisCmd;
		m_iMsgType = RedisReq.m_iMsgType;
		m_pSender = RedisReq.m_pSender;
		m_strKey = RedisReq.m_strKey;
		m_uReqTime = RedisReq.m_uReqTime;
		m_uMaxSize = RedisReq.m_uMaxSize;
		m_string = RedisReq.m_string;
	}

	TRedisReq& operator=(const TRedisReq& RedisReq)
	{
		m_strSessionID = RedisReq.m_strSessionID;
		m_iSeq = RedisReq.m_iSeq;
		m_RedisCmd = RedisReq.m_RedisCmd;
		m_iMsgType = RedisReq.m_iMsgType;
		m_pSender = RedisReq.m_pSender;
		m_strKey = RedisReq.m_strKey;
		m_uReqTime = RedisReq.m_uReqTime;
		m_uMaxSize = RedisReq.m_uMaxSize;
		m_string = RedisReq.m_string;
		return *this;
	}

	UInt64 m_uReqTime;		//打印日志，看redis请求的耗时
    std::string m_RedisCmd;		//redis命令
	UInt32 m_uMaxSize;		//本次请求最大的应答条数，对于redislist有用
	string m_strKey;		// eg: report_cache:clientid_20160920。 本次操作的键值
	string m_string;
};

class TRedisResp: public TMsg
{
public:
	TRedisResp()
	{
		m_uReqTime = 0;
	}

	UInt64 m_uReqTime;
	string m_RedisCmd;
    int m_iResult;
    redisReply* m_RedisReply;
	string m_string;
};

class TRedisListReportReq: public TMsg
{
public:
	TRedisListReportReq()
	{
		m_uReqTime = 0;
		m_iIndex = -1;
	}

	UInt32 m_iIndex;
	UInt64 m_uReqTime;
    std::string m_RedisCmd;
	string m_strKey;		// eg: report_cache:clientid_20160920
	UInt32 m_uMaxSize;

};

class TRedisListReportResp: public TMsg
{
public:
	TRedisListReportResp()
	{
		m_uReqTime = 0;
	}

	UInt64 m_uReqTime;
    int m_iResult;		//0 ,ok, -1,no data/no key, -2.link error
    UInt32 m_uMaxSize;		//maxSize
    string m_strReport;	//eg: strReport="[{****},{****},{****}]";
    string m_strKey;		// eg: report_cache:clientid_20160920
};

class TRedisListResp: public TMsg
{
public:
	TRedisListResp()
	{
		m_pRespList = NULL;
		m_uMaxSize = 0;
		m_iResult = 0;
		m_pRespList = new list<string>;
	}
	~TRedisListResp()
	{
		if(m_pRespList)
		{
			m_pRespList->clear();
			delete m_pRespList;
		}
		m_pRespList = NULL;
	}
	int m_iResult;		//0 ,ok, -1,no data/no key, -2.link error
	list<string>* m_pRespList;
	UInt32 m_uMaxSize;
	string m_RedisCmd;
	string m_strKey;
};


class TRedisUpdateInfo : public TMsg
{
public:
	TRedisUpdateInfo()
	{
		m_uPort = 0;
	}

	TRedisUpdateInfo(const TRedisUpdateInfo& other){
		this->m_strIP = other.m_strIP;
		this->m_uPort = other.m_uPort;
		this->m_strPwd= other.m_strPwd;
		this->m_iMsgType = other.m_iMsgType;

	}

	TRedisUpdateInfo& operator=(const TRedisUpdateInfo& other)
	{
		this->m_strIP = other.m_strIP;
		this->m_uPort = other.m_uPort;
		this->m_strPwd= other.m_strPwd;
		this->m_iMsgType = other.m_iMsgType;
		return *this;
	}
public:
	std::string m_strIP;
	UInt32 m_uPort;
	std::string m_strPwd;
};

class CRedisThread:public CThread
{

public:
    CRedisThread(const char *name,UInt32 uRedisIndex);
    ~CRedisThread();
    bool Init(std::string ip, unsigned int port, int connTimeout, std::string &strPwd );
	bool PostMngMsg(TMsg* pMsg);

private:
    virtual void HandleMsg(TMsg* pMsg);
    void RedisReqProc(TMsg* pMsg);
	void RedisListReportReq(TMsg* pMsg);
    void RedisListReportMultiReq(TMsg* pMsg);
	void RedisLisReq(TMsg* pMsg);	//
	void RedisListMultiReq(TMsg* pMsg);	//
	void RedisFailedAlarm(UInt32 selectIndex, int iCount, int type);
	void MainLoop();
	bool UpdateRedisAddrInfo( TMsg *pMsg );
	void RedisReqProcMuti( TMsg *pMsg );
	int  RedisPipelineGetResult();
	bool HandleTimeOut();
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);

	bool checkRedisServState();

	CRedisBase  m_redisServ0;  /* 主连接信息*/
	CRedisBase  m_redisServ1;  /* 备连接信息*/
    CRedisBase *m_redisRunner; /* 当前运行的节点*/
	CRedisBase *m_redisFree;   /* 当前空闲节点*/
    TMsgQueue   m_msgMngQueue; /* 管理队列优先处理*/

	UInt32 m_uRedisIndex;
	UInt32 m_uRedisExecultErrCunts;
	UInt64 m_componentId;
	UInt64 m_uLastCheckTime;
	UInt64 m_uCheckPeriod; /* 检测周期*/
	list < TRedisReq > m_listRedisReq;
	UInt32 m_uRedisCmdAppendMaxCnt;
    bool m_bUsePipeline;
	bool   		m_bSwitchNow;
	CThreadWheelTimer *m_pTimer;
};

#endif

