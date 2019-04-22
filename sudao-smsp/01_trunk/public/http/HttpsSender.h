#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <curl/curl.h>

typedef unsigned int UInt32;

enum{
	HTTPS_MODE_GET = 0,
	HTTPS_MODE_POST,
};


namespace http
{
    class HttpsSender
    {
    public:
        HttpsSender();
        ~HttpsSender();
        
    public:
        UInt32 Send(UInt32 uIp, std::string& url, std::string& data,
					std::string &strResponse, int mode, std::vector<std::string>* mpHeader = NULL);	

		bool isAvailable();

    public:
        int CurlPerform(const std::string& strUrl, const std::string& strPost,
        						std::string& strResponse, int mode, std::vector<std::string>* mpHeader = NULL);


	private:
	     CURL* m_curl;	
    };
}
