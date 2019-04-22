#ifndef __TPSANDHOLDCOUNT__
#define __TPSANDHOLDCOUNT__

#include "Thread.h"
#include <iostream>
#include <string>

using namespace std;


const string COUNT_TYPE_TPS = "0";
const string COUNT_TYPE_OVER = "1";
const string COUNT_TYPE_CHARGE = "2";


class CTpsAndHoldCount
{
public:
	CTpsAndHoldCount();
	~CTpsAndHoldCount();

	static void proTpsAndHoldReq(const string& strExchange,const string& strRoutingKey,string countType,CThread* pThread,const string& strClientId,UInt32 uNum);
};

#endif ////__TPSANDHOLDCOUNT__
