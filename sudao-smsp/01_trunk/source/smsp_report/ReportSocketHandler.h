#ifndef PLUGINS_WEB_REPORTSOCKETHANDLER_H_
#define PLUGINS_WEB_REPORTSOCKETHANDLER_H_

#include <string>
//#include "MsgType.h"
#include "ThreadMacro.h"

#include "eventtype.h"
//#include "socket.h"				
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"		
#include "httprequest.h"	
#include "httpresponse.h"	
#include "internalsocket.h"
#include "address.h"

using namespace std;


class TSMSQueryReq;
class ReportSocketHandler : public SocketHandler
{	
	typedef std::map<std::string, std::string> EnvironmentMap;
public:
    ReportSocketHandler(CThread* pThread, UInt64 uSeq);
    virtual ~ReportSocketHandler();
	bool Init(InternalService *service, int socket, const Address &address);
	std::string GetClientAddress();
	void Destroy();
	InternalSocket *m_socket;
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    bool OnData(const char *data, UInt32 size);
	std::string _retcode2Json(string strReturncode);
	void ProcessRequest(std::string result);   
    void Status404();
    void Status500();
	void Toupper(std::string & str);
private:
	CThread* m_pThread;
	UInt64 m_uSeq;
	
	string m_data;	//httpResponse quan bao wen
	bool	m_bIsComplete;
	std::string	m_strClientAddr;
};



#endif /* PLUGINS_WEB_REPORTSOCKETHANDLER_H_ */

