#ifndef _CHOSTIP_H_
#define _CHOSTIP_H_

#include <map>
#include <queue>
#include <string>
#include "Thread.h"
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "platform.h"
#include <ares.h>



#define CLEAN_MAP_TIMEOUT       30*60*1000   //30min do clean.
#define CLEAN_MAP_TIMER_MSGID   3070


class HostIP
{
public:
    HostIP()
    {
        m_timeadd = 0;
    }
    void release()
    {
        delete this;
    }

    long  m_timeadd;
    std::string m_domain;
    std::string m_ip;
};

typedef std::map<std::string, HostIP*> HostMap;



class THostIpReq: public TMsg
{
public:
    std::string m_DomainUrl;
};

class THostIpResp: public TMsg
{
public:
    int m_iResult;
    UInt32 m_uIp;
};

class CHostIpThread:public CThread
{

public:
    CHostIpThread(const char *name);
    ~CHostIpThread();
    bool Init();
	void AddMap(HostIP* hostip);
	HostIP* GetMap(std::string url);

private:
	virtual void MainLoop(void);
    virtual void HandleMsg(TMsg* pMsg);
    bool HostIpReqProc(TMsg* pMsg);
	void CaresProcess();


    void CleanMap();

    HostMap m_mHostParsered;
    CThreadWheelTimer *m_pTimer;
	ares_channel m_ares;
};

class AresCbParam
{
	public:
		AresCbParam(){}

		THostIpReq* 	m_pHostIpReq;
		CHostIpThread*  m_pHostIpThread;
};

#endif

