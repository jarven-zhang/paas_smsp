#include "CDomesticNumberRouterReback.h"

CDomesticNumberRouterReback::CDomesticNumberRouterReback()
 : CDomesticNumberRouter()
{

}

CDomesticNumberRouterReback::~CDomesticNumberRouterReback()
{

}


bool CDomesticNumberRouterReback::CheckChannelQueueSize(const models::Channel& smsChannel)
{
	return true;
}

