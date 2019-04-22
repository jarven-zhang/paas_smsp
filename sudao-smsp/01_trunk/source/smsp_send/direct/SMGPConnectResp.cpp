#include "SMGPConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

SMGPConnectResp::SMGPConnectResp()
{
	isOK_ = false;
	status_ = 0;
	serverVersion_ = 0;
	authClient_ = "";
	password_ = "";
}

SMGPConnectResp::~SMGPConnectResp() 
{
}

std::string SMGPConnectResp::getAuthServer()
{
	unsigned char md5[16] = { 0 };
	char target[1024] = { 0 };
	char* pos = target;
	memcpy(pos, &status_, sizeof(status_));
	pos += sizeof(status_);
	memcpy(pos, authClient_.data(), authClient_.size());
	pos += authClient_.size();
	memcpy(pos, password_.data(), password_.size());
	pos += password_.size();

	MD5((const unsigned char*) target, pos - target, md5);
	return (const char*) md5;
}
bool SMGPConnectResp::Pack(Writer& writer) const
{
	return false;
}

bool SMGPConnectResp::Unpack(Reader& reader)
{
	if (reader.GetSize() == 0)
    {
		return false;
	}
	reader(status_)((UInt32)16, authServer_)(serverVersion_);
	if (status_ == 0)
    {
		isOK_ = true;
	}


	//add by fangjinxiong 20161117.
	UInt32 ureaded = 4 + 16 + 1;
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
