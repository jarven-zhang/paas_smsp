#ifndef __SGIPPUSH__
#define __SGIPPUSH__

#include "SGIPBase.h"
#include <map>
#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"			
#include "internalsocket.h"

using namespace std;


enum RtAndMoLinkState
{
	CONNECT_STATUS_OK = 0,
	CONNECT_STATUS_NO = 1,
	CONNECT_STATUS_INIT = 2,
};


class CSgipPush: public SocketHandler
{
public:
	CSgipPush();
	virtual ~CSgipPush();
public:
	virtual void init(string& strIp,UInt16 uPort,string& strClientId,string& strPwd);
	virtual void destroy();
protected:
	virtual void OnEvent(int type, InternalSocket *socket);
private:
	void onData();
	void conn();
	void disConn();
	void reConn();
	void onConnResp(pack::Reader& reader, SGIPBase& resp);
	bool onTransferData();

public:
	vector<TMsg*> m_vecRtAndMo;
	UInt32 m_uState;

	CThreadWheelTimer* m_ExpireTimer;
	string	m_strClientId;
	InternalSocket* m_pSocket;
private:
    
	string m_strPwd;
	string m_strIp;
	UInt16 m_uPort;
};

#endif ////SGIPPUSH
