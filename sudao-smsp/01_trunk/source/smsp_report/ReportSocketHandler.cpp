#include "ReportSocketHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>
#include "json/value.h"
#include "json/writer.h"
#include "UrlCode.h"
#include "HttpParams.h"
#include "ErrorCode.h"
#include "propertyutils.h"
#include "main.h"

using namespace std;


int hex_char_value_Ex(char c)     
{     
    if(c >= '0' && c <= '9')     
        return c - '0';     
    else if(c >= 'a' && c <= 'f')     
        return (c - 'a' + 10);     
    else if(c >= 'A' && c <= 'F')     
        return (c - 'A' + 10);     
    assert(0);     
    return 0;     
} 

int hex_to_decimal_Ex(const char* szHex, int len)     
{     
    int result = 0;     
    for(int i = 0; i < len; i++)     
    {     
        result += (int)pow((float)16, (int)len-i-1) * hex_char_value_Ex(szHex[i]);     
    }     
    return result;     
}

ReportSocketHandler::ReportSocketHandler(CThread* pThread, UInt64 uSeq)
{   
    m_socket = NULL;
    m_pThread = pThread;
    m_uSeq = uSeq;

    m_bIsComplete = true;
    m_data = "";
    m_strClientAddr = "";
}

ReportSocketHandler::~ReportSocketHandler()
{
    
}

bool ReportSocketHandler::Init(InternalService *service, int socket, const Address &address)
{
    InternalSocket *pSocket = new InternalSocket();
    if (false == pSocket->Init(service, socket, address,m_pThread->m_pLinkedBlockPool))
    {       
        LogError("ReportSocketHandler::Init is failed.");
        delete pSocket;
        return false;
    }
    pSocket->SetHandler(this);
    m_socket = pSocket;
    return true;
}
        
std::string ReportSocketHandler::GetClientAddress(){
    
    return m_strClientAddr;
}

void ReportSocketHandler::OnEvent(int type, InternalSocket *socket)
{   
    if (type == EventType::Connected) 
    {
    } 
    else if (type == EventType::Read) 
    {
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
    else if (type == EventType::Closed) 
    {
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Writable) 
    {
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    } 
    else if (type == EventType::Error)
    {
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else 
    {
        //LogDebug("");
    }
}


bool ReportSocketHandler::OnData(const char *data, UInt32 size)
{
    LogNotice("Recive OnData[%s]", data);
    m_data = m_data + string(data,size);

    http::HttpRequest request;
    if (!request.Decode(m_data))
    {
        LogError("this is data format is error.");
        string strRes = "ok";
        ProcessRequest(strRes);
        return true;
    }
    else
    {   
        request.GetHeader("x-forwarded-for", m_strClientAddr);
        
        string strlength;
        if(request.GetHeader("Content-Length",strlength) || request.GetHeader("content-length",strlength))
        {
            //¨®Dcontent-length
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
    if(request.GetHeader("transfer-encoding",strChunked) || request.GetHeader("Transfer-Encoding",strChunked))
    {   
        if(strChunked == "chunked" || strChunked == "Chunked")
        {
            string strContentSrc = request.GetContent();
            string strContentDest;
            string strLenth;

            if(m_data.substr(m_data.length()-4) != "\r\n\r\n")
                {
                    m_bIsComplete = false;
                    return true;
                }
                else
                {
                    m_bIsComplete = true;
                }

            std::string::size_type pos = strContentSrc.find_first_of("\r\n");
            while(pos != string::npos)
            {
                strLenth = strContentSrc.substr(0 ,pos);
                int iLength = hex_to_decimal_Ex(strLenth.data(), strLenth.length());
                strContentDest = strContentDest + strContentSrc.substr(pos + 2, iLength);
                strContentSrc = strContentSrc.substr(pos + 2 + iLength);    
                pos = string::npos;
                pos = strContentSrc.find_first_of("\r\n");
            }
            request.SetContent(strContentDest);
        }
    }
    
    std::string uri = request.GetUri();
    std::string postData = request.GetContent();
    std::string file;
    std::string query;
    std::string::size_type pos = uri.find_first_of('?');
    if (pos != std::string::npos)
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

    if (std::string::npos != file.find("report.do") && g_pReportReceiveThread != NULL && g_pReportReceiveThread == m_pThread)       
    {   
        std::string::size_type posEnd = file.find("report.do");     
        std::string channel = file.substr(1, posEnd-1);
        Toupper(channel);

        TReportReciveRequest* req = new TReportReciveRequest();
        req->m_strRequest = strData;
        req->m_strChannel = channel;
        req->m_socketHandler = this;
        req->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
        req->m_iSeq = m_uSeq;
        m_pThread->PostMsg(req);
    }
    else if(file.compare("/syniverse.do")==0 && g_pReportReceiveThread != NULL && g_pReportReceiveThread == m_pThread)
    {
        TReportReciveRequest* req = new TReportReciveRequest();
        req->m_strRequest = strData;
        req->m_strChannel = "SYNIVERSE";
        req->m_socketHandler = this;
        req->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
        req->m_iSeq = m_uSeq;
        m_pThread->PostMsg(req);
    }
    else if(std::string::npos != file.find("/getreport"))       
    {
        //user getreport
        THttpServerRequest* req = new THttpServerRequest();
        req->m_strRequest = strData;
        req->m_socketHandler = this;
        req->m_iMsgType = MSGTYPE_REPORT_GET_REPORT_REQ;
        req->m_iSeq = m_uSeq;
        req->m_strSessionID = "getreport";
        g_pGetReportServerThread->PostMsg(req);
    }
    else if(std::string::npos != file.find("/getmo"))       
    {
        //user getreport
        THttpServerRequest* req = new THttpServerRequest();
        req->m_strRequest = strData;
        req->m_socketHandler = this;
        req->m_iMsgType = MSGTYPE_REPORT_GET_MO_REQ;
        req->m_iSeq = m_uSeq;
        req->m_strSessionID = "getmo";
        g_pGetReportServerThread->PostMsg(req);
    }
    else if(std::string::npos != file.find("/getbalance"))
    {   
        //user getbalance
        THttpServerRequest* req = new THttpServerRequest();
        req->m_strRequest = strData;
        req->m_socketHandler = this;
        req->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
        req->m_iSeq = m_uSeq;
        req->m_strSessionID = "getbalance";
        g_pGetBalanceServerThread->PostMsg(req);
    }
    else if(std::string::npos != file.find("/authentication"))
    {
        LogDebug("Length[%u].", strData.length());

        THttpServerRequest* req = new THttpServerRequest();
        req->m_strRequest = strData;
        req->m_socketHandler = this;
        req->m_iSeq = m_uSeq;
        req->m_strSessionID = "authentication";
        req->m_iMsgType = MSGTYPE_AUTHENTICATION_REQ;
        g_pGetBalanceServerThread->PostMsg(req);
    }
    else if(std::string::npos != file.find("/queryreport"))
    {
        //user query report
        std::string::size_type posBegin = file.find("report/");
        std::string::size_type posEnd = file.find("/queryreport");
        string strClientid;
        
        if(posBegin != string::npos && posEnd!= string::npos && (posBegin+7) < posEnd)
        {
            strClientid = file.substr(posBegin+7, posEnd - posBegin-7);
        }
        else
        {
            //do nothing
        }
        
        THttpServerRequest* req = new THttpServerRequest();
        req->m_strRequest = strData;
        req->m_socketHandler = this;
        req->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
        req->m_iSeq = m_uSeq;
        req->m_strSessionID = strClientid;
        g_pQueryReportServerThread->PostMsg(req);
    }
    else
    {
        LogElk("this is 404");
        Status404();
        return false;
    }

    return true;
}

void ReportSocketHandler::ProcessRequest(std::string result)
{
    std::string content;
    http::HttpResponse respone;
    respone.SetStatusCode(200);
    respone.SetContent(result);
    respone.Encode(content);
    m_socket->Out()->Write(content.data(), content.size());
    m_socket->Out()->Flush();
}

std::string ReportSocketHandler::_retcode2Json(string strReturncode)
{
    Json::Value root;
    root["result"] = Json::Value(strReturncode);
    Json::FastWriter fast_writer;
    return fast_writer.write(root);
}


void ReportSocketHandler::Destroy()
{
    m_socket->Close();
    delete m_socket;
}

void ReportSocketHandler::Status404()
{
    std::string content = "HTTP/1.1 404 Not Found\r\n\r\n 404 NOT FOUND";
    m_socket->Out()->Write(content.data(), content.size());
    m_socket->Out()->Flush();
}

void ReportSocketHandler::Status500()
{
    std::string content = "HTTP/1.1 500 Internel Server Error\r\n\r\n";
    m_socket->Out()->Write(content.data(), content.size());
    m_socket->Out()->Flush();
}


void ReportSocketHandler::Toupper(std::string & str) 
{
    for(size_t i=0;i<str.size();i++)
        str.at(i)=toupper(str.at(i));
}




