#include "SMGPConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace smsp
{
	SMGPConnectResp::SMGPConnectResp()
	{
		isOK_ = false;
		status_ = 0;
		serverVersion_ = 0;
		authClient_ = "";
		password_ = "";
		requestId_ = SMGP_LOGIN_RESP;
	}

	SMGPConnectResp::~SMGPConnectResp() 
	{
	}
	
	UInt32 SMGPConnectResp::bodySize() const 
	{
		return 4 + 16 + 1 + SMGPBase::bodySize();
	}

/*	std::string SMGPConnectResp::getAuthServer()
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
*/
	bool SMGPConnectResp::Pack(Writer& writer) const
	{
    	SMGPBase::Pack(writer);
		char authISMG[17]= {0};	//todo...not null.
        writer(status_)((UInt32)16, authISMG)(serverVersion_);
        return writer;
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
		return reader;
	}
}
