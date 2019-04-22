#include "SMGPSubmitReq.h"
#include <stdio.h>
#include <stdlib.h>
#include "UTFString.h"
#include "main.h"

namespace smsp
{
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
		pkTotal_ = 1;
		pkNum_ = 1;	
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
    	UInt32 headLength = 12;
		UInt32 bodyLength = 0;
		UInt32 optLength = 0;  
		//UInt8 PhoneCount = 0;
		string onePhoneTmp = "";
		string onePhone;
		string tempContent = "";
		m_uFlag = 0;
		
        if(reader.GetSize() == 0)
			return false;
		reader(msgType_)(needReport_)(priority_)((UInt32)10,serviceId_)((UInt32)2,feeType_)((UInt32)6,feeCode_)
			((UInt32)6,fixedFee_)(msgFormat_)((UInt32)17,validTime_)((UInt32)17,atTime_)((UInt32)21,srcTermId_)
			((UInt32)21,chargeTermId_)(destTermIdCount_);
		////LogDebug("phone num is %d",destTermIdCount_);
		
		int i = 0;
		for(;i < destTermIdCount_;i ++)
		{
			if( ! reader((UInt32)21,onePhoneTmp))
				return reader;
			onePhone = onePhoneTmp.data();
			///LogDebug("==phone[%s]. num[%d]",onePhone.data(),i);
			phonelist_.push_back(onePhone);
			onePhone.clear();
		}
		////LogDebug("phonenum = [%d]",phonelist_.size());
		reader(msgLength_);
		////LogDebug("==msgLength[%d].",msgLength_);
		
		reader((UInt32)msgLength_,tempContent)((UInt32)8,reserve_);
		LogDebug("==tempContent[%s],tempContentLen[%d].",tempContent.data(),tempContent.length());

		
		
		bodyLength = headLength + 114 + 21 * destTermIdCount_ + msgLength_;
		UInt16 pid_t = 0;
		UInt16 udhi_t =0 ;
		UInt16 tempLength = 0;
		UInt16 tempTag = 0;
		char tempValue[1024] = {0};
		UInt32 contentHead = 0;
		optLength = packetLength_- bodyLength;
		/////LogDebug("==optLength[%d], packetLength_[%d], bodyLength[%d].",optLength, packetLength_, bodyLength);
		
		if(optLength < 0)
		{
             return false;
		}
		else if(optLength == 0)
		{
			msgContent_ = tempContent;
		}
		else
		{
			for(;optLength > 0;)
			{
				reader(tempTag);
				reader(tempLength);
				reader((UInt32)tempLength,tempValue);
				if(tempTag == 0x0001)
				{
					pid_t = tempTag;	
				}
				if(tempTag == 0x0002)
				{
					udhi_t = tempTag;
				}
				optLength = optLength - 2 - 2 - tempLength;
			}
			if(pid_t == 0x0001 && udhi_t == 0x0002 )
			{
				m_uFlag = 1;	
			}
            
            msgContent_ = tempContent;
		}
		
		if((m_uFlag == 1) && (tempContent.length() > 6))//log mesage
		{
			/////LogDebug("long mesig!");
			char s1 = tempContent.at(0);
			int abc = s1 & 0xFF;
			///LogDebug(" abc[%d]", abc);
			if(abc == 5)
			{ 
				contentHead = 6;
				char s4 = tempContent.at(4);
				pkTotal_ = s4;
				/////LogDebug("pkTotal = %d",pkTotal_);
				char s5 = tempContent.at(5);
				pkNum_ = s5;
				/////LogDebug("pkNum = %d",pkNum_);
				msgLength_ = msgLength_ -6;
			}
			if(abc == 6)
			{	
				contentHead = 7;
				char s4 = tempContent.at(6);
				pkTotal_ = s4;
				/////LogDebug("pkTotal = %d",pkTotal_);
				char s5 = tempContent.at(7);
				pkNum_ = s5;
				/////LogDebug("pkNum = %d",pkNum_);				
				msgLength_ = msgLength_ - 7;
			}
			
			msgContent_ = tempContent.substr(contentHead,msgLength_);
			/////LogDebug("realContent =%s,realLength = %d",msgContent_.data(),msgLength_);
		}
        return reader;
    }


	void SMGPSubmitReq::setContent(std::string content)
	{
		msgContent_=content;
		msgLength_ = msgContent_.length();
	}
}
