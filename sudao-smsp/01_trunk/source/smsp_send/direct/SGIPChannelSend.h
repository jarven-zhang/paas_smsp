#ifndef SGIPCHANNELSEND_H_
#define SGIPCHANNELSEND_H_

#include "IChannel.h"
#include "StateReport.h"
#include "SGIPBase.h"
#include "sockethandler.h"
#include <map>

class SGIPChannelSend: public IChannel, SocketHandler
{
	typedef void (SGIPChannelSend::*HANDLER_FUN)(pack::Reader& reader,SGIPBase& resp);
public:
	SGIPChannelSend();
	virtual ~SGIPChannelSend();
public:
	virtual void init(models::Channel& chan);
    virtual UInt32 sendSms( smsDirectInfo &SmsInfo );	
	virtual void destroy();
	virtual void reConn();
protected:
	virtual void OnEvent(int type, InternalSocket *socket);
	virtual void OnTimer();
private:
	void onData();
	void conn();
	void disConn();
	void bindHandler();
	void onConnResp(pack::Reader& reader, SGIPBase& resp);
	void onSubmitResp(pack::Reader& reader, SGIPBase& resp);
	void onUnBindResp(pack::Reader& reader, SGIPBase& resp);
	bool onTransferData();

	void sendSgipLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);
    void sendSgipShortMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese);
private:
	UInt32 m_uHeartbeatTick;
    InternalSocket* m_pSocket;
    map<UInt32, HANDLER_FUN> _hander;
};

#endif
