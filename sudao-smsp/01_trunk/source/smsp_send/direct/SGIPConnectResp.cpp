#include "SGIPConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

SGIPConnectResp::SGIPConnectResp()
{
	isOK_ = false;
	result_ = 0;
    requestId_ = SGIP_BIND_RESP;
}

SGIPConnectResp::~SGIPConnectResp()
{
}

UInt32 SGIPConnectResp::bodySize() const
{
    return SGIPBase::bodySize() +9;
}


std::string SGIPConnectResp::getAuthServer()
{

	return (const char*) NULL;
}
bool SGIPConnectResp::Pack(Writer& writer) const
{
    SGIPBase::Pack(writer);
    char reserve[8]={0};
    snprintf(reserve,8,"%s",reserve_.data());
    writer(result_)((UInt32)8, reserve);
	return writer;
}

bool SGIPConnectResp::Unpack(Reader& reader)
{
	if(reader.GetSize() == 0)
    {
        LogError("reader.GetSize is 0.");
		return false;
	}

	reader(result_)(8, reserve_);
	if ((requestId_ == SGIP_BIND_RESP) && (0 == result_))
    {
		isOK_ = true;
	}
	else
	{
		isOK_ = false;
	}
	
    LogDebug("result[%d], reserve[%s], isOK_[%d].", result_, reserve_.data(), isOK_);

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

