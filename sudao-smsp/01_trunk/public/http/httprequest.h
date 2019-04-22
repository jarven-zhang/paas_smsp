#ifndef HTTP_HTTPREQUEST_H_
#define HTTP_HTTPREQUEST_H_
#include "platform.h"

#ifndef API
#include <string>
#include <map>
#endif /* API */

#include "params.h"

namespace http
{

    class HttpRequest
    {
    public:
        HttpRequest();
        virtual ~HttpRequest();

    public:
        const std::string &GetMethod() const;
        void SetMethod(const std::string &method);



        const std::string &GetProtocolVersion() const;
        void SetProtocolVersion(const std::string &version);

        const std::string &GetUri() const;
        void SetUri(const std::string &uri);

        bool GetHeader(const std::string &name, std::string &value) const;
        void SetHeader(const std::string &name, const std::string &value);
        void RemoveHeader(const std::string &name);
        const Params &GetHeaders() const;

        const std::string &GetContent() const;
        void SetContent(const std::string &content);

    public:
        bool Decode(const std::string &data);
        bool Encode(std::string &data);
    private:
        void GetPath(std::string& path) const;
    private:
        const char *ReadRequestLine(const char *begin, const char *end);
        const char *ReadMethod(const char *begin, const char *end);
        const char *ReadSpace(const char *begin, const char *end);
        const char *ReadUri(const char *begin, const char *end);
        const char *ReadVersion(const char *begin, const char *end);
        const char *ReadHeaders(const char *begin, const char *end);
        const char *ReadHeader(const char *begin, const char *end);

    private:
        Params headers_;
        std::string version_;
        std::string uri_;
        std::string method_;
        std::string content_;
        std::string path_;
        std::string host_;
    };

}

#endif /* HTTP_HTTPREQUEST_H_ */

