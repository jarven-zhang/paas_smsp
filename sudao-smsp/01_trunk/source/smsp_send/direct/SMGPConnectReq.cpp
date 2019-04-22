#include "SMGPConnectReq.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssl/md5.h"


char * timestampSMGP(char *buf) 
{
	time_t tt;
	struct tm now = {0};

	time(&tt);
	localtime_r(&tt,&now);
	snprintf(buf, 64,"%02d%02d%02d%02d%02d", now.tm_mon + 1, now.tm_mday,now.tm_hour, now.tm_min, now.tm_sec);
	return buf;
}

SMGPConnectReq::SMGPConnectReq()
{
	requestId_ = SMGP_LOGIN;
	clientId_ = "";
	password_ = "";
	clientVersion_ = (Int8) (0x30);
	loginMode_ = (Int8)(2);
}

void SMGPConnectReq::createAuthSource() 
{
	//tm* ptr;
	char str[80];
	timestampSMGP(str);
	timestamp_ = atoi(str);
	unsigned char md5[16] = {0};
	char target[1024] = {0};
	char* pos = target;
	UInt32 clientIdLen = clientId_.size();
	UInt32 targetLen = clientIdLen + 7 + password_.size() + 10;
	memcpy(pos, clientId_.data(), clientIdLen);
	pos += (clientIdLen + 7);

	memcpy(pos, password_.data(), password_.size());
	pos += password_.size();

	memcpy(pos, str, 10);
	authClient_.clear();

	MD5((const unsigned char*) target, targetLen, md5);
	authClient_= std::string((const char*) md5);
}

SMGPConnectReq::~SMGPConnectReq() 
{
}

UInt32 SMGPConnectReq::bodySize() const 
{
	return 8 + 16 + 1 + 4 + 1 + SMGPBase::bodySize();
}

bool SMGPConnectReq::Pack(Writer& writer) const 
{
	SMGPBase::Pack(writer);
	char clientID[9]={0};
	snprintf(clientID,9,"%s",clientId_.data());

    char authClient[17] = {0};
    snprintf(authClient,17,"%s",authClient_.data());
    
    
	writer((UInt32)8,(const char*)clientID)((UInt32)16, authClient)(loginMode_)(timestamp_)(clientVersion_);
	return writer;
}

bool SMGPConnectReq::Unpack(Reader& reader)
{
	return false;
}
