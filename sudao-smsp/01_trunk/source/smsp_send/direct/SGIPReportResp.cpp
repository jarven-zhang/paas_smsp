#include "SGIPReportResp.h"
#include <stdio.h>
#include <time.h>

SGIPReportResp::SGIPReportResp()
{
	_result = 0;
    _reserve = "";
    requestId_ = SGIP_REPORT_RESP;
}

SGIPReportResp::~SGIPReportResp()
{
}

UInt32 SGIPReportResp::bodySize() const
{
	return SGIPBase::bodySize()+9;
}

bool SGIPReportResp::Pack(Writer& writer) const
{
	SGIPBase::Pack(writer);

    char reserve[9] = {0};
    snprintf(reserve,9,"%s",_reserve.data());
    
	writer(_result)((UInt32) 8,reserve);
	return writer;
}

