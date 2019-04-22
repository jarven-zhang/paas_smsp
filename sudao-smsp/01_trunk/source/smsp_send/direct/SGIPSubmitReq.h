#ifndef SGIPSUBMITREQ_H_
#define SGIPSUBMITREQ_H_
#include "SGIPBase.h"

class SGIPSubmitReq: public SGIPBase
{
public:
	SGIPSubmitReq();
	virtual ~SGIPSubmitReq();
public:
	virtual UInt32 bodySize() const;
	virtual bool Pack(Writer &writer) const;
	virtual bool Unpack(Reader &reader);

	void setContent(std::string content)
	{
		msgContent_=content;
		msgLength_ = msgContent_.length();
	}
	void setContent(char* content,UInt32 len)
    {
        msgContent_.reserve(20480);
        msgContent_.append(content,len);
        msgLength_ = len;
    }
public:
	//SGIP_SUBMIT
	std::string spNum_;//21    接入号
	std::string chargeNum_;//21
	UInt8 userCount_;
	std::string userNum_;//21  手机号码

	std::string corpId_;//5    企业代码
	std::string serviceType_;//10
	UInt8 feeType_;
	std::string feeValue_;//6
	std::string givenValue_;
	UInt8 agentFlag_;
	UInt8 morelatetoMTFalg_;
	UInt8 priority_;
	std::string expireTime_;//16
	std::string scheduleTime_;//16
	UInt8 reportFlag_;  ////reportflag  1

	UInt8 tpPid_;
	UInt8 tpUdhi_;     ////long message 0x01
	UInt8 msgCoding_;  //// code type
	UInt8 msgType_;
	UInt32 msgLength_;
	std::string msgContent_;
	std::string reserve_;//8
};

#endif
