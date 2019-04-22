#include "CForeignRouterReback.h"

CForeignRouterReback::CForeignRouterReback()
 : CForeignRouter()
{

}

CForeignRouterReback::~CForeignRouterReback()
{

}

bool CForeignRouterReback::CheckChannelQueueSize(const models::Channel& smsChannel)
{
	return true;
}