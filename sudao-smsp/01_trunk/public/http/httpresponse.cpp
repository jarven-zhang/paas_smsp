#include "httpresponse.h"

#include "notation.h"
#include "stringutils.h"
#include<stdlib.h>
#include <stdio.h>

namespace http
{

    HttpResponse::HttpResponse()
    {
        //
        version_="HTTP/1.1";
        status_ = 0;
    }

    HttpResponse::~HttpResponse()
    {

    }

    const std::string &HttpResponse::GetContent() const
    {
        return content_;
    }

    void HttpResponse::SetContent(const std::string &content)
    {
        content_ = content;
        char length[256] = { 0 };
        snprintf(length, 256,"%d", (UInt32) content_.size());
        SetHeader("content-length", length);
    }

    const std::string &HttpResponse::GetProtocolVersion() const
    {
        return version_;
    }

    void HttpResponse::SetProtocolVersion(const std::string &version)
    {
        version_ = version;
    }

    int HttpResponse::GetStatusCode() const
    {
        return status_;
    }

    void HttpResponse::SetStatusCode(int code)
    {
        status_ = code;
    }

    std::string HttpResponse::GetReasonPhrase() const
    {
        return reason_;
    }

    bool HttpResponse::GetHeader(const std::string &name,
                                 std::string &value) const
    {
        Params::const_iterator it = headers_.find(name);
        if (it != headers_.end())
        {
            value = it->second;
            return true;
        }
        else
        {
            return false;
        }
    }

    void HttpResponse::SetHeader(const std::string &name,
                                 const std::string &value)
    {
        headers_[name] = value;
    }

    void HttpResponse::RemoveHeader(const std::string &name)
    {
        Params::iterator it = headers_.find(name);
        if (it != headers_.end())
        {
            headers_.erase(it);
        }
    }

    const Params &HttpResponse::GetHeaders() const
    {
        return headers_;
    }

    bool HttpResponse::Decode(const std::string &data)
    {
        const char *begin = data.data();
        const char *end = data.data() + data.size();

        begin = ReadStatusLine(begin, end);
        begin = ReadHeaders(begin, end);

        std::string value;
        if (begin != NULL)
        {
            if (GetHeader("content-length", value))     //match "content-length"
            {
                int length = 0;
                if (StringUtils::ToInteger(value, length) && length <= end - begin)
                {
                    content_.assign(begin, length);
                    return true;
                }
                else
                {
                    return true;
                }
            }
            else if(GetHeader("Content-Length", value))     //ADD BY FANGJINXIONG BEGIN. match "Content-Length".
            {
                int length = 0;
                if (StringUtils::ToInteger(value, length) && length <= end - begin)
                {
                    content_.assign(begin, length);
                    return true;
                }       //ADD BY FANGJINXIONG END
                else
                {
                    return true;
                }
            }
            else    //can not get content-length head
            {
                content_.assign(begin, end - begin);
                return true;
            }
        }
        printf("http request decode data err!\n");
        return false;
    }

    bool HttpResponse::Encode(std::string &data)
    {
        //request line
        SetHeader("Content-Type","application/json;charset=utf-8");
        data.reserve(2048);
        data.append(version_);
        data.append(" ");
        char status[24]= {0};
        snprintf(status,24,"%d",status_);
        data.append(status);
        data.append(" ");
        data.append(reason_);
        data.append(" ");
        data.append("\r\n");
        //request header
        for(Params::const_iterator it=headers_.begin(); it!=headers_.end(); it++)
        {
            data.append((*it).first);
            data.append(":");
            data.append((*it).second);
            data.append("\r\n");
        }
        //request body
        data.append("\r\n");
        data.append(content_);
        return true;
    }

    bool HttpResponse::EncodeTextHtml(std::string &data)
    {
        //request line
        SetHeader("Content-Type","text/html;charset=utf-8");
        data.reserve(2048);
        data.append(version_);
        data.append(" ");
        char status[24]= {0};
        snprintf(status,24,"%d",status_);
        data.append(status);
        data.append(" ");
        data.append(reason_);
        data.append(" ");
        data.append("\r\n");
        //request header
        for(Params::const_iterator it=headers_.begin(); it!=headers_.end(); it++)
        {
            data.append((*it).first);
            data.append(":");
            data.append((*it).second);
            data.append("\r\n");
        }
        //request body
        data.append("\r\n");
        data.append(content_);
        return true;
    }

    const char *HttpResponse::ReadStatusLine(const char *begin, const char *end)
    {
        ///printf("[HttpResponse::ReadStatusLine] begin:%s", begin);
        begin = ReadVersion(begin, end);
        begin = ReadSpace(begin, end);
        begin = ReadStatus(begin, end);
        begin = ReadSpace(begin, end);
        begin = ReadReason(begin, end);
        return begin;
    }

    const char *HttpResponse::ReadVersion(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        for (const char *i = begin; i < end; ++i)
        {
            if (Notation::IsSpace(i[0]))
            {
                if (i != begin)
                {
                    version_.assign(begin, i - begin);
                    return i;
                }
                break;
            }
        }
        return NULL;
    }

    const char *HttpResponse::ReadSpace(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        if (Notation::IsSpace(begin[0]))
        {
            return begin + 1;
        }
        else
        {
            return NULL;
        }
    }

    const char *HttpResponse::ReadStatus(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        if (!Notation::IsDigit(begin[0]))
        {
            return NULL;
        }
        status_ = begin[0] - '0';

        while (++begin < end)
        {
            if (!Notation::IsDigit(begin[0]))
            {
                return begin;
            }
            status_ = status_ * 10 + (begin[0] - '0');
        }
        return NULL;
    }

    const char *HttpResponse::ReadReason(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        for (const char *i = begin; i + 1 < end; ++i)
        {
            if (Notation::IsReturn(i[0]) && Notation::IsLineFeed(i[1]))
            {
                reason_.assign(begin, i - begin);
                return i;
            }
        }
        return NULL;
    }

    const char *HttpResponse::ReadHeaders(const char *begin, const char *end)
    {
        while (begin < end && begin != NULL)
        {
            const char *pos = Notation::GetCRLF(begin, end);
            if (pos == NULL)
            {
                return NULL;
            }
            begin = pos;
            pos = Notation::GetCRLF(begin, end);
            if (pos == NULL)
            {
                begin = ReadHeader(begin, end);
            }
            else
            {
                begin = pos;
                break;
            }
        }
        return begin;
    }

    const char *HttpResponse::ReadHeader(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        std::string key;
        const char *fieldEnd = Notation::GetToken(begin, end);
        if (fieldEnd != NULL)
        {
            key.assign(begin, fieldEnd - begin);
            begin = fieldEnd;
        }
        else
        {
            return NULL;
        }

        if (begin < end && begin != NULL && begin[0] == ':')
        {
            begin = begin + 1;
        }
        else
        {
            return NULL;
        }

        std::string value;
        while (begin < end && begin != NULL)
        {
            const char *pos = begin;
            bool readed = false;
            while (pos != NULL)
            {
                pos = Notation::GetLWS(begin, end);
                if (pos != NULL)
                {
                    readed = true;
                    begin = pos;
                }
            }
            if (readed && !value.empty())
            {
                value.append(1, ' ');
            }
            pos = Notation::GetText(begin, end);
            if (pos != NULL)
            {
                readed = true;
                value.append(begin, pos);
                begin = pos;
            }
            if (!readed)
            {
                break;
            }
        }
        headers_[key] = value;
        return begin;
    }

}

