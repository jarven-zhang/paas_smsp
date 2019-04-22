#ifndef SMGPCHANNELSEND_H_
#define SMGPCHANNELSEND_H_
#include "IChannel.h"
#include "StateReport.h"
#include "SMGPBase.h"
#include "sockethandler.h"
#include <map>

class SMGPChannelSend: public IChannel, SocketHandler

{
	typedef void (SMGPChannelSend::*HANDLER_FUN)(pack::Reader& reader,UInt32 sequenceId, UInt32 packetLen);	//packetLen = length from head
public:
	SMGPChannelSend();
	virtual ~SMGPChannelSend();

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
	void respDeliver(UInt32 seq, string msgId);
private:
	void onData();
	void conn();

	void bindHandler();

	void onConnResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
	void onSubmitResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
	void onDeliver(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
	void onHeartbeatResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
	void onHeartbeatReq(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen);
	bool onTransferData();
	void sendSmgpLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);
    void sendSmgpShortMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);

private:
	UInt32 m_uHeartbeatTick;
    InternalSocket* m_pSocket;
    map<UInt32, HANDLER_FUN> _hander;
};
#endif /* SMGPCHANNEL_H_ */
