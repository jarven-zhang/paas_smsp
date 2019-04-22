#include "SGIPSocketHandler.h"
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


SGIPSocketHandler::SGIPSocketHandler(CThread* pThread, string strLinkId)
{
    m_socket = NULL;
    m_strLinkId = strLinkId;
    m_pThread = pThread;
    m_iLinkState = LINK_INIT;
    m_strAccount = "";
}

SGIPSocketHandler::~SGIPSocketHandler()
{
    if(NULL != m_socket)
    {
        m_socket->Close();
        delete m_socket;
    }
}

bool SGIPSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
    InternalSocket *pSocket = new InternalSocket();
    if (false == pSocket->Init(service, socket, address, m_pThread->m_pLinkedBlockPool))
    {
        LogError("SGIPSocketHandler::Init is failed.");
        delete pSocket;
        return false;
    }
    m_strClientIP = address.GetInetAddress();
    pSocket->SetHandler(this);
    m_socket = pSocket;
    return true;
}

void SGIPSocketHandler::OnEvent(int type, InternalSocket *socket)
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
        LogError("sgip account[%s],link[%s] close it!",m_strAccount.data(),m_strLinkId.data());
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Error)
    {
        LogError("sgip account[%s],link[%s] error it!",m_strAccount.data(),m_strLinkId.data());
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
    }
}

void SGIPSocketHandler::OnData()
{
    while (ParseCMPPData())
    {
    }
}

bool SGIPSocketHandler::ParseCMPPData()
{

    SGIPBase resp;
    pack::Reader reader(m_socket->In());

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

        if(resp.requestId_ == SGIP_BIND)
        {
            LogDebug("sgip_connect commandLength[%d],commandId:0x[%x]",resp.packetLength_,resp.requestId_);

            SGIPConnectReq bindReq;
            if(bindReq.Unpack(reader))
            {
                LogDebug("===sgip connect account[%s],length[%d].",bindReq.loginName_.data(),bindReq.loginName_.length());

                m_strAccount.assign(bindReq.loginName_.data());

                CTcpLoginReqMsg* pMsg = new CTcpLoginReqMsg();
                pMsg->m_strLinkId = m_strLinkId;
                pMsg->m_strAccount = bindReq.loginName_.data();
                pMsg->m_strSgipPassWd = bindReq.loginPassword_.data();
                pMsg->m_uSmsFrom = SMS_FROM_ACCESS_SGIP;
                pMsg->m_uSequenceIdNode = resp.sequenceIdNode_;
                pMsg->m_uSequenceIdTime = resp.sequenceIdTime_;
                pMsg->m_uSequenceId = resp.sequenceId_;
                pMsg->m_strIp = m_strClientIP;
                pMsg->m_iMsgType = MSGTYPE_TCP_LOGIN_REQ;
//              CHK_ALL_THREAD_INIT_OK_RETURN_RET(reader);
                g_pUnitypThread->PostMsg(pMsg);
            }
            else
            {
                return reader;
            }
        }
        else if (resp.requestId_ == SGIP_SUBMIT)
        {
            LogDebug("sgip_submit commandLength[%d],commandId:0x[%x]",resp.packetLength_,resp.requestId_);
            if (m_iLinkState != LINK_CONNECTED)
            {
                SGIPSubmitReq submitReq;
                submitReq.Unpack(reader);
            }
            else
            {
                SGIPSubmitReq submitReq;
                if(submitReq.Unpack(reader))
                {
                    CTcpSubmitReqMsg* pReq = new CTcpSubmitReqMsg();
                    pReq->m_vecPhone = submitReq._phonelist;
                    pReq->m_strAccount.assign(m_strAccount);
                    pReq->m_strLinkId.assign(m_strLinkId);
                    pReq->m_strMsgContent.assign(submitReq.msgContent_);
                    pReq->m_strSrcId.assign(submitReq.spNum_.data());
                    pReq->m_uMsgFmt = submitReq.msgCoding_;
                    pReq->m_uMsgLength = submitReq.msgLength_;
                    pReq->m_uPkNumber = submitReq._pkNumber;
                    pReq->m_uPkTotal = submitReq._pkTotal;
                    pReq->m_uRandCode = submitReq._randCode8;
                    pReq->m_uPhoneNum = submitReq.userCount_;

                    pReq->m_uSequenceIdNode = resp.sequenceIdNode_;
                    pReq->m_uSequenceIdTime = resp.sequenceIdTime_;
                    pReq->m_uSequenceId = resp.sequenceId_;

                    pReq->m_uSmsFrom = SMS_FROM_ACCESS_SGIP;
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
        else if (resp.requestId_ == SGIP_UNBIND)
        {
            LogError("unbind sgip account[%s],link[%s] close it!",m_strAccount.data(),m_strLinkId.data());
            CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
            req->m_strAccount = m_strAccount;
            req->m_strLinkId = m_strLinkId;
            req->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
            m_pThread->PostMsg(req);
        }
        else if((resp.requestId_ == SGIP_UNBIND_RESP)
            || (resp.requestId_ == SGIP_SUBMIT_RESP)
            || (resp.requestId_ == SGIP_REPORT)
            || (resp.requestId_ == SGIP_REPORT_RESP)
            || (resp.requestId_ == SGIP_DELIVER)
            || (resp.requestId_ == SGIP_DELIVER_RESP)
            || (resp.requestId_ == SGIP_BIND_RESP))
        {
            LogError("sgip unsupported commandId:0x[%x], discard it.", resp.requestId_);
            if (resp.packetLength_ > 20)
            {
                char* buffer = new char[resp.packetLength_ - 20];
                reader(resp.packetLength_ - 20, buffer);
                delete[] buffer;
            }
        }
        else
        {
            LogDebug("===sgip== error commandId:0x[%x], Close link[%s].", resp.requestId_, m_strLinkId.data());
            CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
            req->m_strAccount = m_strAccount;
            req->m_strLinkId = m_strLinkId;
            req->m_iMsgType = MSGTYPE_SGIP_LINK_CLOSE_REQ;
            m_pThread->PostMsg(req);
        }

    }

    return reader;

}

