#ifndef SMGPCONNECTREQ_H_
#define SMGPCONNECTREQ_H_
#include "SMGPBase.h"


class SMGPConnectReq: public SMGPBase
{
public:
	SMGPConnectReq();
	virtual ~SMGPConnectReq();
	SMGPConnectReq(const SMGPConnectReq& other)
	{
		this->clientId_ = other.clientId_;
		this->authClient_ = other.authClient_;
		this->loginMode_ = other.loginMode_;
		this->timestamp_ = other.timestamp_;
		this->clientVersion_ = other.clientVersion_;

	}
	
	SMGPConnectReq& operator =(const SMGPConnectReq& other)
	{
		this->clientId_ = other.clientId_;
		this->authClient_ = other.authClient_;
		this->loginMode_ = other.loginMode_;
		this->timestamp_ = other.timestamp_;
		this->clientVersion_ = other.clientVersion_;

		return *this;
	}
	void setAccout(std::string cliendID, std::string password)
	{
		clientId_ = cliendID;
		password_ = password;
		createAuthSource();
	}
	void createAuthSource();
public:
	virtual UInt32 bodySize() const;
	virtual bool Pack(Writer &writer) const;
	virtual bool Unpack(Reader &reader);
public:
	std::string password_;
	// SMGP_LOGIN 8,16,1,4,1
	std::string clientId_;
	std::string authClient_;
	UInt8 loginMode_;
	UInt32 timestamp_;
	UInt8 clientVersion_;

};
#endif 
