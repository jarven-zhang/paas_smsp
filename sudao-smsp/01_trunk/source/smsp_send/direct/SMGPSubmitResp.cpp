#include "SMGPSubmitResp.h"
#include <stdio.h>
#include "main.h"

SMGPSubmitResp::SMGPSubmitResp()
{
	msgId_="";
	status_=0;
}

SMGPSubmitResp::~SMGPSubmitResp()
{
}

bool SMGPSubmitResp::Unpack(Reader& reader)
{
	msgId_.reserve(11);
	unsigned char msgID[10]={0};
	char msg[30]={0};
	reader((UInt32)10,(char*)msgID)(status_);
	char* pos=msg;
	for(int i=0;i<10;i++)
    {
		snprintf(pos,30,"%02x",msgID[i]);
		pos+=2;
	}
	msgId_=msg;

	//add by fangjinxiong 20161117.
	UInt32 ureaded = 10 + 4;
	int ileftByte = packetLength_ - ureaded - SMGPBase::bodySize();
	if(ileftByte > 0)
	{
		LogDebug("ileftByte[%d]", ileftByte);
		string strtmp;
		reader((UInt32)ileftByte, strtmp);
	}
	//add end
	
	return reader;
}
