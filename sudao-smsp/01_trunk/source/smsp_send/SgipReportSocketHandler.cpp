#include "SgipReportSocketHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "SGIPUnBindReq.h"
#include "SGIPUnBindResp.h"
#include "SGIPConnectReq.h"
#include "SGIPConnectResp.h"
#include "SGIPReportResp.h"
#include "SGIPReportReq.h"
#include "SGIPDeliverReq.h"
#include "SGIPDeliverResp.h"
#include "main.h"
#include "Channel.h"

using namespace std;

SgipReportSocketHandler::SgipReportSocketHandler(CThread* pThread, UInt64 uSeq)
{   
    m_pSocket = NULL;
    m_uSeq = uSeq;
    m_pThread = pThread;
    m_uChannelId = 0;
    m_uChannelType = 0;
}

SgipReportSocketHandler::~SgipReportSocketHandler()
{
    
}

bool SgipReportSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
    InternalSocket *pSocket = new InternalSocket();
    if (false == pSocket->Init(service, socket, address,m_pThread->m_pLinkedBlockPool))
    {
        LogError("******SgipReportSocketHandler::Init is failed.");
        delete pSocket;
        return false;
    }

    pSocket->SetHandler(this);
    m_pSocket = pSocket;
    return true;
}

void SgipReportSocketHandler::OnEvent(int type, InternalSocket *socket)
{   
    if (type == EventType::Connected) 
    {
    } 
    else if (type == EventType::Read) 
    {
        onData();
    } 
    else if (type == EventType::Closed) 
    {
        LogError("this is close.");
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    } 
    else if (type == EventType::Error)
    {
        LogError("this is error.");
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else 
    {
        LogError("this type[%d] is invalid.",type);
    }
}


bool SgipReportSocketHandler::onData()
{
    UInt32 uFlag = 0;
    while (onTransferData(uFlag))
    {

    }

    if (1 == uFlag)
    {
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    return true;
}

void SgipReportSocketHandler::Destroy()
{
    if(m_pSocket){
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }
}

bool SgipReportSocketHandler::checkLogin(string& strName,string& strpasswd)
{
    ////char pName[64] = {0};
    ////char pPassword[64] = {0};
    ////sprintf(pName,"%s",strName.data());
    ////sprintf(pPassword,"%s",strpasswd.data());
    string strNameEx = strName.data();
    string strPasswdEx = strpasswd.data();
    LogDebug("===strName[%s],length[%d],strpasswd[%s],length[%d].",strName.data(),strName.length(),strpasswd.data(),strpasswd.length());
    LogDebug("===strNameEx[%s],length[%d],strPasswdEx[%s],length[%d].",strNameEx.data(),strNameEx.length(),strPasswdEx.data(),strPasswdEx.length());
    
    std::map<int, models::Channel>::iterator itr = g_pSgipReportThread->m_mapChannelInfo.begin();
    for (; itr != g_pSgipReportThread->m_mapChannelInfo.end(); ++itr)
    {
        LogDebug("***name[%s],length[%d],passwd[%s],length[%d]",
            itr->second.m_strRespClientId.data(),itr->second.m_strRespClientId.length(),
            itr->second.m_strRespPasswd.data(),itr->second.m_strRespPasswd.length());
        if ((0 == itr->second.m_strRespClientId.compare(strNameEx))
            && (0 == itr->second.m_strRespPasswd.compare(strPasswdEx)))
        {
            m_uChannelId = itr->first;
            m_uChannelType = itr->second.m_uChannelType;
            m_strSp = itr->second._accessID;
            return true;
        }

        if (strNameEx.empty() && strPasswdEx.empty()
            && (itr->second.m_strRespClientId.compare("0") == 0)
            && (itr->second.m_strRespPasswd.compare("0") == 0))
        {
            if (itr->second.m_strSgipReportUrl.empty())
            {
                continue;
            }
            LogNotice("0,0,m_pSocket->GetRemoteAddress().GetInetAddress()=%s, itr->second.m_strSgipReportUrl=%s"
            , m_pSocket->GetRemoteAddress().GetInetAddress().data(), itr->second.m_strSgipReportUrl.data());
            if (0 == m_pSocket->GetRemoteAddress().GetInetAddress().compare(itr->second.m_strSgipReportUrl))
            {
                m_uChannelId = itr->first;
                m_uChannelType = itr->second.m_uChannelType;
                m_strSp = itr->second._accessID;

                return true;
            }
        }
    }
    
    return false;
}

bool SgipReportSocketHandler::onTransferData(UInt32& uFlag)
{

    SGIPBase resp;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
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
        
        LogDebug("***###***packetLength[%d],commandID[%d],node[%d],time[%d],sequenceID[%u]",
                resp.packetLength_,resp.requestId_,resp.sequenceIdNode_,resp.sequenceIdTime_,resp.sequenceId_);
        
        if (resp.requestId_ == SGIP_BIND) 
        {
            ////account authenticate
            SGIPConnectReq reqBind;
            reqBind.packetLength_ = resp.packetLength_;
            reqBind.Unpack(reader);

            UInt8 U8_Result = 0 ;
            if (false == checkLogin(reqBind.loginName_,reqBind.loginPassword_))
            {
                LogError("channelId[%u], this userName[%s],userPasswd[%s] is not exist.",
                         m_uChannelId, reqBind.loginName_.data(),reqBind.loginPassword_.data());
                uFlag = 1;
                U8_Result = 1;
                reader.SetGood( false );
                //return false;
            }

            SGIPConnectResp respBind;
            respBind.sequenceIdNode_ = resp.sequenceIdNode_;
            respBind.sequenceIdTime_ = resp.sequenceIdTime_;
            respBind.sequenceId_ = resp.sequenceId_;
            respBind.result_  = U8_Result ;
            
            pack::Writer writer(m_pSocket->Out());
            respBind.Pack(writer);
            return reader;

        } 
        else if (resp.requestId_ == SGIP_REPORT)
        {
            SGIPReportReq reqReport;
            reqReport.packetLength_ = resp.packetLength_;
            reqReport.Unpack(reader);

            LogElk("==sgip report== channelId[%d] smsid[%s],remark[%s],phone[%s].",
                    m_uChannelId, reqReport._msgId.data(),reqReport._remark.data(),reqReport.userNumber_.data());
            TDirectToDeliveryReportReqMsg* pMsg = new TDirectToDeliveryReportReqMsg();
            UInt32 status = reqReport.state_;
            if(status != 0)     //report failed
            {
                status = SMS_STATUS_CONFIRM_FAIL;
            }
            else    //report suc
            {
                status = SMS_STATUS_CONFIRM_SUCCESS;
            
            }
            
            pMsg->m_strDesc = reqReport._remark;
            pMsg->m_strErrorCode = reqReport._remark;
            pMsg->m_uChannelId = m_uChannelId;
            pMsg->m_iStatus = status;
            pMsg->m_strChannelSmsId = reqReport._msgId;
            pMsg->m_lReportTime = time(NULL);
            pMsg->m_iMsgType = MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ;
            pMsg->m_iHttpMode = LT_SGIP;
            string strPhone = reqReport.userNumber_.data();
            if (0 == strPhone.compare(0,2,"86"))
            {
                strPhone = strPhone.substr(2);
            }
            pMsg->m_strPhone = strPhone;

            std::map<int, models::Channel>::iterator itrId = g_pSgipReportThread->m_mapChannelInfo.find(m_uChannelId);
            if (itrId == g_pSgipReportThread->m_mapChannelInfo.end())
            {
                LogError("channelId[%d] is not find in g_pSgipReportThread->m_mapChannelInfo.",m_uChannelId);
            }
            else
            {
                pMsg->m_uChannelIdentify = itrId->second.m_uChannelIdentify;
                pMsg->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (itrId->second.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
            }
            
            g_CChannelThreadPool->smsTxPostMsg(pMsg);
            
            SGIPReportResp respReport;
            respReport.sequenceIdNode_ = resp.sequenceIdNode_;
            respReport.sequenceIdTime_ = resp.sequenceIdTime_;
            respReport.sequenceId_ = resp.sequenceId_;
            pack::Writer writer(m_pSocket->Out());
            respReport.Pack(writer);
        }
        else if (resp.requestId_ == SGIP_UNBIND)
        {
            SGIPUnBindResp respUnBind;
            respUnBind.sequenceIdNode_ = resp.sequenceIdNode_;
            respUnBind.sequenceIdTime_ = resp.sequenceIdTime_;
            respUnBind.sequenceId_ = resp.sequenceId_;
            pack::Writer writer(m_pSocket->Out());
            respUnBind.Pack(writer);
            uFlag = 1;
            return false;
        }
        else if (resp.requestId_ == SGIP_DELIVER)
        {
            SGIPDeliverReq req;
            req.packetLength_ = resp.packetLength_;
            req.Unpack(reader);

            string strContent = "";
            ContentCoding(req.m_strMsgContent,req.m_uMsgCoding,strContent);
            LogElk("==sgip MO== channelId[%d] codeType[%d],DstSp[%s], srcphone[%s],content[%s].",
                     m_uChannelId, req.m_uMsgCoding,req.m_strSpNum.data(),
                     req.m_strPhone.data(),strContent.data());

            string strPhone = req.m_strPhone.data();
            if (0 == strPhone.compare(0,2,"86"))
            {
                strPhone = strPhone.substr(2);
            }

            TToDirectMoReqMsg* pMo = new TToDirectMoReqMsg();
            pMo->m_iMsgType = MSGTYPE_TO_DIRECTMO_REQ;
            pMo->m_strContent = strContent;
            pMo->m_strPhone = strPhone;
            pMo->m_strDstSp = req.m_strSpNum.data();
            pMo->m_strSp = m_strSp.data();
            pMo->m_uChannelId = m_uChannelId;
            pMo->m_uMoType = LT_SGIP;
            pMo->m_uChannelType = m_uChannelType;
            g_pDirectMoThread->PostMsg(pMo);

            SGIPDeliverResp deResp;
            deResp.sequenceIdNode_ = resp.sequenceIdNode_;
            deResp.sequenceIdTime_ = resp.sequenceIdTime_;
            deResp.sequenceId_ = resp.sequenceId_;
            pack::Writer writer(m_pSocket->Out());
            deResp.Pack(writer);
        }
        else
        {
            char* buffer = new char[resp.packetLength_ - 20];
            reader(resp.packetLength_ - 20, buffer);
            if(buffer != NULL)
            {
                delete[] buffer;
            }
        }
    }
    else
    {
        cout<<"#########################################"<<endl;
        ///uFlag = 1;
        return false;
    }

    return reader;
}

bool SgipReportSocketHandler::ContentCoding(string& strSrc,UInt8 uCodeType,string& strDst)
{
    utils::UTFString utfHelper;
    
    if (0 == uCodeType)
    {
        LogWarn("codeType[%d] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (3 == uCodeType)
    {
        LogWarn("codeType[%d] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (4 == uCodeType)
    {
        LogWarn("codeType[%d] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (8 == uCodeType)
    {
        utfHelper.u162u(strSrc,strDst);
    }
    else if (15 == uCodeType)
    {
        utfHelper.g2u(strSrc,strDst);
    }
    else
    {
        LogError("strSrc[%s],codeType[%d] is invalid code type.",strSrc.data(),uCodeType);
        return false;
    }

    return true;
}
