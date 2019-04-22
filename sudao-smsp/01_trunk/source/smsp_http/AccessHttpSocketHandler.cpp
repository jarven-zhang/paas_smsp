#include "AccessHttpSocketHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "Comm.h"
#include "json/value.h"
#include "json/writer.h"
#include "LogMacro.h"

#include "UrlCode.h"
#include "propertyutils.h"
#include "CHttpServiceThread.h"
#include "GetReportServerThread.h"
#include "GetBalanceServerThread.h"
#include "CQueryReportServerThread.h"

AccessHttpSocketHandler::AccessHttpSocketHandler(CThread *pThread, UInt64 ullSeq)
{
    m_socket 		= NULL;
    m_ullSeq 		= ullSeq;
    m_pThread 		= pThread;
    m_bIsComplete 	= true;
    m_data 			= "";
    m_strServerIpAddress = "";
}

AccessHttpSocketHandler::~AccessHttpSocketHandler()
{
}

bool AccessHttpSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
    InternalSocket *pSocket = new InternalSocket();
    if (false == pSocket->Init(service, socket, address, m_pThread->m_pLinkedBlockPool))
    {
        LogError("******SendSocketHandler::Init is failed.");
        SAFE_DELETE(pSocket);
        return false;
    }

    pSocket->SetHandler(this);
    m_socket = pSocket;
    return true;
}

void AccessHttpSocketHandler::OnEvent(int type, InternalSocket *socket)
{
    if (type == EventType::Connected)
    {
    }
    else if (type == EventType::Read)
    {
        LogDebug("sendsockethandler onRead");
        UInt32 size = socket->In()->Available();
        char *buffer = new char[size];
        if (socket->In()->Read(buffer, size))
        {
            if (OnData(buffer, size))
            {
                socket->In()->Mark();
            }
            else
            {
                socket->In()->Reset();
            }
        }
        delete[] buffer;
    }
    else if (type == EventType::Writable)
    {
        // wirte over
        TMsg *req = new TMsg();
        req->m_iSeq = m_ullSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Closed)
    {
        // proc close
        TMsg *req = new TMsg();
        req->m_iSeq = m_ullSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Error)
    {
        // proc error
        TMsg *req = new TMsg();
        req->m_iSeq = m_ullSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else
    {
        //LogDebug("");
    }
}

void AccessHttpSocketHandler::ProcessRequest(string result)
{
    string content = "";
    http::HttpResponse respone;
    respone.SetStatusCode(200);
    respone.SetContent(result);
    respone.Encode(content);
    m_socket->Out()->Write(content.data(), content.size());
    m_socket->Out()->Flush();
}

bool AccessHttpSocketHandler::OnData(const char *data, UInt32 size)
{
    LogElk("Log1:OnData AccessHttpServer[%s] m_ullSeq:%lu", string(data).c_str(), m_ullSeq);
    m_data = m_data + string(data, size);

    http::HttpRequest request;
    if (!request.Decode(m_data))
    {
        LogError("data:%s, this is data format is error.", string(data).c_str());
        string strRes = "ok";
        ProcessRequest(strRes);
        return true;
    }
    else
    {
        request.GetHeader("x-forwarded-for", m_strServerIpAddress);

        string strlength = "";
        if(request.GetHeader("Content-Length", strlength) || request.GetHeader("content-length", strlength))
        {
            //Dcontent-length
            if ((int)(request.GetContent().length()) < atoi(strlength.data()))
            {
                LogDebug("getcontent.length[%d], strlength[%d]", request.GetContent().length(), atoi(strlength.data()));
                m_bIsComplete = false;
                return true;
            }
            else
            {
                m_bIsComplete = true;
            }
        }
    }

    string strChunked;
    if(request.GetHeader("transfer-encoding", strChunked) || request.GetHeader("Transfer-Encoding", strChunked))
    {
        if(strChunked == "chunked" || strChunked == "Chunked")
        {
            string strContentSrc = request.GetContent();
            string strContentDest = "", strLenth = "";

            if(m_data.substr(m_data.length() - 4) != "\r\n\r\n")
            {
                m_bIsComplete = false;
                return true;
            }
            else
            {
                m_bIsComplete = true;
            }

            string::size_type pos = strContentSrc.find_first_of("\r\n");
            while(pos != string::npos)
            {
                strLenth = strContentSrc.substr(0 , pos);
                int iLength = Comm::hex_to_decimal_Ex(strLenth.data(), strLenth.length());
                strContentDest = strContentDest + strContentSrc.substr(pos + 2, iLength);
                strContentSrc = strContentSrc.substr(pos + 2 + iLength);
                pos = string::npos;
                pos = strContentSrc.find_first_of("\r\n");
            }
            request.SetContent(strContentDest);
        }
    }

    string uri = request.GetUri();
    string postData = request.GetContent();
    string file = "", query = "";
    string::size_type pos = uri.find_first_of('?');
    if (pos != string::npos)
    {
        query = uri.substr(pos + 1);
        file = uri.substr(0, pos);
    }
    else
    {
        file = uri;
    }

    string strData = "";
    if (0 == strncmp("GET", m_data.data(), strlen("GET")))
    {
        strData = query;
    }
    else if (0 == strncmp("POST", m_data.data(), strlen("POST")))
    {
        strData = postData;
    }
    else
    {
        LogError("this httpmode is not support.");
        string strRes = "ok";
        ProcessRequest(strRes);
        return true;
    }

    if (file.find("/sendsms") != string::npos)							// 短信下行
    {
        if (file.find("httpserver/") != string::npos)                   // "/sms-partner/httpserver/b000k2/sendsms"
        {
            string::size_type posBegin = file.find("httpserver/");
            string::size_type posEnd = file.find("/sendsms");
            string strClientid = "";

            if(posBegin != string::npos && posEnd != string::npos && (posBegin + 11) < posEnd)
            {
                strClientid = file.substr(posBegin + 11, posEnd - posBegin - 11);
            }
            else
            {
                //do nothing
            }

            THTTPRequest *req = new THTTPRequest();
            req->m_strRequest = strData;
            req->m_iSeq = m_ullSeq;
            req->m_strSessionID = strClientid;
            req->m_socketHandle = this;
            req->m_ucHttpProtocolType = HttpProtocolType_Get_Post;
            req->m_iMsgType = MSGTYPE_SMSERVICE_REQ_2;
            m_pThread->PostMsg(req);
        }
        else if (file.find("/smsp/access/") != string::npos)        // "/smsp/access/b000k2/sendsms"
        {
            // for tencent cloud
            THTTPRequest *req = new THTTPRequest();
            req->m_strRequest = postData;
            req->m_iSeq = m_ullSeq;
            req->m_socketHandle = this;
            req->m_ucHttpProtocolType = HttpProtocolType_TX_Json;
            req->m_iMsgType = MSGTYPE_TENCENT_SMSERVICE_REQ;
            m_pThread->PostMsg(req);
        }
        else                                                       	// "/sms-partner/access/b00jn6/sendsms"
        {
            // for universal
            THTTPRequest *req = new THTTPRequest();
            req->m_strRequest = postData;
            req->m_iSeq = m_ullSeq;
            req->m_socketHandle = this;
            req->m_ucHttpProtocolType = HttpProtocolType_Json;
            req->m_iMsgType = MSGTYPE_SMSERVICE_REQ;
            m_pThread->PostMsg(req);
        }
    }
    else if (file.find("MWGate/wmgw.asmx") != string::npos)
    {
        THTTPRequest *req = new THTTPRequest();

        req->m_strRequest = postData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_iMsgType = MSGTYPE_SMSERVICE_REQ_JD;
        if(file.find("MongateCsSpSendSmsNew") != string::npos)
        {
            req->m_ucHttpProtocolType = HttpProtocolType_JD;
        }
        else
        {
            req->m_ucHttpProtocolType = HttpProtocolType_JD_Webservice;
        }
        m_pThread->PostMsg(req);
    }
    else if (file.find("timer_send_sms") != string::npos)			// "sms-partner/access/{用户帐号}/ timer_send_sms" 定时短信下行
    {
        // "/timer_send_sms"
        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = postData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_ucHttpProtocolType = HttpProtocolType_Json;
        req->m_iMsgType = MSGTYPE_TIMERSEND_SMSERVICE_REQ;
        m_pThread->PostMsg(req);
    }
    else if (file.find("/api/send/index.php") != string::npos)
    {
        // "/sh mlrt"
        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = postData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_ucHttpProtocolType = HttpProtocolType_ML;
        req->m_iMsgType = MSGTYPE_SMSERVICE_REQ_ML;
        m_pThread->PostMsg(req);
    }
    else if(file.find("/templatesms") != string::npos)          // "/sms-partner/access/{用户帐号}/templatesms" 模板短信下行
    {
        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = postData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_ucHttpProtocolType = HttpProtocolType_Json;
        req->m_iMsgType = MSGTYPE_TEMPLATE_SMSERVICE_REQ;
        m_pThread->PostMsg(req);
    }
    else if(string::npos != file.find("/getbalance")) 			// "/sms-partner/report/{用户帐号}/getbalance" 余额查询
    {
        //user getbalance
        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = strData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_strSessionID = "getbalance";
        req->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
        g_pGetBalanceServerThread->PostMsg(req);
    }
    else if(string::npos != file.find("/authentication"))		// "/sms-partner/report/esms/authentication" 联网认证
    {
        LogDebug("Length[%u].", strData.length());

        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = strData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_strSessionID = "authentication";
        req->m_iMsgType = MSGTYPE_AUTHENTICATION_REQ;
        g_pGetBalanceServerThread->PostMsg(req);
    }
    else if(string::npos != file.find("/getreport"))			// "/sms-partner/report/{用户帐号}/getreport"	短信状态报告－拉取方式
    {
        //user getreport
        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = strData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_strSessionID = "getreport";
        req->m_iMsgType = MSGTYPE_REPORT_GET_REPORT_REQ;
        g_pGetReportServerThread->PostMsg(req);
    }
    else if(string::npos != file.find("/getmo"))				// "/sms-partner/report/{用户帐号}/getmo"	短信上行－拉取方式
    {
        //user getreport
        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = strData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_strSessionID = "getmo";
        req->m_iMsgType = MSGTYPE_REPORT_GET_MO_REQ;
        g_pGetReportServerThread->PostMsg(req);
    }
    else if(std::string::npos != file.find("/queryreport"))
    {
        //user query report
        std::string::size_type posBegin = file.find("report/");
        std::string::size_type posEnd = file.find("/queryreport");
        string strClientid = "";

        if(posBegin != string::npos && posEnd != string::npos && (posBegin + 7) < posEnd)
        {
            strClientid = file.substr(posBegin + 7, posEnd - posBegin - 7);
        }
        else
        {
            //do nothing
        }

        THTTPRequest *req = new THTTPRequest();
        req->m_strRequest = strData;
        req->m_iSeq = m_ullSeq;
        req->m_socketHandle = this;
        req->m_strSessionID = strClientid;
        req->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
        g_pQueryReportServerThread->PostMsg(req);
    }
    else
    {
        LogError("[WebHandler::OnData] this is 404");
        Status404();
        return false;
    }

    return true;
}

string AccessHttpSocketHandler::_retcode2Json(string strReturncode)
{
    Json::Value root;
    root["result"] = Json::Value(strReturncode);
    Json::FastWriter fast_writer;
    return fast_writer.write(root);
}

void AccessHttpSocketHandler::Destroy()
{
    m_socket->Close();
    SAFE_DELETE(m_socket);
}

void AccessHttpSocketHandler::Status404()
{
    string content = "HTTP/1.1 404 Not Found\r\n\r\n 404 NOT FOUND";
    m_socket->Out()->Write(content.data(), content.size());
    m_socket->Out()->Flush();
    //Destroy();
}

string AccessHttpSocketHandler::GetServerIpAddress()
{
    return m_strServerIpAddress;
}

