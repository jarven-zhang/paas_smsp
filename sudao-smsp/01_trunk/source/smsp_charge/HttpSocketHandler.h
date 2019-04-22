#ifndef CHARGE_HTTP_SOCKETHANDLER_H_
#define CHARGE_HTTP_SOCKETHANDLER_H_

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


class CThread;
class HttpSocketHandler : public SocketHandler
{

public:
    HttpSocketHandler(CThread* pThread, UInt64 uSeq);
    virtual ~HttpSocketHandler();
    bool Init(InternalService* service, int socket, const Address& address);
    void Destroy();
    InternalSocket* m_socket;
protected:
    virtual void OnEvent(int type, InternalSocket* socket);
    bool OnData(const char* data, UInt32 size);
    void ProcessRequest(std::string result);
    void Status404();
    void Status500();
private:
    //string m_strSeq;
    UInt64 m_uSeq;
    CThread* m_pThread;
};



#endif /* CHARGE_HTTP_SOCKETHANDLER_H_ */

