#ifndef __SMPPCHANNELSEND__
#define __SMPPCHANNELSEND__

#include "IChannel.h"
#include "StateReport.h"
#include "SMPPBase.h"
#include "sockethandler.h"
#include <map>
//#include "socket.h"
using namespace std;


class SMPPChannelSend: public IChannel, SocketHandler
{
    typedef void (SMPPChannelSend::*HANDLER_FUN)(pack::Reader& reader, SMPPBase& resp);
public:
    SMPPChannelSend();
    virtual ~SMPPChannelSend();
public:
    virtual void init(models::Channel& chan);    
    virtual UInt32 sendSms( smsDirectInfo &SmsInfo );
    virtual void destroy();
    void OnTimer();
	virtual void reConn();
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    ///virtual void OnTimer(net::Timer *timer);
private:
    void sendHeartbeat();
    void respDeliver(SMPPBase& resp);
private:
    void onData();
    void conn();
    void bindHandler();

    void onConnResp(pack::Reader& reader, SMPPBase& resp);
    void onSubmitResp(pack::Reader& reader, SMPPBase& resp);
    void onDeliver(pack::Reader& reader, SMPPBase& resp);
    void onHeartbeat(pack::Reader& reader, SMPPBase& resp);

    bool onTransferData();

    void sendSmppLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uShowSignType,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);
    void sendSmppShortMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent, UInt32 uShowSignType,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);

private:
    UInt32 m_uHeartbeatTick;
    InternalSocket* m_pSocket;
    map<UInt32, HANDLER_FUN> _hander;

};

#endif ///__SMPPCHANNEL__
