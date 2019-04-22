#include "SGIPSubmitReq.h"
#include <stdio.h>
#include "UTFString.h"
#include "main.h"

SGIPSubmitReq::SGIPSubmitReq()
{
    requestId_ = SGIP_SUBMIT;
	spNum_ = "";//21
	chargeNum_ = "000000000000000000000";//21
	userCount_ = 1;
	userNum_ = "";//21
	corpId_ = "";//5
	serviceType_ = "";//10
	feeType_ = 0;
	feeValue_ = "";//6
	givenValue_ = "";
	agentFlag_ = 0;
	morelatetoMTFalg_ = 1;
	priority_ = 0;
	expireTime_ = "";//16
	scheduleTime_ = "";//16
	reportFlag_ = 1;
	tpPid_ = 0;
	tpUdhi_ = 0;
	msgCoding_ = 8;
	msgType_ = 0;
	msgContent_ = "";
	msgLength_ = 0;
	reserve_ = "";
}

SGIPSubmitReq::~SGIPSubmitReq()
{

}

UInt32 SGIPSubmitReq::bodySize() const
{
	return msgContent_.size() + 144 + SGIPBase::bodySize();
}

bool SGIPSubmitReq::Pack(Writer& writer) const 
{
    SGIPBase::Pack(writer);
	char spNum[32] = {0};
	snprintf(spNum, 32,"%s", spNum_.data());
    
	char chargeNum[32] = {0};
	snprintf(chargeNum, 32,"%s", chargeNum_.data());
	
	char userNum[32] = {0};
	snprintf(userNum, 32,"%s", userNum_.data());

	char corpId[16] = {0};
	snprintf(corpId, 16,"%s", corpId_.data());

	char serviceType[16] = {0};
	snprintf(serviceType, 16,"%s", serviceType_.data());

	char feeValue[16] = {0};
	snprintf(feeValue, 16,"%s", feeValue_.data());

	char givenValue[16] = {0};
	snprintf(givenValue, 16,"%s", givenValue_.data());

	char expireTime[17] = {0};
	snprintf(expireTime, 17,"%s", expireTime_.data());

	char scheduleTime[17] = {0};
	snprintf(scheduleTime, 17,"%s", scheduleTime_.data());

    char reserve[9] = {0};
    snprintf(reserve, 9,"%s", reserve_.data());


    char msgId[256]={0};
	snprintf(msgId,256,"%d%d%u",sequenceIdNode_, sequenceIdTime_, sequenceId_);
	
	LogNotice("******submit msgid[%s]",msgId);

	writer((UInt32)21,spNum)((UInt32)21,chargeNum)(userCount_)((UInt32)21,userNum)(
			(UInt32)5,corpId)((UInt32) 10,serviceType)(feeType_)((UInt32) 6, feeValue)(
			(UInt32)6,givenValue)(agentFlag_)(morelatetoMTFalg_)(priority_)((UInt32) 16, expireTime)(
			(UInt32) 16, scheduleTime)(reportFlag_)(tpPid_)(tpUdhi_)(msgCoding_)(msgType_)(
			msgLength_)((UInt32) msgLength_, msgContent_.data())((UInt32) 8,reserve);

	return writer;
}

bool SGIPSubmitReq::Unpack(Reader& reader)
{
	return false;
}

