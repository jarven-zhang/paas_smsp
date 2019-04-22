#include "HttpSender.h"
#include <stdlib.h>
#include "httprequest.h"
#include "httpresponse.h"
#include <math.h>
#include <vector>
#include "internalservice.h"
#include "internalsocket.h"
#include "eventtype.h"
#include "address.h"
#include "LogMacro.h"

int hex_char_value(char c)
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

int hex_to_decimal(const char* szHex, int len)
{
    int result = 0;
    for(int i = 0; i < len; i++)
    {
        result += (int)pow((float)16, (int)len-i-1) * hex_char_value(szHex[i]);
    }
    return result;
}

namespace http
{
    HttpSender::HttpSender()
    {
        _socket = NULL;
        _useCache = false;
        m_pInternalService = NULL;
        m_pThread = NULL;
        m_data = "";
        m_bIsComplete = true;
    }


    bool HttpSender::Init(InternalService* pInternalService, UInt64 iSeq, CThread* pThread)
    {
        m_pInternalService = pInternalService;
        m_pThread = pThread;
        m_uSeq = iSeq;
        InternalSocket *pInternalSocket = new InternalSocket();

        if (false == pInternalSocket->Init(m_pInternalService,pThread->m_pLinkedBlockPool))
        {
            LogError("******HttpSender::Init is failed.");
            delete pInternalSocket;
            return false;
        }
        _socket = pInternalSocket;
        _socket->SetHandler(this);
        return true;
    }

    HttpSender::~HttpSender()
    {

    }

    void HttpSender::OnEvent(int type, InternalSocket* socket)
    {
        if (type == EventType::Connected)
        {
            LogDebug("this is connect.");
        }
        else if (type == EventType::Read)
        {
            UInt32 size = socket->In()->Available();
            char *buffer = new char[size + 1];
            memset(buffer, 0, size + 1);

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
            LogDebug("this EventType[%d].", type);
            //close
            THttpResponseMsg* msgclose = new THttpResponseMsg();
            msgclose->m_uStatus = THttpResponseMsg::HTTP_CLOSED;       //CLOSE
            msgclose->m_iSeq = m_uSeq;
            msgclose->m_pSender = m_pThread;
            msgclose->m_iMsgType = MSGTYPE_HTTPRESPONSE;
            m_pThread->PostMsg(msgclose);
        }
        else if(type == EventType::Error)
        {
            LogDebug("this EventType[%d].", type);
            THttpResponseMsg* msgclose = new THttpResponseMsg();
            msgclose->m_uStatus = THttpResponseMsg::HTTP_ERR;      //CLOSE
            msgclose->m_iSeq = m_uSeq;
            msgclose->m_pSender = m_pThread;
            msgclose->m_iMsgType = MSGTYPE_HTTPRESPONSE;
            m_pThread->PostMsg(msgclose);
        }
        else
        {
            LogDebug("this EventType[%d].", type);
        }

    }

    void HttpSender::Get(std::string& url)
    {
        LogDebug("httpsend get[%s]", url.data());
        http::HttpRequest request;
        request.SetMethod("GET");
        request.SetUri(url);
        std::string data;
        request.Encode(data);
        std::string add;
        request.GetHeader("host", add);
        std::string::size_type pos = add.find(":");
        Address addr;
        if (pos != std::string::npos)
        {
            if(_useCache)
            {
                addr.SetIp(_ip);
            }
            else
            {
                addr.SetInetAddress(add.substr(0, pos));
            }
            addr.SetPort(atoi(add.substr(pos + 1).data()));
        }
        else
        {
            if(_useCache)
            {
                addr.SetIp(_ip);
            }
            else
            {
                addr.SetInetAddress(add);
            }
            addr.SetPort(80);
        }
        _socket->Connect(addr);
        _socket->Out()->Write(data.data(), data.size());
        _socket->Out()->Flush();
    }

    void HttpSender::Post(std::string& url, std::string& data, std::vector<std::string>* mvHeader )
    {
        LogDebug("httpsend Post[%s], data[%s]", url.data(), data.data());
        http::HttpRequest request;
        request.SetMethod("POST");
        request.SetUri(url);
        request.SetContent(data);

        if(mvHeader != NULL && mvHeader->size()>0)
        {
            std::vector <std::string>::iterator mv_Iter;
            for(mv_Iter = mvHeader->begin(); mv_Iter!=mvHeader->end(); mv_Iter++)
            {
                std::string strtmp = *mv_Iter;
                std::string strval;
                std::string strkey;
                std::string::size_type nPos;

                if (std::string::npos != (nPos = strtmp.find_first_of(":")))
                {
                    strkey = strtmp.substr(0, nPos);
                    strval = strtmp.substr(nPos+1).data();

                    if(strkey.length()>0 && strval.length()>0)
                    {
                        request.SetHeader(strkey, strval);
                    }
                }
            }

        }

        std::string content;
        request.Encode(content);
        std::string add;
        request.GetHeader("host", add);
        std::string::size_type pos = add.find(":");
        Address addr;
        if (pos != std::string::npos)
        {
            if(_useCache)
            {
                addr.SetIp(_ip);
            }
            else
            {
                addr.SetInetAddress(add.substr(0, pos));
            }
            addr.SetPort(atoi(add.substr(pos + 1).data()));
        }
        else
        {
            if(_useCache)
            {
                addr.SetIp(_ip);
            }
            else
            {
                addr.SetInetAddress(add);
            }
            addr.SetPort(80);
        }
        LogDebug("======content======[%s]", content.data());
        _socket->Connect(addr);
        _socket->Out()->Write(content.data(), content.size());
        _socket->Out()->Flush();

    }

    void HttpSender::Destroy()
    {
        if (NULL == _socket)
        {
            return;
        }

        _socket->Close();
        delete _socket;
    }


    bool HttpSender::OnData(const char* data, UInt32 size)
    {
        LogDebug("===data[%s].", data);
        ////////////////////////////////
        m_data = m_data + string(data, size);


        ////////////////////////////////////////////
        //unpacket, check if complete
        UInt32 uStatus = -1;

        THttpResponseMsg* msg = new THttpResponseMsg();
        http::HttpResponse Response;

        if (!msg->m_tResponse.Decode(m_data))
        {
            LogError("respone.Decode is failed.");
            uStatus = THttpResponseMsg::HTTP_ERR;
            m_bIsComplete = true;
        }
        else
        {
            uStatus = THttpResponseMsg::HTTP_RESPONSE;
            string strlength;
            if(msg->m_tResponse.GetHeader("Content-Length",strlength) || msg->m_tResponse.GetHeader("content-length",strlength))
            {
                //¨®Dcontent-length
                if ((int)(msg->m_tResponse.GetContent().length()) < atoi(strlength.data()))
                {
                    LogDebug("getcontent.length[%d], strlength[%d]", msg->m_tResponse.GetContent().length(), atoi(strlength.data()));
                    m_bIsComplete = false;
                    delete msg;
                    return true;
                }
                else
                {
                    m_bIsComplete = true;
                }
            }
        }

        string strChunked;
        if(msg->m_tResponse.GetHeader("transfer-encoding",strChunked) || msg->m_tResponse.GetHeader("Transfer-Encoding",strChunked))
        {
            if(strChunked == "chunked" || strChunked == "Chunked")
            {
                string strContentSrc = msg->m_tResponse.GetContent();
                string strContentDest;
                string strLenth;

                if(m_data.substr(m_data.length()-4) != "\r\n\r\n")
                {
                    m_bIsComplete = false;
                    delete msg;
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
                    int iLength = hex_to_decimal(strLenth.data(), strLenth.length());

                    LogNotice("================strContentSrc[%s], pos[%d], ilength[%d]", strContentSrc.c_str(), pos, iLength);

                    /*
                    strContentDest = strContentDest + strContentSrc.substr(pos + 2, iLength);
                    strContentSrc = strContentSrc.substr(pos + 2 + iLength);    //
                    */

                    if(strContentSrc.length() >= pos + 2 + iLength + 4)
                    {
                        strContentDest = strContentDest + strContentSrc.substr(pos + 2, iLength);
                        strContentSrc = strContentSrc.substr(pos + 2 + iLength);    //
                    }
                    else
                    {
                        LogWarn("trunk length is not correct. iLength[%d], strLength[%d], pos[%d]", iLength, strContentSrc.length(), pos);
                        strContentDest = strContentDest.append(strContentSrc.substr(pos + 2));
                        break;
                    }


                    pos = string::npos;
                    pos = strContentSrc.find_first_of("\r\n");
                }
                msg->m_tResponse.SetContent(strContentDest);
            }
        }

        if (m_bIsComplete == true)
        {
            msg->m_uStatus = uStatus;
            msg->m_iSeq = m_uSeq;
            msg->m_pSender = m_pThread;
            msg->m_iMsgType = MSGTYPE_HTTPRESPONSE;
            m_pThread->PostMsg(msg);
            return true;
        }
        else
        {
            delete msg;
            return false;
        }

    }


}
