#ifndef SGIP_REPORT_SOCKET_HANDLER__
#define SGIP_REPORT_SOCKET_HANDLER__

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"			
#include "internalsocket.h"

using namespace std;
class TSMSQueryReq;
class CThread;

class SgipReportSocketHandler : public SocketHandler
{	
public:
    SgipReportSocketHandler(CThread* pThread, UInt64 uSeq);
    virtual ~SgipReportSocketHandler();
	bool Init(InternalService *service, int socket, const Address &address);
	void Destroy();
	InternalSocket* m_pSocket;
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    bool onData();
	bool onTransferData(UInt32& uFlag);

	bool ContentCoding(string& strSrc,UInt8 uCodeType,string& strDst);
	bool checkLogin(string& strName,string& strpasswd);
	
	
private:
	UInt64 m_uSeq;
	CThread* m_pThread;
	UInt32 m_uChannelId;
	UInt32 m_uChannelType;
	string m_strSp;
};
#endif ////SGIP_REPORT_SOCKET_HANDLER__

