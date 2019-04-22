#ifndef HTTPSEND_H_
#define HTTPSEND_H_
#include "platform.h"

#include <vector>
#include "sockethandler.h"
#include "hiredis.h"
#include "internalservice.h"
#include "address.h"
#include "eventtype.h"
///#include "socket.h"
#include "internalsocket.h"
#include "sockethandler.h"
#include "inputbuffer.h"
#include "outputbuffer.h"

using namespace std;
class CThreadWheelTimer;
class CThread;

namespace http
{	
	
    class HttpSender:public SocketHandler
    {
    public:
        HttpSender();
        virtual ~HttpSender();
		bool Init(InternalService* pInternalService, UInt64 iSeq, CThread* pThread);
        void Get(std::string& url);
        void Post(std::string& url,std::string& data, std::vector<std::string>* mpHeader = NULL);
        void setIPCache(UInt32 ip)
        {
            _ip = ip;
            _useCache = true;
        }
        void Destroy();

    protected:
        virtual void OnEvent(int type, InternalSocket *socket);
    private:
        bool OnData(const char *data, UInt32 size);
    private:
        InternalSocket* _socket;
		UInt64 m_uSeq;
        UInt32 _ip;
        bool _useCache;
       	InternalService* m_pInternalService;
		CThread* m_pThread;
		
		string m_data;	//httpResponse quan bao wen
		bool	m_bIsComplete;
    };

} /* namespace web */
#endif /* HTTPSEND_H_ */
