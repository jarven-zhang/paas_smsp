#include "SMPPSocketHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include "UrlCode.h"
#include "propertyutils.h"
#include "main.h"

using namespace std;


SMPPSocketHandler::SMPPSocketHandler(CThread* pThread, string strLinkId)
{
	m_socket = NULL;
	m_strLinkId = strLinkId;
	m_pThread = pThread;
	m_iLinkState = LINK_INIT;
    m_strAccount = "";
}

SMPPSocketHandler::~SMPPSocketHandler()
{
	if(NULL != m_socket)
	{
		m_socket->Close();
		delete m_socket;
	}
}

bool SMPPSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
	InternalSocket *pSocket = new InternalSocket();
	if (false == pSocket->Init(service, socket, address, m_pThread->m_pLinkedBlockPool))
	{
        LogError("******CMPPSocketHandler::Init is failed.");
	    delete pSocket;
		return false;
	}
	m_strClientIP = address.GetInetAddress();
	pSocket->SetHandler(this);
    m_socket = pSocket;
	return true;
}

void SMPPSocketHandler::OnEvent(int type, InternalSocket *socket)
{
	if (type == EventType::Connected)
	{
		LogDebug("Connected");
	}
	else if (type == EventType::Read)
	{
		OnData();
	}
	else if (type == EventType::Closed)
	{
        LogError("==err== smpp account[%s],link[%s] close it!",m_strAccount.data(),m_strLinkId.data());
		CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_SMPP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
	}
	else if (type == EventType::Error)
	{
		LogError("==err== smpp account[%s],link[%s] error it!",m_strAccount.data(),m_strLinkId.data());
		CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_SMPP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
	}
}

void SMPPSocketHandler::OnData()
{
	while (ParseSMPPData())
	{
	}
}

bool SMPPSocketHandler::ParseSMPPData()
{

	SMPPBase resp;
	pack::Reader reader(m_socket->In());

	if(reader.GetSize() < 16)
	{
        reader.SetGood(false);
    	return reader;
    }

	if (resp.Unpack(reader))
	{
		if(reader.GetSize() < (resp._commandLength - 16))
		{
            LogWarn("reader size[%d] is little content[%d].",reader.GetSize(),resp._commandLength - 16);
            reader.SetGood(false);
        	return reader;
        }

		if(resp._commandId == BIND_TRANSCEIVER)
		{
            LogDebug("Bind_transceive commandLength[%d],commandStatus[%d],commandId:0x[%x],sequenceNum[%d]",
                resp._commandLength,resp._commandStatus,resp._commandId,resp._sequenceNum);

			/////smsp::SMPPConnectReq* reqConnect = new smsp::SMPPConnectReq(resp);
			smsp::SMPPConnectReq reqConnect(resp);
			if(reqConnect.Unpack(reader))
			{
                m_strAccount = reqConnect._systemId;
				CTcpLoginReqMsg* pMsg = new CTcpLoginReqMsg();
                pMsg->m_strAccount = reqConnect._systemId;
                pMsg->m_strPwd = reqConnect._password;
                pMsg->m_strLinkId = m_strLinkId;
                pMsg->m_uSequenceNum = resp._sequenceNum;
                pMsg->m_uSmsFrom = SMS_FROM_ACCESS_SMPP;
                pMsg->m_strIp = m_strClientIP;
                pMsg->m_iMsgType = MSGTYPE_TCP_LOGIN_REQ;
//				CHK_ALL_THREAD_INIT_OK_RETURN_RET(reader);
                g_pUnitypThread->PostMsg(pMsg);
			}
			else
			{
				return reader;
			}
		}
		else if (resp._commandId == SUBMIT_SM)
		{
            LogDebug("Submit_sm commandLength[%d],commandStatus[%d],commandId:0x[%x],sequenceNum[%d]",resp._commandLength,resp._commandStatus,resp._commandId,resp._sequenceNum);
            if (m_iLinkState != LINK_CONNECTED)
            {
                smsp::SMPPSubmitReq submitReq(resp);
                submitReq.Unpack(reader);
            }
            else
            {
                smsp::SMPPSubmitReq submitReq(resp);
    			if(submitReq.Unpack(reader))
    			{
                    CTcpSubmitReqMsg* pReq = new CTcpSubmitReqMsg();

                    pReq->m_strAccount.assign(m_strAccount);
                    pReq->m_strLinkId.assign(m_strLinkId);

                    pReq->m_vecPhone = submitReq._phonelist;
                    pReq->m_strMsgContent.assign(submitReq._shortMessage);
                    ////pReq->m_strPhone.assign(submitReq._phone);
                    pReq->m_strSrcId.assign(submitReq._sourceAddr);
                    pReq->m_uMsgFmt = submitReq._dataCoding;
                    pReq->m_uMsgLength = submitReq._smLength;
                    pReq->m_uPkNumber = submitReq._pkNumber;
                    pReq->m_uPkTotal = submitReq._pkTotal;
                    pReq->m_uRandCode = submitReq._randeCode;
                    pReq->m_uSequenceNum = resp._sequenceNum;
                    pReq->m_uPhoneNum = 1;
                    pReq->m_uSmsFrom = SMS_FROM_ACCESS_SMPP;

					pReq->m_uDestAddrTon = submitReq._destAddrTon;
					pReq->m_uDestAddrNpi = submitReq._destAddrNpi;

					pReq->m_uSourceAddrTon = submitReq._sourceAddrTon;
					pReq->m_uSourceAddrNpi = submitReq._sourceAddrNpi;

                    pReq->m_iMsgType = MSGTYPE_TCP_SUBMIT_REQ;
//					CHK_ALL_THREAD_INIT_OK_RETURN_RET(reader);
                    g_pUnitypThread->PostMsg(pReq);
    			}
    			else
    			{
    				return reader;
    			}
            }
		}
		else if (resp._commandId == DELIVER_SM_RESP)
		{
            LogDebug("Submit_sm commandLength[%d],commandStatus[%d],commandId:0x[%x],sequenceNum[%d]",
                resp._commandLength,resp._commandStatus,resp._commandId,resp._sequenceNum);

			smsp::SMPPDeliverResp respDeliver(resp);
			respDeliver.Unpack(reader);
		}
		else if (resp._commandId == ENQUIRE_LINK)
		{
			smsp::SMPPHeartbeatReq* reqHeartBeat = new smsp::SMPPHeartbeatReq();
			reqHeartBeat->_commandLength = resp._commandLength;
			reqHeartBeat->_commandId   = resp._commandId;
			reqHeartBeat->_sequenceNum  = resp._sequenceNum;

			TSMPPHeartBeatReq* req = new TSMPPHeartBeatReq();
			req->m_reqHeartBeat = reqHeartBeat;
			req->m_socketHandle = this;
			req->m_strSessionID = m_strLinkId;
			req->m_iMsgType = MSGTYPE_SMPP_HEART_BEAT_REQ;
			m_pThread->PostMsg(req);
		}
		else if (resp._commandId == ENQUIRE_LINK_RESP)
		{
			smsp::SMPPHeartbeatResp* respHeartBeat = new smsp::SMPPHeartbeatResp();
			TSMPPHeartBeatResp* req = new TSMPPHeartBeatResp();
			req->m_respHeartBeat = respHeartBeat;
			req->m_socketHandle = this;
			req->m_strSessionID = m_strLinkId;
			req->m_iMsgType = MSGTYPE_SMPP_HEART_BEAT_RESP;
			m_pThread->PostMsg(req);
		}
		else if (resp._commandId == GENERIC_NACK)
		{
            LogError("Generic_nack commandLength[%d],commandStatus[%d],commandId:0x[%x],sequenceNum[%d]",
                resp._commandLength,resp._commandStatus,resp._commandId,resp._sequenceNum);

            CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
            req->m_strAccount = m_strAccount;
            req->m_strLinkId = m_strLinkId;
            req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
            m_pThread->PostMsg(req);
		}
		else if( resp._commandId == BIND_RECEIVER || resp._commandId == BIND_TRANSMITTER
				|| resp._commandId == QUERY_SM || resp._commandId == UNBIND
				|| resp._commandId == REPLACE_SM	|| resp._commandId == CANCEL_SM
				|| resp._commandId == OUTBIND || resp._commandId == SUBMIT_MULTI
				|| resp._commandId == DATA_SM	|| resp._commandId == DATA_SM_RESP)
		{
			LogError("not support commandLength[%d],commandStatus[%d],commandId:0x[%x],sequenceNum[%d]",
                resp._commandLength,resp._commandStatus,resp._commandId,resp._sequenceNum);

            char* buffer = new char[resp._commandLength - 16];
            reader(resp._commandLength - 16, buffer);
            delete[] buffer;

            smsp::SMPPGeneric genericReq;
    		genericReq._sequenceNum = resp._sequenceNum;
    		pack::Writer writer(m_socket->Out());
    		genericReq.Pack(writer);
		}
		else if (resp._commandId == UNBIND_RESP)
        {
            LogWarn("UNBIND_RESP commandId:0x[%x],sequenceNum[%d], strlink[%s], totalLen[%u]"
                ,resp._commandId, resp._sequenceNum, m_strLinkId.c_str(), resp._commandLength);
            if (resp._commandLength > 16)
            {
                char* buffer = new char[resp._commandLength - 16];
                reader(resp._commandLength - 16, buffer);
                delete[] buffer;
            }
        }
		else
		{
			LogDebug("commandId:0x[%x] is not support, Close link[%s].", resp._commandId, m_strLinkId.data());

            smsp::SMPPGeneric genericReq;
    		genericReq._sequenceNum = 0;
    		pack::Writer writer(m_socket->Out());
    		genericReq.Pack(writer);

			CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
            req->m_strAccount = m_strAccount;
            req->m_strLinkId = m_strLinkId;
            req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
            m_pThread->PostMsg(req);
		}
	}

	return reader;

}
