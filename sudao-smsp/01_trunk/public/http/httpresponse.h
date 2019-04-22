#ifndef HTTP_HTTPRESPONSE_H_
#define HTTP_HTTPRESPONSE_H_
#include "platform.h"

#ifndef API
#include <string>
#include <map>
#endif /* API */

#include "params.h"

namespace http
{

    class HttpResponse
    {
    public:
        HttpResponse();
        virtual ~HttpResponse();

    public:
        const std::string &GetContent() const;
        void SetContent(const std::string &content);
		bool EncodeTextHtml(std::string &data);

        const std::string &GetProtocolVersion() const;
        void SetProtocolVersion(const std::string &version);

        int GetStatusCode() const;
        void SetStatusCode(int code);
        std::string GetReasonPhrase() const;

        bool GetHeader(const std::string &name, std::string &value) const;
        void SetHeader(const std::string &name, const std::string &value);
        void RemoveHeader(const std::string &name);

        const Params &GetHeaders() const;

    public:
        bool Decode(const std::string &data);
        bool Encode(std::string &data);

    private:
        const char *ReadStatusLine(const char *begin, const char *end);
        const char *ReadVersion(const char *begin, const char *end);
        const char *ReadSpace(const char *begin, const char *end);
        const char *ReadStatus(const char *begin, const char *end);
        const char *ReadReason(const char *begin, const char *end);
        const char *ReadHeaders(const char *begin, const char *end);
        const char *ReadHeader(const char *begin, const char *end);

    private:
        Params headers_;
        int status_;
        std::string version_;
        std::string content_;
        std::string reason_;
    };

}

#endif /* HTTP_HTTPRESPONSE_H_ */

