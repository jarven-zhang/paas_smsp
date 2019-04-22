#include "SGIPReportReq.h"
#include "main.h"

SGIPReportReq::SGIPReportReq()
{
	subimtSeqNumNode_ = 0;
	subimtSeqNumTime_ = 0;
	subimtSeqNum_ = 0;
	queryType_ = 0;
	userNumber_= "";
	state_ = 0;
	errorCode_ = 0;
	reserve_= "";
	_msgId = "";
}

SGIPReportReq::~SGIPReportReq() 
{

}

bool SGIPReportReq::Unpack(Reader& reader)
{
	reader(subimtSeqNumNode_)(subimtSeqNumTime_)(subimtSeqNum_)(queryType_)((UInt32)21,
		userNumber_)(state_)(errorCode_)((UInt32)8,reserve_);
	
	LogDebug("subimtSeqNumNode_:[%ld].",subimtSeqNumNode_);
	LogDebug("subimtSeqNumTime_:[%ld].",subimtSeqNumTime_);
	LogDebug("subimtSeqNum_:[%ld].",subimtSeqNum_);
	LogDebug("queryType_:[%d].",queryType_);
	LogDebug("userNumber_:[%s].",userNumber_.data());
	LogDebug("state_:[%d].",state_);
	LogDebug("errorCode_:[%d].",errorCode_);
	LogDebug("reserve_:[%s].",reserve_.data());

	char msgId[256]={0};
	snprintf(msgId,256,"%u%u%u",subimtSeqNumNode_, subimtSeqNumTime_, subimtSeqNum_);
	_msgId = std::string(msgId);
    
	char szErrorCode_[256]={0};
	snprintf(szErrorCode_,256,"%d",errorCode_);
	_remark = szErrorCode_;

	//add by fangjinxiong 20161117.    12+1+21+1+1+8 
	int ileftByte = packetLength_ -12 -1 -21 -1 -1 -8 -SGIPBase::bodySize();	//rewrite bodySize
	if(ileftByte > 0)
	{
		LogDebug("ileftByte[%d]", ileftByte);
		string strtmp;
		reader((UInt32)ileftByte, strtmp);
	}
	//add end

    return reader;
}

