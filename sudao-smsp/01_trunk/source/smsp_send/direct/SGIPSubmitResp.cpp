#include "SGIPSubmitResp.h"
#include <stdio.h>
#include "main.h"

SGIPSubmitResp::SGIPSubmitResp()
{
	_msgId="";
	_reserve="";
	_result=0xff;

}

SGIPSubmitResp::~SGIPSubmitResp()
{
}

bool SGIPSubmitResp::Unpack(Reader& reader)
{
	reader(_result)(8, _reserve);

	char msgId[256]={0};
	snprintf(msgId,256,"%u%u%u",sequenceIdNode_, sequenceIdTime_, sequenceId_);
	_msgId = std::string(msgId);
	
	LogDebug("****** submitresp result:[%d] reserve:[%s] msgid[%s]\n",_result,_reserve.data(),_msgId.data());

	//add by fangjinxiong 20161117.
	int ileftByte = packetLength_ -1 -8 - SGIPBase::bodySize();	
	if(ileftByte > 0)
	{
		LogDebug("ileftByte[%d]", ileftByte);
		string strtmp;
		reader((UInt32)ileftByte, strtmp);
	}
	//add end
	
	return reader;
}

