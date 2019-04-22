#ifndef __H_REDIS_BASE__
#define __H_REDIS_BASE__

#include "platform.h"
#include <string>
#include "hiredis.h"

using namespace std;

void paserReply(const char* field, redisReply* reply, string& outstr);
void freeReply(void* reply);

class CRedisBase
{

public:	
	CRedisBase();
	~CRedisBase();

	bool Init( UInt32 ConnTimeout, UInt32 connMaxCount, UInt32 uSelectId );
	bool Conn( std::string ip, UInt32 port, std::string strPwd="" );
	bool reConn();	
	bool Close();
	
	bool SetTimeOut( Int32 timeout );
	bool getRedisServAddr(std::string &strIP, UInt32 &uPort);
	UInt64 getSequence();
	redisReply* execSyncCmd( const char* redisCmd, UInt32 uRetranCnt = 1 );
	bool checkConnState( bool bSendPing = false );
	Int32 getReply(void **redisReply );
	Int32 appendCmd( const char* redisCmd);

public:
	std::string m_strIP;
	UInt32 m_uPort;
	std::string m_strPassWord;	/*»œ÷§√‹¬Î*/
	UInt64 m_uSequence;
	redisContext* m_RedisCtx;	
	Int32  m_iredisConnTimeout;
	Int32  m_uConnectCount;
	Int32  m_uRedisIdx;
};

#endif
