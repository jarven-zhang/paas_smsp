#include "HttpSocketHandler.h"
#include "HttpServerThread.h"
#include "LogMacro.h"
#include "UrlCode.h"
#include "propertyutils.h"


using namespace std;

HttpSocketHandler::HttpSocketHandler(CThread* pThread, UInt64 uSeq)
{
    m_socket = NULL;
    m_uSeq = uSeq;
    m_pThread = pThread;
}

HttpSocketHandler::~HttpSocketHandler()
{

}

bool HttpSocketHandler::Init(InternalService* service, int socket, const Address& address)
{
    InternalSocket* pSocket = new InternalSocket();

    if (false == pSocket->Init(service, socket, address, m_pThread->m_pLinkedBlockPool))
    {
        LogError("******HttpSocketHandler::Init is failed.");
        delete pSocket;
        return false;
    }

    pSocket->SetHandler(this);
    m_socket = pSocket;
    return true;
}

void HttpSocketHandler::OnEvent(int type, InternalSocket* socket)
{
    if (type == EventType::Connected)
    {
    }
    else if (type == EventType::Read)
    {
        LogDebug("HttpSocketHandler onRead");
        UInt32 size = socket->In()->Available();
        char* buffer = new char[size];

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
        //wirte over
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Closed)
    {
        //// proc close
        TMsg* req = new TMsg();
        req->m_iSeq = m_uSeq;
        req->m_iMsgType = MSGTYPE_SOCKET_WRITEOVER;
        m_pThread->PostMsg(req);
    }
    else if (type == EventType::Error)
    {
        //// proc error
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


bool HttpSocketHandler::OnData(const char* data, UInt32 size)
{
    LogDebug("Log1:OnData[%s]", data);
    http::HttpRequest request;

    if (!request.Decode(std::string(data, size)))
    {
        LogError("Decode error.");
        return false;
    }

    std::string uri = request.GetUri();
    std::string file;
    std::string query;
    std::string postData = request.GetContent();
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

    //adapt GET and POST
    string strData = "";

    if (0 == strncmp("GET", data, strlen("GET")))
    {
        strData = query;
    }
    else if (0 == strncmp("POST", data, strlen("POST")))
    {
        strData = postData;
    }
    else
    {
        LogError("this httpmode is not support.");
        string strRes = "ok";

        std::string content;
        http::HttpResponse respone;
        respone.SetStatusCode(200);
        respone.SetContent(strRes);
        respone.Encode(content);
        m_socket->Out()->Write(content.data(), content.size());
        m_socket->Out()->Flush();

        return true;
    }


    if (file.compare("/smscharge") == 0)
    {
        THttpRequest* req = new THttpRequest();
        req->m_strRequest = strData;
        req->m_iSeq = m_uSeq;
        req->m_socketHandle = this;
        req->m_iMsgType = MSGTYPE_HTTP_SERVICE_REQ;
        m_pThread->PostMsg(req);
    }
    else    //error
    {
        Status404();
        return false;
    }

    return true;
}

void HttpSocketHandler::Destroy()
{
    m_socket->Close();
    delete m_socket;
}

void HttpSocketHandler::Status404()
{
    std::string content = "HTTP/1.1 404 Not Found\r\n\r\n 404 NOT FOUND";
    m_socket->Out()->Write(content.data(), content.size());
    m_socket->Out()->Flush();
}





