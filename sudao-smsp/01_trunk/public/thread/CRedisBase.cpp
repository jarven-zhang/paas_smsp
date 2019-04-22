#include "CRedisBase.h"
#include "LogMacro.h"


void paserReply(const char* field, redisReply* reply, string& outstr)
{
    if (reply)
    {
        size_t i = 0;
        for (i = 0; i < reply->elements; i++)
        {
            if (strcasecmp(reply->element[i]->str, field)==0)
            {
                break;
            }
        }
        if (i != reply->elements)
        {
            outstr.append(reply->element[i+1]->str, reply->element[i+1]->len);
        }
    }
}

void freeReply(void* reply)
{
    if (reply)
    {
        freeReplyObject(reply);
    }
}

CRedisBase::CRedisBase()
{
}

CRedisBase::~CRedisBase()
{
}

bool CRedisBase::Init( UInt32 ConnTimeout, UInt32 connMaxCount, UInt32 uSelectId )
{
	m_iredisConnTimeout = ConnTimeout;
	m_uConnectCount = connMaxCount;
	m_uRedisIdx     = uSelectId;
	return true;
}

/*连接到redis服务器*/
bool CRedisBase::Conn( std::string ip, UInt32 port, std::string strPwd )
{	
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * m_iredisConnTimeout;
	char Rediscmd[128] = {0};
	Int32 iConTimes = 0;
	redisReply * pReply = NULL;

	/*建立连接*/	
	do
	{
		m_RedisCtx = redisConnectWithTimeout( ip.c_str(), port, tv );
	    if ( !m_RedisCtx || m_RedisCtx->err )
	    {
	        LogWarn("redis connect [%s:%d] fail !!", 
					m_strIP.data(), m_uPort );		
			Close();
			iConTimes++;
	    }
		else
		{
			/*Success*/
			break;
		}
		
	} while ( iConTimes < m_uConnectCount );

	if( iConTimes >= m_uConnectCount )
	{
		LogError("redis connect %d times fail!! ", m_uConnectCount);
		return false; 
	}
		
    SetTimeOut(500);

	/*密码认证*/
	if(!strPwd.empty())
	{
		snprintf(Rediscmd, sizeof(Rediscmd), "AUTH %s", strPwd.data());
		pReply = execSyncCmd( Rediscmd, m_uConnectCount );
		if( NULL == pReply  || NULL == pReply->str )
		{
			LogError(" redis [%s:%d] Auth[%s] fail!!", 
						m_strIP.data(), m_uPort, strPwd.data());
			freeReply(pReply);
			Close();
			return false;
		}
		else if ( 0 != strncasecmp( pReply->str,"OK", 2 ))
		{	
			/*服务端没有设置密码*/
			string strNoPwd =  pReply->str;
			if( string::npos == strNoPwd.find("no password"))
			{
				LogError(" redis [%s:%d] Auth[%s] fail!!", 
						m_strIP.data(), m_uPort, strPwd.data());
				freeReply(pReply);
				Close();
				return false;
			}
		}
		freeReply(pReply);
		pReply = NULL;

		LogNotice("Redis[ %s:%d ] Auth Success", m_strIP.data(), m_uPort );
		
	}


	/* 选择分区*/
	memset(Rediscmd, 0, sizeof(Rediscmd));
	snprintf(Rediscmd, sizeof(Rediscmd), "SELECT %d", m_uRedisIdx );
	/* 选择分区*/
	pReply = execSyncCmd( Rediscmd, m_uConnectCount );
	if( NULL == pReply  
		|| pReply->str == NULL 
		|| 0 != strncasecmp( pReply->str,"OK",2 ))
	{		
		LogError("redis select index[%d] fail!! \n", m_uRedisIdx );
		freeReply(pReply);
		Close();
		return false;
	}

	freeReply(pReply);
    LogDebug("redis connect successed[%d] select index[%d]!! \n",
			m_RedisCtx->err, m_uConnectCount );

	m_strIP = ip;
	m_uPort = port;
	m_strPassWord = strPwd;
	
	return true;
	
}

bool CRedisBase::SetTimeOut( Int32 timeout )
{
    if ( !m_RedisCtx || timeout == -1 )
    {
        LogWarn("redis m_RedisCtx is null or timeout == -1 !!! \n");
		return false;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000*timeout;
    redisSetTimeout(m_RedisCtx, tv);
    return true;
}

bool CRedisBase::getRedisServAddr(std::string &strIP, UInt32 &uPort)
{
	strIP = m_strIP;
	uPort = m_uPort;
	return true;
}

UInt64 CRedisBase::getSequence()
{
	return m_uSequence;
}

redisReply* CRedisBase::execSyncCmd( const char* redisCmd , UInt32 uRetranCnt )
{	
	redisReply *pReply = NULL;	
	UInt32      uExecCnt = 0;

	if( m_RedisCtx )
	{		
		if( ++m_uSequence > 10000000 )
		{
			m_uSequence = 0;
		}

		do
		{
			pReply = (redisReply *)redisCommand( m_RedisCtx, redisCmd );
			if( pReply != NULL ){
				break;
			}
			
			uExecCnt++;
		}while( uExecCnt < uRetranCnt );
	}

	return pReply;

}

Int32 CRedisBase::getReply(void **redisReply )
{
	if( ! m_RedisCtx ){
		return REDIS_ERR;
	}
	return redisGetReply( m_RedisCtx, redisReply );
}

Int32 CRedisBase::appendCmd(const char* redisCmd )
{
	if( !m_RedisCtx ){
		return REDIS_ERR;
	}
	return redisAppendCommand( m_RedisCtx, redisCmd );
}

bool CRedisBase::checkConnState( bool bSendPing )
{
	redisReply* reply = NULL;
	int ret = true;

	if(! m_RedisCtx )
	{
		return false;
	}

	if( bSendPing )
	{
		reply = (redisReply *)redisCommand( m_RedisCtx, "PING");
		if( NULL == reply || reply->str == NULL 
			|| 0 != strncasecmp( reply->str, "PONG", strlen("PONG")))
		{
			LogWarn(" redis ping fail, re connect!! ");
			ret = reConn();
		}
		else
		{
			freeReplyObject(reply);
		}
	}

	return ret;	
}

bool CRedisBase::reConn()
{
	return Close() && Conn( m_strIP, m_uPort, m_strPassWord );
}

bool CRedisBase::Close()
{	
	if( m_RedisCtx )
	{
		redisFree( m_RedisCtx );
		m_RedisCtx = NULL;
	}
	return true;
}
