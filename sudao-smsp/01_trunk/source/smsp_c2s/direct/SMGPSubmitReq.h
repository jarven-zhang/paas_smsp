#ifndef SMGPSUBMITREQ_H_
#define SMGPSUBMITREQ_H_
#include "SMGPBase.h"
#include <vector>

namespace smsp
{
	class SMGPSubmitReq: public SMGPBase
	{
	public:
		SMGPSubmitReq();
		virtual ~SMGPSubmitReq();
	public:
		virtual UInt32 bodySize() const;
		virtual bool Pack(Writer &writer) const;
		virtual bool Unpack(Reader &reader);
		void setContent(std::string content);
		void setContent(char* content,UInt32 len)
	    {
	        msgContent_.reserve(20480);
	        msgContent_.append(content,len);
	        msgLength_ = len;
	    }
	public:


		//SMGP_SUBMIT 1,1,1,10,2,6,6,1,17,17,21,21,1,21,1,msglength,8
		UInt8 msgType_;
		UInt8 needReport_;
		UInt8 priority_;
		std::string  serviceId_;
		std::string feeType_;
		std::string feeCode_;
		std::string fixedFee_;
		UInt8 msgFormat_;
		std::string validTime_;
		std::string atTime_;
		std::string srcTermId_;
		std::string chargeTermId_;
		UInt8 destTermIdCount_;
		std::string destTermId;
		UInt8 msgLength_;
		std::string msgContent_;
		std::string reserve_;
		
		std::string phone_;
		UInt32 m_uFlag;  //// 0 is short message, 1 is long message
		UInt8 longSmProtocalHeadType_;
		UInt8 randCode8_;   
		UInt16 randCode_;
		UInt8 pkTotal_;
		UInt8 pkNum_;
		std::vector<std::string> phonelist_;
	};
}
#endif
