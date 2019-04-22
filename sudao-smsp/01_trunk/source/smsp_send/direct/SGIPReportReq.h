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
public:
	UInt32 subimtSeqNumNode_;
	UInt32 subimtSeqNumTime_;
	UInt32 subimtSeqNum_;
	UInt8 queryType_;
	std::string userNumber_;
	UInt8 state_;
	UInt8 errorCode_;///tuo chuan guo lai de int zhuangtaibaogao
	std::string  reserve_;
	std::string _msgId;
	std::string _remark;///int chuan huan de zhifuchuan zhuangtaibaogao
};
#endif /* SGIPDELIVERREQ_H_ */
