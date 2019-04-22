#ifndef __C_WHITEKEYWORD_ROUTER_STRATEGY_H__
#define __C_WHITEKEYWORD_ROUTER_STRATEGY_H__

#include "CRouterStrategy.h"
#include "Comm.h"
#include "ChannelWhiteKeyword.h"

class CWhiteKeywordRouterStrategy : public CRouterStrategy
{
public:
    CWhiteKeywordRouterStrategy();
    virtual ~CWhiteKeywordRouterStrategy();

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);
protected:
    //virtual Int32 CheckRoute(const models::Channel& smsChannel,string& strReason);

private:
	bool SelectChannelFromWhiteKeyword(string& strKeyword,ChannelWhiteKeywordMap& channelWhiteKeywordmap,models::Channel& smsChannel);
	bool CheckWhiteKeyword(ChannelWhiteKeyword& channelWhiteKeyword);
	string m_strSign;
	string m_strContent;
};

#endif    //__C_WHITEKEYWORD_ROUTER_STRATEGY_H__