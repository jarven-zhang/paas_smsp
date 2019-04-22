
#ifndef SGIPCONNECTREQ_H_
#define SGIPCONNECTREQ_H_
#include "SGIPBase.h"


class SGIPConnectReq: public SGIPBase
{
public:
	SGIPConnectReq();
	virtual ~SGIPConnectReq();
	SGIPConnectReq(const SGIPConnectReq& other)
	{
		this->loginType_=other.loginType_;
		this->loginName_=other.loginName_;
		this->loginPassword_=other.loginPassword_;
		this->reserve_=other.reserve_;
	}

	SGIPConnectReq& operator =(const SGIPConnectReq& other)
	{
		if (this == &other)
		{
			return *this;
		}
		
		this->loginType_=other.loginType_;
		this->loginName_=other.loginName_;
		this->loginPassword_=other.loginPassword_;
		this->reserve_=other.reserve_;

		return *this;
	}
	
	void setAccout(std::string loginName,std::string loginPassword)
	{
		loginName_ = loginName;
		loginPassword_ = loginPassword;
	}

public:
	virtual UInt32 bodySize() const;
	virtual bool Pack(Writer &writer) const;
	virtual bool Unpack(Reader &reader);
public:

	//SGIP_BIND
	UInt8 loginType_;            ////1
	std::string loginName_;      ////16
	std::string loginPassword_;  ////16
	std::string reserve_;        ////8
};

#endif 
