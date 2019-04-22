#ifndef PLUGINS_WEB_SENDSOCKETHANDLER_H_
#define PLUGINS_WEB_SENDSOCKETHANDLER_H_

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"
#include "inputbuffer.h"
#include "outputbuffer.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "internalsocket.h"

using namespace std;


class TSMSQueryReq;
class CThread;

class AccessHttpSocketHandler : public SocketHandler
{
public:
    AccessHttpSocketHandler(CThread *pThread, UInt64 uSeq);
    virtual ~AccessHttpSocketHandler();
    bool Init(InternalService *service, int socket, const Address &address);
    string GetServerIpAddress();
    void Destroy();
    InternalSocket *m_socket;
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    bool OnData(const char *data, UInt32 size);
    void ProcessRequest(std::string result);
    string _retcode2Json(string strReturncode);
    void Status404();
    void Status500();
private:
    UInt64 		m_ullSeq;
    CThread 	*m_pThread;
    string 		m_data;					// httpResponse quan bao wen
    bool		m_bIsComplete;			// 包体是否完整
    string 		m_strServerIpAddress;	// 服务ip地址
};

#endif /* PLUGINS_WEB_SENDSOCKETHANDLER_H_ */

