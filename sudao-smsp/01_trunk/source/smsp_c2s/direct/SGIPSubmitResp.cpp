#include "SGIPSubmitResp.h"
#include <stdio.h>
#include "main.h"

SGIPSubmitResp::SGIPSubmitResp()
{
	_msgId="";
	_reserve="";
	_result=0xff;
    
    requestId_ = SGIP_SUBMIT_RESP;
}

SGIPSubmitResp::~SGIPSubmitResp()
{
}

UInt32 SGIPSubmitResp::bodySize() const
{
    return 9 + SGIPBase::bodySize();
}

bool SGIPSubmitResp::Unpack(Reader& reader)
{
    /*
	reader(_result)(8, _reserve);

	char msgId[256]={0};
	snprintf(msgId,256,"%d%d%u",sequenceIdNode_, sequenceIdTime_, sequenceId_);
	_msgId = std::string(msgId);
	
	LogDebug("****** submitresp result:[%d] reserve:[%s] msgid[%s]\n",_result,_reserve.data(),_msgId.data());
    
	return reader;
	*/
	return false;
}

bool SGIPSubmitResp::Pack(Writer &writer) const
{
    SGIPBase::Pack(writer);

    char cReserve[9] = {0};
    
    writer(_result)((UInt32)8,cReserve);

    return writer;
}

