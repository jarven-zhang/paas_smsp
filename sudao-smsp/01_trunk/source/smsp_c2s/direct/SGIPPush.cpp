#include "SGIPPush.h"
#include "UTFString.h"
#include <sstream>
#include <cwctype>
#include <algorithm>
#include "main.h"
#include "base64.h"

CSgipPush::CSgipPush()
{
    m_pSocket = NULL;
    m_uState = CONNECT_STATUS_INIT;
    m_ExpireTimer = NULL;
}

CSgipPush::~CSgipPush()
{
}

void CSgipPush::destroy()
{
    for (vector<TMsg*>::iterator iter = m_vecRtAndMo.begin(); iter != m_vecRtAndMo.end(); ++iter)
    {	
    	TMsg* msg= *iter;
    	if(msg->m_iMsgType == MSGTYPE_SGIP_RT_REQ)
    	{
			//report
			//report cant send to user. save to redis.  //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*
			CSgipRtReqMsg* pRt = (CSgipRtReqMsg*)(msg);
			LogDebug("can not find connect to sgipRpt server. save cache to Redis");
			

			//setRedis
			string strmsgid = Comm::int2str(pRt->m_uSequenceIdNode) + "|" + Comm::int2str(pRt->m_uSequenceIdTime) + "|" + Comm::int2str(pRt->m_uSequenceId);
			
			string strKey = "report_cache:" + pRt->m_strClientId;
			string strCmd = "lpush " + strKey + " clientid=" + pRt->m_strClientId +"&msgid=" + strmsgid +"&status=" + Comm::int2str(pRt->m_uState) 
				+ "&remark=" + Base64::Encode(Comm::int2str(pRt->m_uErrorCode)) + "&phone=" + pRt->m_strPhone+ "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_SGIP);
			
			TRedisReq* pRedis = new TRedisReq();
	        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
	        pRedis->m_RedisCmd.assign(strCmd);
			LogNotice("cmd[%s]", strCmd.c_str());
	        pRedis->m_strKey = strKey;
	        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

			//set expire
			TRedisReq* pRedisExpire = new TRedisReq();
	        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
	        pRedisExpire->m_RedisCmd = "EXPIRE " + strKey + " " +"259200";	//259200 = 72*3600
			LogNotice("expire cmd[%s]", pRedisExpire->m_RedisCmd.c_str());
	        pRedisExpire->m_strKey = strKey;
			SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);
			
		}
		else if(msg->m_iMsgType == MSGTYPE_SGIP_MO_REQ)
		{
			//mo 
			//mo cant send to user. save to redis 		//lpush report_cache:clientId clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*
			CSgipMoReqMsg* pRt = (CSgipMoReqMsg*)(msg);
			LogDebug("can not find connect to sgipRpt server. save mo cache to Redis");

			//setRedis	
			string strKey = "report_cache:" + pRt->m_strClientId;
			string strCmd = "lpush " + strKey + " clientid=" + pRt->m_strClientId +"&ismo=1&destid=" + pRt->m_strSpNum + "&phone=" + pRt->m_strPhone + 
			"&content=" + Base64::Encode(pRt->m_strContent) + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_SGIP);
			

			TRedisReq* pRedis = new TRedisReq();
	        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
	        pRedis->m_RedisCmd.assign(strCmd);
			LogNotice("cmd[%s]", strCmd.c_str());
	        pRedis->m_strKey= strKey;
			SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

			//set expire
			TRedisReq* pRedisExpire = new TRedisReq();
	        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
	        pRedisExpire->m_RedisCmd = "EXPIRE " + strKey + " " +"259200";	//259200 = 72*3600
			LogNotice("expire cmd[%s]", pRedisExpire->m_RedisCmd.c_str());
	        pRedisExpire->m_strKey = strKey;
			SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);
			
		}
		
        delete *iter;
    }
    m_vecRtAndMo.clear();

	
    
    m_uState = CONNECT_STATUS_NO;
	if(m_pSocket){
    	m_pSocket->Close();
    	delete m_pSocket;
		m_pSocket = NULL;
	}
    delete this;
}


void CSgipPush::onConnResp(pack::Reader& reader, SGIPBase& resp)
{
	LogDebug("sgip onConnResp");
	SGIPConnectResp respCon(resp);
	if (respCon.Unpack(reader))
    {

		if (respCon.isLogin())
        {
			m_uState = CONNECT_STATUS_OK;
            LogNotice("SGIP clientid[%s] Login success.",m_strClientId.data());
            for (vector<TMsg*>::iterator iter = m_vecRtAndMo.begin(); iter != m_vecRtAndMo.end(); ++iter)
            {
                g_pSgipRtAndMoThread->PostMsg(*iter);
            }
            m_vecRtAndMo.clear();

			//post msg to g_pRptMoGetThread
			TMsg* msg = new TMsg();
			msg->m_iMsgType = MSGTYPE_SGIP_CLIENT_LOGIN_SUC;
			msg->m_strSessionID = m_strClientId;
			g_pRptMoGetThread->PostMsg(msg);
			
		} 
        else
        {
			LogError("SGIP clientid[%s] Login failed.",m_strClientId.data());
			m_uState = CONNECT_STATUS_NO;
		}
	}
}

void CSgipPush::init(string& strIp,UInt16 uPort,string& strClientId,string& strPwd)
{
    m_strIp.assign(strIp);
    m_uPort = uPort;
    m_strClientId.assign(strClientId);
    m_strPwd.assign(strPwd);
	conn();
}

void CSgipPush::conn() 
{
    if (NULL != m_pSocket)
    {
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }

    m_pSocket = new InternalSocket();
	if(NULL == m_pSocket){
		LogError("new mem error!!");
		return;
	}
    m_pSocket->Init(g_pSgipRtAndMoThread->m_pInternalService,g_pSgipRtAndMoThread->m_pLinkedBlockPool);

    m_pSocket->SetHandler(this);

    LogNotice("===sgip=== SgipPush::connect server [%s:%d] accout[%s]", m_strIp.data(), m_uPort,m_strClientId.data());
    
    Address addr(m_strIp, m_uPort);
    m_pSocket->Connect(addr, 10000);
    SGIPConnectReq reqLogin;
    reqLogin.sequenceId_ = g_pSgipRtAndMoThread->m_SnManager.getDirectSequenceId();
    m_uState = CONNECT_STATUS_INIT;
    reqLogin.setAccout(m_strClientId, m_strPwd);
    pack::Writer writer(m_pSocket->Out());
    reqLogin.Pack(writer);
}

void CSgipPush::disConn()
{
	SGIPUnBindReq req;
	if(NULL == m_pSocket){
		LogError("psocket is null!!");
		return;
	}
	pack::Writer writer(m_pSocket->Out());
	req.Pack(writer);
	m_uState = CONNECT_STATUS_NO;
}

void CSgipPush::reConn()
{
	conn();
}

bool CSgipPush::onTransferData()
{
    SGIPBase resp;
	if(NULL == m_pSocket){
		LogError("psocket is null!!");
		return false;
	}
    pack::Reader reader(m_pSocket->In());
    if(reader.GetSize() < 20)
	{
        reader.SetGood(false);
    	return reader;
    }

    if (resp.Unpack(reader)) 
    {
        if(reader.GetSize() < resp.packetLength_ - 20)
		{
            reader.SetGood(false);
        	return reader;
        }

        if (SGIP_BIND_RESP == resp.requestId_)
        {
            LogDebug("sgip onConnResp");
        	SGIPConnectResp respCon(resp);
        	if (respCon.Unpack(reader))
            {
        		if (respCon.isLogin())
                {
        			m_uState = CONNECT_STATUS_OK;
                    LogNotice("SGIP clientid[%s] Login success.",m_strClientId.data());
                    for (vector<TMsg*>::iterator iter = m_vecRtAndMo.begin(); iter != m_vecRtAndMo.end(); ++iter)
                    {
                        g_pSgipRtAndMoThread->PostMsg(*iter);
                    }
                    m_vecRtAndMo.clear();
        		} 
                else
                {
        			LogError("SGIP clientid[%s] Login failed.",m_strClientId.data());
        			m_uState = CONNECT_STATUS_NO;
        		}
        	}
        }
        else if ((SGIP_UNBIND == resp.requestId_) || (SGIP_UNBIND_RESP == resp.requestId_))
        {
            LogError("sgip command:0x[%x] is unbind.",resp.requestId_);
            CSgipCloseReqMsg* pClose = new CSgipCloseReqMsg();
            pClose->m_strClientId.assign(m_strClientId);
            pClose->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
            g_pSgipRtAndMoThread->PostMsg(pClose);
            
        }
        else if ((SGIP_REPORT_RESP == resp.requestId_) || (SGIP_DELIVER_RESP == resp.requestId_))
        {
            LogDebug("sgip commandId:0x[%x],not analyze.", resp.requestId_);
            char* buffer = new char[resp.packetLength_ - 20];
            reader(resp.packetLength_ - 20, buffer);
            delete[] buffer;
        }
        else
        {
            LogError("sgip command:0x[%x] is invalied.",resp.requestId_);
            CSgipCloseReqMsg* pClose = new CSgipCloseReqMsg();
            pClose->m_strClientId.assign(m_strClientId);
            pClose->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
            g_pSgipRtAndMoThread->PostMsg(pClose);
        }
    }
    else
    {
        LogError("analyze sgip base is failed.");
    }

    return reader;

}

void CSgipPush::OnEvent(int type, InternalSocket *socket)
{
    if (socket != m_pSocket)
    {
        return;
    }
    if (type == EventType::Connected)
    {
        LogDebug("SGIPChannel::Connected");
    }
    else if (type == EventType::Read)
    {
        onData();
    }
    else if (type == EventType::Closed)
    {
        LogError("SGIPChannel::Close");
        m_uState = CONNECT_STATUS_INIT;
        CSgipCloseReqMsg* pClose = new CSgipCloseReqMsg();
        pClose->m_strClientId.assign(m_strClientId);
        pClose->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
        g_pSgipRtAndMoThread->PostMsg(pClose);
    }
    else if (type == EventType::Error)
    {
        LogError("SGIPChannel::Error");
        m_uState = CONNECT_STATUS_INIT;
        CSgipCloseReqMsg* pClose = new CSgipCloseReqMsg();
        pClose->m_strClientId.assign(m_strClientId);
        pClose->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
        g_pSgipRtAndMoThread->PostMsg(pClose);
    }

}

void CSgipPush::onData() 
{
	while (onTransferData()) 
	{
        
	}
}