#include "CDomesticRouterReback.h"

CDomesticRouterReback::CDomesticRouterReback()
 : CDomesticRouter()
{

}

CDomesticRouterReback::~CDomesticRouterReback()
{

}


bool CDomesticRouterReback::CheckChannelQueueSize(const models::Channel& smsChannel)
{
	return true;
}

