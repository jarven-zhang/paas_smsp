#include "SMGPSocketHandler.h"
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


SMGPSocketHandler::SMGPSocketHandler(CThread* pThread, string strLinkId)
{
    m_socket = NULL;
    m_strLinkId = strLinkId;
    m_pThread = pThread;
    m_iLinkState = LINK_INIT;
    m_strAccount = "";
}

SMGPSocketHandler::~SMGPSocketHandler()
{
    if(NULL != m_socket)
    {
        m_socket->Close();
        delete m_socket;
    }
}

bool SMGPSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
    InternalSocket *pSocket = new InternalSocket();
    if (false == pSocket->Init(service, socket, address, m_pThread->m_pLinkedBlockPool))
    {
        LogError("SMGPSocketHandler::Init is failed.");
        delete pSocket;
        return false;
    }
    m_strClientIP = address.GetInetAddress();
    pSocket->SetHandler(this);
    m_socket = pSocket;
    return true;
}

void SMGPSocketHandler::OnEvent(int type, InternalSocket *socket)
{
    if (type == EventType::Connected)
    {
        LogDebug("Connected");
    }
    else if (type == EventType::Read)
    {
        ////if(m_iLinkState != LINK_CLOSE)
        OnData();
    }
    else if (type == EventType::Closed)
    {
        LogError("SMGP account[%s],link[%s] close it!",m_strAccount.data(),m_strLinkId.data());
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_SMGP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Error)
    {
        LogError("SMGP account[%s],link[%s] error it!",m_strAccount.data(),m_strLinkId.data());
        CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
        req->m_strAccount = m_strAccount;
        req->m_strLinkId = m_strLinkId;
        req->m_iMsgType = MSGTYPE_SMGP_LINK_CLOSE_REQ;
        m_pThread->PostMsg(req);
    }
}

void SMGPSocketHandler::OnData()
{
    while (ParseSMGPData())
    {
    }
}

bool SMGPSocketHandler::ParseSMGPData()
{

    smsp::SMGPBase resp;
    pack::Reader reader(m_socket->In());

    if(reader.GetSize() < 12)
    {
        reader.SetGood(false);
        return reader;
    }

    if (resp.Unpack(reader))
    {
        if(reader.GetSize() < resp.packetLength_ - 12)
        {
            reader.SetGood(false);
            return reader;
        }

        if(resp.requestId_ == SMGP_LOGIN)
        {
            LogDebug("SMGP_LOGIN commandLength[%d],commandId:0x[%x],sequenceNum[%d]",resp.packetLength_,resp.requestId_,resp.sequenceId_);

            smsp::SMGPConnectReq reqConnect;
            if(reqConnect.Unpack(reader))
            {
                LogDebug("===SMGP connect account[%s],length[%d].",reqConnect.clientId_.data(),reqConnect.clientId_.length());
                m_strAccount = reqConnect.clientId_.data();
                CTcpLoginReqMsg* pMsg = new CTcpLoginReqMsg();
                pMsg->m_strLinkId = m_strLinkId;
                pMsg->m_strAccount = reqConnect.clientId_.data();
                pMsg->m_uSequenceNum = resp.sequenceId_;
                pMsg->m_uSmsFrom = SMS_FROM_ACCESS_SMGP;
                pMsg->m_uTimeStamp = reqConnect.timestamp_;
                pMsg->m_strAuthSource = reqConnect.authClient_.data();
                pMsg->m_strIp = m_strClientIP;
                pMsg->m_loginMode = reqConnect.loginMode_;
                pMsg->m_iMsgType = MSGTYPE_TCP_LOGIN_REQ;
//              CHK_ALL_THREAD_INIT_OK_RETURN_RET(reader);
                g_pUnitypThread->PostMsg(pMsg);
            }
            else
            {
                return reader;
            }
        }
        else if (resp.requestId_ == SMGP_SUBMIT)
        {
            LogDebug("SMGP_submit commandLength[%d],commandId:0x[%x],sequenceNum[%d]",resp.packetLength_,resp.requestId_,resp.sequenceId_);
            if (m_iLinkState != LINK_CONNECTED)
            {
                smsp::SMGPSubmitReq submitReq;
                submitReq.packetLength_ = resp.packetLength_;
                submitReq.Unpack(reader);
            }
            else
            {
                smsp::SMGPSubmitReq submitReq;
                submitReq.packetLength_ = resp.packetLength_;
                if(submitReq.Unpack(reader))
                {
                    CTcpSubmitReqMsg* pReq = new CTcpSubmitReqMsg();
                    pReq->m_vecPhone = submitReq.phonelist_;
                    pReq->m_strAccount.assign(m_strAccount);
                    pReq->m_strLinkId.assign(m_strLinkId);
                    pReq->m_strMsgContent.assign(submitReq.msgContent_);
                    pReq->m_strSrcId.assign(submitReq.srcTermId_);
                    pReq->m_uMsgFmt = submitReq.msgFormat_;
                    pReq->m_uMsgLength = submitReq.msgLength_;
                    pReq->m_uPkNumber = submitReq.pkNum_;
                    pReq->m_uPkTotal = submitReq.pkTotal_;
                    pReq->m_uRandCode = submitReq.randCode8_;
                    pReq->m_uSequenceNum = resp.sequenceId_;
                    pReq->m_uPhoneNum = submitReq.destTermIdCount_;
                    pReq->m_uSmsFrom = SMS_FROM_ACCESS_SMGP;

                    pReq->m_iMsgType = MSGTYPE_TCP_SUBMIT_REQ;
                    LogDebug("content:[%s]",pReq->m_strMsgContent.data());
                    LogDebug("m_uFlag = [%d]",submitReq.m_uFlag);
//                  CHK_ALL_THREAD_INIT_OK_RETURN_RET(reader);
                    g_pUnitypThread->PostMsg(pReq);
                }
                else
                {
                    return reader;
                }
            }
        }
        else if (resp.requestId_ == SMGP_DELIVER_RESP)
        {
            smsp::SMGPDeliverResp respDeliver;
            respDeliver.Unpack(reader);
        }
        else if (resp.requestId_ == SMGP_ACTIVE_TEST)
        {
            smsp::SMGPHeartbeatReq* reqHeartBeat = new smsp::SMGPHeartbeatReq();
            reqHeartBeat->packetLength_ = resp.packetLength_;
            reqHeartBeat->requestId_   = resp.requestId_;
            reqHeartBeat->sequenceId_  = resp.sequenceId_;

            TSMGPHeartBeatReq* req = new TSMGPHeartBeatReq();
            req->m_reqHeartBeat = reqHeartBeat;
            req->m_socketHandle = this;
            req->m_strSessionID = m_strLinkId;
            req->m_iMsgType = MSGTYPE_SMGP_HEART_BEAT_REQ;
            m_pThread->PostMsg(req);
        }
        else if (resp.requestId_ == SMGP_ACTIVE_TEST_RESP)
        {
            smsp::SMGPHeartbeatResp* respHeartBeat = new smsp::SMGPHeartbeatResp();
            respHeartBeat->packetLength_ = resp.packetLength_;
            respHeartBeat->requestId_   = resp.requestId_;
            respHeartBeat->sequenceId_  = resp.sequenceId_;

            TSMGPHeartBeatResp* req = new TSMGPHeartBeatResp();
            req->m_respHeartBeat = respHeartBeat;
            req->m_socketHandle = this;
            req->m_strSessionID = m_strLinkId;
            req->m_iMsgType = MSGTYPE_SMGP_HEART_BEAT_RESP;
            m_pThread->PostMsg(req);

        }
        else if (resp.requestId_ == SMGP_TERMINATE)
        {
            LogDebug("link[%s] recv SMGP_TERMINATE.", m_strLinkId.data());
            smsp::SMGPTerminateResp respTerminate;
            respTerminate.sequenceId_  = resp.sequenceId_;
            pack::Writer writer(m_socket->Out());
            respTerminate.Pack(writer);
        }
        else if (resp.requestId_ == SMGP_TERMINATE_RESP)
        {
            LogWarn("SMGP_TERMINATE_RESP commandId:0x[%x],sequenceNum[%d], strlink[%s], pacLen[%u]"
                ,resp.requestId_, resp.sequenceId_, m_strLinkId.c_str(), resp.packetLength_);
            if (resp.packetLength_ > 12)
            {
                char* buffer = new char[resp.packetLength_ - 12];
                reader(resp.packetLength_ - 12, buffer);
                delete[] buffer;
            }
        }
        else if( resp.requestId_ == SMGP_QUERY || resp.requestId_ == SMGP_QUERY || resp.requestId_ == Forward ||
            resp.requestId_ == Query_TE_Route || resp.requestId_ == Query_SP_Route || resp.requestId_ == Payment_Request||
            resp.requestId_ == Payment_Affirm || resp.requestId_ == Query_UserState ||resp.requestId_ == Get_All_TE_Route ||
            resp.requestId_ == Get_All_SP_Route || resp.requestId_ == Update_TE_Route ||resp.requestId_ == Update_SP_Route||
            resp.requestId_ == Push_Update_TE_Route||resp.requestId_ ==Push_Update_SP_Route)
        {
            LogError("unsupported commandId[%d], discard it.", resp.requestId_);
            char* buffer = new char[resp.packetLength_ - 12];
            reader(resp.packetLength_ - 12, buffer);
            delete[] buffer;
        }
        else
        {
            LogDebug("error commandId[%d], Close link[%s].", resp.requestId_, m_strLinkId.data());
            CTcpCloseLinkReqMsg* req = new CTcpCloseLinkReqMsg();
            req->m_strAccount = m_strAccount;
            req->m_strLinkId = m_strLinkId;
            req->m_iMsgType = MSGTYPE_SMGP_LINK_CLOSE_REQ;
            m_pThread->PostMsg(req);
        }

    }

    return reader;

}
