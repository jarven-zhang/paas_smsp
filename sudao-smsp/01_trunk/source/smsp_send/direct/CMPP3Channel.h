#ifndef __CMPP3CHANNEL_H_
#define __CMPP3CHANNEL_H_

#include "IChannel.h"
#include "StateReport.h"
#include "CMPP3Base.h"
#include "sockethandler.h"
#include <map>

using namespace std;
using namespace cmpp3;
class CMPP3Channel: public IChannel, SocketHandler
{
    typedef void (CMPP3Channel::*HANDLER_FUN)(pack::Reader& reader,UInt32 sequenceId, UInt32 packetLen);
public:
    CMPP3Channel();
    virtual ~CMPP3Channel();
public:
    virtual void init(models::Channel& chan);
    virtual UInt32 sendSms( smsDirectInfo &SmsInfo );
    virtual void destroy();
	virtual void reConn();
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    virtual void OnTimer();
private:
    void sendHeartbeat();
    void respDeliver(UInt32 seq, UInt64 msgId, int result);

	
private:
    void onData();
    void conn();
    void bindHandler();

    void onConnResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
    void onSubmitResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
    void onDeliver(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
    void onHeartbeat(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
	void onHeartbeatReq(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
    bool onTransferData();

    void sendCmppLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal);
    void sendCmppShortMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);

private:
    UInt32 m_uHeartbeatTick;
    InternalSocket* m_pSocket;
    map<UInt32, HANDLER_FUN> _hander;
};

#endif ////__CMPPCHANNEL_H_
