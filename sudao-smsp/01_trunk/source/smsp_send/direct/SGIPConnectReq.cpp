#include "SGIPConnectReq.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssl/md5.h"
#include "main.h"

SGIPConnectReq::SGIPConnectReq()
{
	requestId_ = SGIP_BIND;
	loginType_ = 0x01;
	loginName_ = "";
	loginPassword_ = "";
	reserve_ = "";
}

SGIPConnectReq::~SGIPConnectReq() 
{
}

UInt32 SGIPConnectReq::bodySize() const
{
	return 1 + 16 + 16 + 8 + SGIPBase::bodySize();
}

bool SGIPConnectReq::Pack(Writer& writer) const
{
	SGIPBase::Pack(writer);
	char loginName[17]={0};
    snprintf(loginName,17,"%s",loginName_.data());
    
	char loginPassword[17]={0};
    snprintf(loginPassword,17,"%s",loginPassword_.data());
    
	char reserve[9]={0};
	snprintf(reserve,9,"%s",reserve_.data());
    
	writer(loginType_)((UInt32)16, loginName)((UInt32)16, loginPassword)((UInt32)8, reserve);
	return writer;
}

bool SGIPConnectReq::Unpack(Reader& reader)
{
	if(reader.GetSize() == 0)
    {
		return false;
	}
	
	reader(loginType_)((UInt32)16, loginName_)((UInt32)16, loginPassword_)((UInt32)8, reserve_);
	if (requestId_ == SGIP_BIND)
    {
        LogDebug("loginType[%d],loginName[%s],loginPassWord[%s],reserve[%s].",loginType_,loginName_.data(),loginPassword_.data(),reserve_.data());
	}

	//add by fangjinxiong 20161117.
	int ileftByte = packetLength_ -1 -16 -16 - 8 -SGIPBase::bodySize();	//rewrite bodySize
	if(ileftByte > 0)
	{
		LogDebug("ileftByte[%d]", ileftByte);
		string strtmp;
		reader((UInt32)ileftByte, strtmp);
	}
	//add end
	
    return reader;
}


