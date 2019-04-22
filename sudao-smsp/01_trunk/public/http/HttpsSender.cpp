#include "HttpsSender.h"
#include "LogMacro.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


namespace http
{
	static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)  
	{  
	    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);  
	    if( NULL == str || NULL == buffer )  
	        return -1;  
	    char* pData = (char*)buffer;  
	    str->append(pData, size * nmemb);
	    return nmemb;  
	} 
	
	HttpsSender::HttpsSender()
	{
		m_curl = curl_easy_init();  
	}

	HttpsSender::~HttpsSender()
	{
	    curl_easy_cleanup(m_curl);  
	}

	UInt32 HttpsSender::Send(UInt32 uIp, std::string& url, std::string& data, 
				std::string &strResponse, int mode, std::vector<std::string>* mpHeader )
	{
	    char* pUrl = NULL;
		
	    struct in_addr addrIp;
	    addrIp.s_addr = uIp;
	    pUrl = inet_ntoa(addrIp);

	    std::string strIp = "";
	    strIp.assign(pUrl);

	    std::string strUrl = "";

	    std::string::size_type pos = url.find_first_of("//");
	    if(pos == std::string::npos){
			LogNotice("bad uri.%s", url.data());
			return CURLE_URL_MALFORMAT;
		}

	    std::string::size_type posBegin;
	    posBegin = pos + 2;

	    strUrl = url.substr(posBegin);	

	    pos = strUrl.find_first_of(":");
	    if(pos != std::string::npos)
	    {
			strUrl = strUrl.substr(0, pos);
			if(!strIp.empty())
	        {
	            url.replace(posBegin, strUrl.size(), strIp);
			}
		}
	    else
	    {
	        pos = strUrl.find_first_of("/");
	        if (pos != std::string::npos)
	        {
			    strUrl = strUrl.substr(0, pos);
	    	}
	    	if(!strIp.empty())
	        {
				url.replace(posBegin, strUrl.size(), strIp);
	    	}
	    }

		if (NULL == mpHeader)
		{
			std::vector<std::string> mv;
			mpHeader = &mv;
		}

		mpHeader->push_back("Host: " + strUrl );

		std::string  strKeepLive = "Connection: ";
		strKeepLive.append( "Keep-Alive" );		
		mpHeader->push_back( strKeepLive );
		
		return CurlPerform(url, data, strResponse, mode, mpHeader);
	} 


	int HttpsSender::CurlPerform(const std::string& strUrl, const std::string& strPost,
									std::string& strResponse, int mode, std::vector<std::string>* mpHeader)
	{  
	    CURLcode res;  
		
	    if( NULL == m_curl )
	    {
			LogError("curl init err!");
			return CURLE_FAILED_INIT; 
		}

		/*重置数据，保留会话*/
		curl_easy_reset( m_curl );

	    struct curl_slist *slist=NULL;
	    if( mpHeader != NULL && mpHeader->size() > 0 )
	    {
	        std::vector <std::string>::iterator mv_Iter;
	        for(mv_Iter = mpHeader->begin(); mv_Iter!=mpHeader->end(); mv_Iter++)
	        {
	            slist = curl_slist_append(slist, (*mv_Iter).c_str());
	        }
	        
	        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, slist);
	    }
	         
	    curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str());  
	    if(HTTPS_MODE_GET != mode){
		    curl_easy_setopt(m_curl, CURLOPT_POST, 1);
		    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, strPost.c_str());
	    }	
	    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);  
	    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);  
	    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);  
	    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);  
	    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, false);  
	    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 3);  
	    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 3);
	    res = curl_easy_perform(m_curl);
	    
	    if (NULL != slist)
	    {
	        curl_slist_free_all(slist);
	    }
		
	    return res;
		
	}

	/*判断当前会话是否失效*/
	bool HttpsSender::isAvailable()
	{
		long sockfd = -1;
		if( !m_curl ){
			LogWarn(" Error: m_curl NULL");
			return false;
		}
		
		CURLcode res = curl_easy_getinfo(m_curl, CURLINFO_LASTSOCKET, &sockfd);
		if( res != CURLE_OK || sockfd == -1 ) 
		{
			LogWarn( "sockfd:%d, Error: %s\n", sockfd, curl_easy_strerror(res));
			return false;
		}
		return true;
	}
}
