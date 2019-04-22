#include "SGIPConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

bool SGIPConnectResp::Pack(Writer& writer) const
{
    SGIPBase::Pack(writer);
    char reserve[9]={0};
    snprintf(reserve,9,"%s",reserve_.data());
    writer(result_)((UInt32)8, reserve);
	return writer;
}

bool SGIPConnectResp::Unpack(Reader& reader)
{
	if(reader.GetSize() == 0)
    {
		return false;
	}

	reader(result_)(8, reserve_);
	if ((requestId_ == SGIP_BIND_RESP) && (0 == result_))
    {
		isOK_ = true;
	}
	return reader;
}

