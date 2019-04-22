#include "SMGPSubmitReq.h"
#include <stdio.h>
#include "UTFString.h"
SMGPSubmitReq::SMGPSubmitReq() 
{
	msgType_=6;
	needReport_=1;
	priority_=3;
	serviceId_ ="";
	feeType_ = "000000";
	feeCode_= "000000";
	fixedFee_= "000000";
	msgFormat_=8;
	//std::string validTime_;
	//std::string atTime_;
	srcTermId_  ="";
	//std::string chargeTermId_=1;
	destTermIdCount_=1;
	//UInt8 msgLength_;
	//std::string msgContent_;
	//std::string reserve_;
	msgLength_=0;
	
	requestId_=SMGP_SUBMIT;
	phone_ = "";
    m_uFlag = 0;

}

SMGPSubmitReq::~SMGPSubmitReq() 
{

}

UInt32 SMGPSubmitReq::bodySize() const 
{
    if (0 == m_uFlag)
    {
        return msgContent_.size() + 135 + SMGPBase::bodySize();
    }
	else
	{
        return msgContent_.size() + 135 + SMGPBase::bodySize() + 10;
    }
}

bool SMGPSubmitReq::Pack(Writer& writer) const
{
	SMGPBase::Pack(writer);
	char serviceId[11] = {0};
    snprintf(serviceId, 11,"%s", serviceId_.data());
    
	char feeType[3] = {0};
    snprintf(feeType, 3,"%s", feeType_.data());
    
	char feeCode[7] = {0};
    snprintf(feeCode, 7,"%s", feeCode_.data());
    
	char fixedFee[7] = {0};
    snprintf(fixedFee, 7,"%s", fixedFee_.data());
    
	char valIdTime[18] = {0};
	char atTime[18] = {0};
    
	char srcTermId[31] = {0};
	snprintf(srcTermId, 31,"%s", srcTermId_.data());
	
	
	
	
	const char reserve[9] = {0};
    
	char chargeTermId[22] = {0};
	snprintf(chargeTermId, 22,"%s", phone_.data());
    
	char destTermId[22]={0};
	snprintf(destTermId, 22,"%s", phone_.data());
    
    if (0 == m_uFlag)
    {
        writer(msgType_)(needReport_)(priority_)((UInt32)10,serviceId)((UInt32)2,feeType)((UInt32) 6,feeCode)
            ((UInt32)6,fixedFee)(msgFormat_)((UInt32) 17, valIdTime)((UInt32) 17, atTime)((UInt32) 21, srcTermId)
            ((UInt32) 21, chargeTermId)(destTermIdCount_)((UInt32)21,destTermId)(msgLength_)
            ((UInt32) msgLength_, msgContent_.data())((UInt32) 8,reserve);
    }
    else
    {
        UInt16 uPid = 0x0001;
        UInt16 uUdhi = 0x0002;
        UInt16 uLength = 0x0001;
        UInt8 uPidValue = 0x00;
        UInt8 uUdhiValue = 0x01;
        writer(msgType_)(needReport_)(priority_)((UInt32)10,serviceId)((UInt32)2,feeType)((UInt32) 6,feeCode)
            ((UInt32)6,fixedFee)(msgFormat_)((UInt32) 17, valIdTime)((UInt32) 17, atTime)((UInt32) 21, srcTermId)
            ((UInt32) 21, chargeTermId)(destTermIdCount_)((UInt32)21,destTermId)(msgLength_)
            ((UInt32) msgLength_, msgContent_.data())((UInt32) 8,reserve)
            (uPid)(uLength)(uPidValue)(uUdhi)(uLength)(uUdhiValue);
    }
	
	
	return writer;

}

bool SMGPSubmitReq::Unpack(Reader& reader)
{
	return false;
}

void SMGPSubmitReq::setContent(std::string content)
{
	msgContent_=content;
	msgLength_ = msgContent_.length();
}
