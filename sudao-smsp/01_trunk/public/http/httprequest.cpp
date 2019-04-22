#include "httprequest.h"

#include <algorithm>
#include <string.h>
#include<stdlib.h>
#include <stdio.h>
#include "notation.h"
#include "stringutils.h"

namespace http
{

    HttpRequest::HttpRequest()
    {
        version_="HTTP/1.1";
    }

    HttpRequest::~HttpRequest()
    {
    }

    const std::string &HttpRequest::GetMethod() const
    {
        return method_;
    }

    void HttpRequest::SetMethod(const std::string &method)
    {
        method_ = method;
    }

    const std::string &HttpRequest::GetProtocolVersion() const
    {
        return version_;
    }

    void HttpRequest::SetProtocolVersion(const std::string &version)
    {
        version_ = version;
    }

    const std::string &HttpRequest::GetUri() const
    {
        return uri_;
    }

    void HttpRequest::SetUri(const std::string &uri)
    {
        uri_ = uri;
        std::string::size_type pos = uri_.find("http://");
        std::string url;
        //printf("pos:%d\n",(int)pos);
        if(pos!= std::string::npos)
        {
            url=uri_.substr(pos+strlen("http://"));
        }
        else
        {
            url=uri_;
        }
        pos = url.find("/");
        //printf("pos:%d\n",(int)pos);
        if(pos!= std::string::npos)
        {
            SetHeader("host",url.substr(0,pos));
            path_=url.substr(pos);
        }
        else
        {
            path_="/";
        }
    }

    bool HttpRequest::GetHeader(const std::string &name, std::string &value) const
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

    void HttpRequest::SetHeader(const std::string &name, const std::string &value)
    {
        headers_[name] = value;
    }

    void HttpRequest::RemoveHeader(const std::string &name)
    {
        Params::iterator it = headers_.find(name);
        if (it != headers_.end())
        {
            headers_.erase(it);
        }
    }

    const Params &HttpRequest::GetHeaders() const
    {
        return headers_;
    }

    const std::string &HttpRequest::GetContent() const
    {
        return content_;
    }

    void HttpRequest::SetContent(const std::string &content)
    {
        content_ = content;
        char length[256]= {0};
        snprintf(length,256,"%d",(UInt32)content_.size());
        SetHeader("Content-Length",length);
        //SetHeader("content-length",length);
    }

    bool HttpRequest::Decode(const std::string &data)
    {
        const char *begin = data.data();
        const char *end = data.data() + data.size();

        begin = ReadRequestLine(begin, end);
        begin = ReadHeaders(begin, end);

        std::string value;
        if (begin != NULL)
        {
            if (GetHeader("content-length", value))
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
            else if(GetHeader("Content-Length", value))     
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
            else
            {
                content_.assign(begin, end - begin);
                return true;
            }
        }
        return false;
    }

    void HttpRequest::GetPath(std::string& path) const
    {
        if(path_.empty())
        {
            path="/";
        }
        else
        {
            path=path_;
        }
    }
    bool HttpRequest::Encode(std::string &data)
    {
        //request line

        std::string strcotype;
        if(GetHeader("Content-Type", strcotype))
        {
            //add-by-fjx:already has "Content-Type"
        }
        else
        {
            //if(method_ == "POST")
            //{
            //    SetHeader("Content-Type","application/x-www-form-urlencoded");
            //}
            //else
            //{
            //    SetHeader("Content-Type","text/html; charset=UTF-8");
            //}
            
            SetHeader("Content-Type","application/json;charset=utf-8");
        }

        data.reserve(2048);
        data.append(method_);
        data.append(" ");
        std::string path;
        GetPath(path);
        data.append(path);
        data.append(" ");
        data.append(version_);
        data.append("\r\n");
        //request header
        for(Params::const_iterator it=headers_.begin(); it!=headers_.end(); it++)
        {
            data.append((*it).first);
            data.append(": ");
            data.append((*it).second);
            data.append("\r\n");
        }
        //request body
        data.append("\r\n");
        data.append(content_);
        return false;
    }

    const char *HttpRequest::ReadRequestLine(const char *begin, const char *end)
    {
        begin = ReadMethod(begin, end);
        begin = ReadSpace(begin, end);
        begin = ReadUri(begin, end);
        begin = ReadSpace(begin, end);
        begin = ReadVersion(begin, end);
        return begin;
    }

    const char *HttpRequest::ReadMethod(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        const char *methodEnd = Notation::GetToken(begin, end);
        if (methodEnd != NULL)
        {
            method_.assign(begin, methodEnd - begin);
        }
        return methodEnd;
    }

    const char *HttpRequest::ReadSpace(const char *begin, const char *end)
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

    const char *HttpRequest::ReadUri(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        for (const char *i = begin; i < end; ++i)
        {
            if (Notation::IsSpace(*i))
            {
                uri_.assign(begin, i - begin);
                return i;
            }
        }
        return NULL;
    }

    const char *HttpRequest::ReadVersion(const char *begin, const char *end)
    {
        if (begin >= end || begin == NULL)
        {
            return NULL;
        }
        for (const char *i = begin; i + 1 < end; ++i)
        {
            if (Notation::IsReturn(i[0]) && Notation::IsLineFeed(i[1]))
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

    const char *HttpRequest::ReadHeaders(const char *begin, const char *end)
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

    const char *HttpRequest::ReadHeader(const char *begin, const char *end)
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
            std::transform(key.begin(), key.end(), key.begin(), tolower);
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

