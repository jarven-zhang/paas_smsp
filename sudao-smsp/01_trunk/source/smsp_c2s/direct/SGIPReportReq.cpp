#include "SGIPReportReq.h"
#include "main.h"

SGIPReportReq::SGIPReportReq()
{
	subimtSeqNumNode_ = 0;
	subimtSeqNumTime_ = 0;
	subimtSeqNum_ = 0;
	queryType_ = 1;
	userNumber_= "";
	state_ = 0;
	errorCode_ = 0;
	reserve_= "";
	_msgId = "";

    requestId_ = SGIP_REPORT;
}

SGIPReportReq::~SGIPReportReq() 
{

}

bool SGIPReportReq::Unpack(Reader& reader)
{
    /*
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
	snprintf(msgId,256,"%d%d%d",sequenceIdNode_, subimtSeqNumTime_, subimtSeqNum_);
	_msgId = std::string(msgId);
    
	char szErrorCode_[256]={0};
	snprintf(szErrorCode_,256,"%d",errorCode_);
	_remark = szErrorCode_;

    return reader;
    */
    return false;
}

UInt32 SGIPReportReq::bodySize() const
{
    return 12+1+21+1+1+8 + SGIPBase::bodySize();
}

bool SGIPReportReq::Pack(Writer &writer) const
{
    SGIPBase::Pack(writer);

    char cPhone[22] = {0};
    snprintf(cPhone,22,"%s",userNumber_.data());

    char cReserve[9] = {0};

    writer(subimtSeqNumNode_)(subimtSeqNumTime_)(subimtSeqNum_)(queryType_)
        ((UInt32)21,cPhone)(state_)(errorCode_)((UInt32)8,cReserve);

    return writer;
}

