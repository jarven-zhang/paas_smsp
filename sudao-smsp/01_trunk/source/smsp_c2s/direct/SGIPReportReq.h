#ifndef SGIPREPORTREQ_H_
#define SGIPREPORTREQ_H_
#include "SGIPBase.h"

class SGIPReportReq: public SGIPBase
{
public:
	SGIPReportReq();
	virtual ~SGIPReportReq();
public:
	virtual bool Unpack(Reader &reader);
	virtual UInt32 bodySize() const;
	virtual bool Pack(Writer &writer) const;
public:
	UInt32 subimtSeqNumNode_;
	UInt32 subimtSeqNumTime_;
	UInt32 subimtSeqNum_;
	UInt8 queryType_;
	std::string userNumber_;
	UInt8 state_;///zhuang tai bao gao zhuang tai
	UInt8 errorCode_; ///zhuang tai bao gao tuo chuan
	std::string  reserve_;
	std::string _msgId;
	std::string _remark;
};
#endif /* SGIPDELIVERREQ_H_ */
