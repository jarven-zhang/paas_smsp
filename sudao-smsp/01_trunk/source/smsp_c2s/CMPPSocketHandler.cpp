#include "CMPPSocketHandler.h"
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


CMPPSocketHandler::CMPPSocketHandler(CThread* pThread, string strLinkId)
{
    m_socket = NULL;
    m_strLinkId = strLinkId;
    m_pThread = pThread;
    m_iLinkState = LINK_INIT;
    m_strAccount = "";
}

CMPPSocketHandler::~CMPPSocketHandler()
{
    if(NULL != m_socket)
    {
        m_socket->Close();
        delete m_socket;
    }
}

bool CMPPSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
    InternalSocket *pSocket = new InternalSocket();
    if (false == pSocket->Init(service, socket, address, m_pThread->m_pLinkedBlockPool))
    {
        LogError("CMPPSocketHandler::Init is failed.");
        delete pSocket;
        return false;
    }
    m_strClientIP = address.GetInetAddress();
    pSocket->SetHandler(this);
    m_socket = pSocket;
    return true;
}

void CMPPSocketHandler::OnEvent(int type, InternalSocket *socket)
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
        LogError("cmpp account[%s],link[%s] close it!",m_strAccount.data(),m_strLinkId.data());
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Error)
    {
        LogError("cmpp account[%s],link[%s] error it!",m_strAccount.data(),m_strLinkId.data());
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
    }
}

void CMPPSocketHandler::OnData()
{
    while (ParseCMPPData())
    {
    }
}

bool CMPPSocketHandler::ParseCMPPData()
{

    smsp::CMPPBase resp;
    pack::Reader reader(m_socket->In());

    if(reader.GetSize() < 12)
    {
        reader.SetGood(false);
        return reader;
    }

    if (resp.Unpack(reader))
    {
        if(reader.GetSize() < resp._totalLength - 12)
        {
            reader.SetGood(false);
            return reader;
        }

        if(resp._commandId == CMPP_CONNECT)
        {
            /////LogDebug("cmpp_connect commandLength[%d],commandId:0x[%x],sequenceNum[%d]",resp._totalLength,resp._commandId,resp._sequenceId);

            smsp::CMPPConnectReq reqConnect;
            if(reqConnect.Unpack(reader))
            {
                //LogNotice("===cmpp connect account[%s],length[%d].",reqConnect._sourceAddr.data(),reqConnect._sourceAddr.length());
                m_strAccount = reqConnect._sourceAddr;
                CTcpLoginReqMsg* pMsg = new CTcpLoginReqMsg();
                pMsg->m_strLinkId = m_strLinkId;
                pMsg->m_strAccount = reqConnect._sourceAddr;
                pMsg->m_uSequenceNum = resp._sequenceId;
                pMsg->m_uSmsFrom = SMS_FROM_ACCESS_CMPP;
                pMsg->m_uTimeStamp = reqConnect._timestamp;
                pMsg->m_strAuthSource = reqConnect._authSource;
                pMsg->m_strIp = m_strClientIP;
                pMsg->m_iMsgType = MSGTYPE_TCP_LOGIN_REQ;
                g_pUnitypThread->PostMsg(pMsg);
            }
            else
            {
                return reader;
            }
        }
        else if (resp._commandId == CMPP_SUBMIT)
        {
            //LogNotice("cmpp_submit commandLength[%d],commandId:0x[%x],sequenceNum[%d], account[%s]",resp._totalLength,resp._commandId,resp._sequenceId, m_strAccount.data());
            if (m_iLinkState != LINK_CONNECTED)
            {
                LogWarn("cmpplink error. commandLength[%d],commandId:0x[%x],sequenceNum[%d], account[%s]",resp._totalLength,resp._commandId,resp._sequenceId, m_strAccount.data());
                smsp::CmppSubmitReq submitReq;
                submitReq.Unpack(reader);
            }
            else
            {
                smsp::CmppSubmitReq submitReq;
                if(submitReq.Unpack(reader))
                {
                    CTcpSubmitReqMsg* pReq = new CTcpSubmitReqMsg();
                    pReq->m_vecPhone = submitReq._phonelist;
                    pReq->m_strAccount.assign(m_strAccount);
                    pReq->m_strLinkId.assign(m_strLinkId);
                    pReq->m_strMsgContent.assign(submitReq._msgContent);
                    pReq->m_strSrcId.assign(submitReq._srcId);
                    pReq->m_uMsgFmt = submitReq._msgFmt;
                    pReq->m_uMsgLength = submitReq._msgLength;
                    pReq->m_uPkNumber = submitReq._pkNumber;
                    pReq->m_uPkTotal = submitReq._pkTotal;
                    pReq->m_uRandCode = submitReq._randCode;
                    pReq->m_uSequenceNum = resp._sequenceId;
                    pReq->m_uPhoneNum = submitReq._destUsrTl;
                    pReq->m_uSmsFrom = SMS_FROM_ACCESS_CMPP;

                    pReq->m_iMsgType = MSGTYPE_TCP_SUBMIT_REQ;
//                  CHK_ALL_THREAD_INIT_OK_RETURN_RET(reader);
                    g_pUnitypThread->PostMsg(pReq);
                }
                else
                {
                    return reader;
                }
            }
        }
        else if (resp._commandId == CMPP_DELIVER_RESP)
        {
            smsp::CMPPDeliverResp respDeliver;
            respDeliver.Unpack(reader);
        }
        else if (resp._commandId == CMPP_ACTIVE_TEST)
        {
            smsp::CMPPHeartbeatReq* reqHeartBeat = new smsp::CMPPHeartbeatReq();
            reqHeartBeat->_totalLength = resp._totalLength;
            reqHeartBeat->_commandId   = resp._commandId;
            reqHeartBeat->_sequenceId  = resp._sequenceId;

            TCMPPHeartBeatReq* req = new TCMPPHeartBeatReq();
            req->m_reqHeartBeat = reqHeartBeat;
            req->m_socketHandle = this;
            req->m_strSessionID = m_strLinkId;
            req->m_iMsgType = MSGTYPE_CMPP_HEART_BEAT_REQ;
            m_pThread->PostMsg(req);
        }
        else if (resp._commandId == CMPP_ACTIVE_TEST_RESP)
        {
            smsp::CMPPHeartbeatResp* respHeartBeat = new smsp::CMPPHeartbeatResp();
            respHeartBeat->_totalLength = resp._totalLength;
            if(respHeartBeat->Unpack(reader))
            {
                TCMPPHeartBeatResp* req = new TCMPPHeartBeatResp();
                req->m_respHeartBeat = respHeartBeat;
                req->m_socketHandle = this;
                req->m_strSessionID = m_strLinkId;
                req->m_iMsgType = MSGTYPE_CMPP_HEART_BEAT_RESP;
                m_pThread->PostMsg(req);
            }
            else
            {
                delete respHeartBeat;
                respHeartBeat = NULL;
                return reader;
            }
        }
        else if (resp._commandId == CMPP_TERMINATE)
        {
            /////LogDebug("link[%s] recv CMPP_TERMINATE.", m_strLinkId.data());
            smsp::CMPPTerminateResp respTerminate;
            respTerminate._sequenceId  = resp._sequenceId;
            pack::Writer writer(m_socket->Out());
            respTerminate.Pack(writer);
        }
        else if (resp._commandId == CMPP_TERMINATE_RESP)
        {
            LogWarn("CMPP_TERMINATE_RESP commandId:0x[%x],sequenceNum[%d], strlink[%s], totallen[%u]"
                ,resp._commandId, resp._sequenceId, m_strLinkId.c_str(), resp._totalLength);
            if (resp._totalLength > 12)
            {
                char* buffer = new char[resp._totalLength - 12];
                reader(resp._totalLength - 12, buffer);
                delete[] buffer;
            }
           
        }
        else if( resp._commandId == CMPP_QUERY || resp._commandId == CMPP_CANCEL
                || resp._commandId == CMPP_FWD || resp._commandId == CMPP_MT_ROUTE
                || resp._commandId == CMPP_MO_ROUTE || resp._commandId == CMPP_GET_ROUTE
                || resp._commandId == CMPP_MT_ROUTE_UPDATE || resp._commandId == CMPP_MO_ROUTE_UPDATE
                || resp._commandId == CMPP_PUSH_MT_ROUTE_UPDATE || resp._commandId == CMPP_PUSH_MT_ROUTE_UPDATE
                || resp._commandId == CMPP_GET_MO_ROUTE )
        {
            LogError("unsupported commandId[%u], discard it.", resp._commandId);
            char* buffer = new char[resp._totalLength - 12];
            reader(resp._totalLength - 12, buffer);
            delete[] buffer;
        }
        else
        {
            LogError("error commandId[%u], Close link[%s].", resp._commandId, m_strLinkId.data());
            CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
            req->m_strAccount = m_strAccount;
            req->m_strLinkId = m_strLinkId;
            req->m_iMsgType = MSGTYPE_CMPP_LINK_CLOSE_REQ;
            m_pThread->PostMsg(req);
        }

    }

    return reader;

}
